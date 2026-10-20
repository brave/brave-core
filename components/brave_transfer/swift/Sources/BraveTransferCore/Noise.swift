// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Noise_XXpsk3_25519_ChaChaPoly_SHA256 (Noise revision 34), exactly the
/// pattern Brave Transfer uses. Checked against the cacophony vector in
/// components/brave_transfer/core/test/data/noise_cacophony_xxpsk3.json.
public enum Noise {
  public static let protocolName = "Noise_XXpsk3_25519_ChaChaPoly_SHA256"
  public static let dhLength = 32
  public static let hashLength = 32
  public static let tagLength = 16
  public static let maxMessageLength = 65535

  enum Token {
    case e, s, ee, es, se, psk
  }

  static let pattern: [[Token]] = [[.e], [.e, .ee, .s, .es], [.s, .se, .psk]]
}

public struct NoiseError: Error, CustomStringConvertible {
  public let description: String
  init(_ description: String) { self.description = description }
}

private func noiseHKDF(chainingKey: Bytes, ikm: Bytes) -> (Bytes, Bytes, Bytes) {
  let temp = CryptoPrimitives.hmacSHA256(key: chainingKey, data: ikm)
  let out1 = CryptoPrimitives.hmacSHA256(key: temp, data: [0x01])
  let out2 = CryptoPrimitives.hmacSHA256(key: temp, data: out1 + [0x02])
  let out3 = CryptoPrimitives.hmacSHA256(key: temp, data: out2 + [0x03])
  return (out1, out2, out3)
}

public final class CipherState {
  public let key: Bytes?
  private(set) var nonce: UInt64 = 0

  init(key: Bytes?) {
    self.key = key
  }

  var hasKey: Bool { key != nil }

  private func nextNonce() throws -> Bytes {
    guard nonce < UInt64.max else { throw NoiseError("nonce exhausted") }
    var bytes = Bytes(repeating: 0, count: 4)
    for i in 0..<8 {
      bytes.append(UInt8(truncatingIfNeeded: nonce >> (8 * UInt64(i))))
    }
    nonce += 1
    return bytes
  }

  public func encrypt(ad: Bytes, plaintext: Bytes) throws -> Bytes {
    guard let key else { return plaintext }
    return try CryptoPrimitives.chaChaPolySeal(
      key: key, nonce: try nextNonce(), ad: ad, plaintext: plaintext)
  }

  public func decrypt(ad: Bytes, ciphertext: Bytes) throws -> Bytes {
    guard let key else { return ciphertext }
    do {
      return try CryptoPrimitives.chaChaPolyOpen(
        key: key, nonce: try nextNonce(), ad: ad, ciphertext: ciphertext)
    } catch let error as NoiseError {
      throw error
    } catch {
      throw NoiseError("decryption failed")
    }
  }
}

final class SymmetricState {
  private(set) var chainingKey: Bytes
  private(set) var h: Bytes
  private(set) var cipher = CipherState(key: nil)

  init() {
    let name = Bytes(Noise.protocolName.utf8)
    h =
      name.count <= Noise.hashLength
      ? name + Bytes(repeating: 0, count: Noise.hashLength - name.count)
      : CryptoPrimitives.sha256(name)
    chainingKey = h
  }

  func mixKey(_ ikm: Bytes) {
    let (ck, tempK, _) = noiseHKDF(chainingKey: chainingKey, ikm: ikm)
    chainingKey = ck
    cipher = CipherState(key: tempK)
  }

  func mixHash(_ data: Bytes) {
    h = CryptoPrimitives.sha256(h + data)
  }

  func mixKeyAndHash(_ ikm: Bytes) {
    let (ck, tempH, tempK) = noiseHKDF(chainingKey: chainingKey, ikm: ikm)
    chainingKey = ck
    mixHash(tempH)
    cipher = CipherState(key: tempK)
  }

  func encryptAndHash(_ plaintext: Bytes) throws -> Bytes {
    let ciphertext = try cipher.encrypt(ad: h, plaintext: plaintext)
    mixHash(ciphertext)
    return ciphertext
  }

  func decryptAndHash(_ ciphertext: Bytes) throws -> Bytes {
    let plaintext = try cipher.decrypt(ad: h, ciphertext: ciphertext)
    mixHash(ciphertext)
    return plaintext
  }

  func split() -> (CipherState, CipherState) {
    let (k1, k2, _) = noiseHKDF(chainingKey: chainingKey, ikm: [])
    return (CipherState(key: k1), CipherState(key: k2))
  }
}

/// One side of an XXpsk3 handshake. `ephemeralPrivateKey` is only fixed in
/// test vectors.
public final class NoiseHandshake {
  public let isInitiator: Bool
  private let symmetric = SymmetricState()
  private let staticPrivateKey: Bytes
  private let staticPublicKey: Bytes
  private var ephemeralPrivateKey: Bytes?
  private var psk: Bytes?
  private var messageIndex = 0

  public private(set) var remoteStaticKey: Bytes?
  public private(set) var remoteEphemeralKey: Bytes?
  public private(set) var sendCipher: CipherState?
  public private(set) var receiveCipher: CipherState?
  public private(set) var handshakeHash: Bytes?

  public init(
    initiator: Bool,
    prologue: Bytes,
    staticPrivateKey: Bytes,
    psk: Bytes? = nil,
    ephemeralPrivateKey: Bytes? = nil
  ) throws {
    isInitiator = initiator
    self.staticPrivateKey = staticPrivateKey
    staticPublicKey = try CryptoPrimitives.x25519PublicKey(privateKey: staticPrivateKey)
    self.ephemeralPrivateKey = ephemeralPrivateKey
    self.psk = psk
    symmetric.mixHash(prologue)
  }

  public var isComplete: Bool { messageIndex == Noise.pattern.count }

  public func setPSK(_ psk: Bytes) throws {
    guard psk.count == 32 else { throw NoiseError("psk must be 32 bytes") }
    self.psk = psk
  }

  private var isMyTurn: Bool { (messageIndex % 2 == 0) == isInitiator }

  private func dh(_ token: Noise.Token) throws -> Bytes {
    guard let e = ephemeralPrivateKey, let re = remoteEphemeralKey else {
      throw NoiseError("missing ephemeral key")
    }
    switch token {
    case .ee:
      return try CryptoPrimitives.x25519(privateKey: e, publicKey: re)
    case .es:
      if isInitiator {
        guard let rs = remoteStaticKey else { throw NoiseError("missing remote static") }
        return try CryptoPrimitives.x25519(privateKey: e, publicKey: rs)
      }
      return try CryptoPrimitives.x25519(privateKey: staticPrivateKey, publicKey: re)
    case .se:
      if isInitiator {
        return try CryptoPrimitives.x25519(privateKey: staticPrivateKey, publicKey: re)
      }
      guard let rs = remoteStaticKey else { throw NoiseError("missing remote static") }
      return try CryptoPrimitives.x25519(privateKey: e, publicKey: rs)
    default:
      throw NoiseError("not a DH token")
    }
  }

  public func writeMessage(payload: Bytes) throws -> Bytes {
    guard !isComplete, isMyTurn else { throw NoiseError("not our turn to write") }
    var out = Bytes()
    for token in Noise.pattern[messageIndex] {
      switch token {
      case .e:
        if ephemeralPrivateKey == nil {
          ephemeralPrivateKey = CryptoPrimitives.randomBytes(32)
        }
        let ePub = try CryptoPrimitives.x25519PublicKey(privateKey: ephemeralPrivateKey!)
        out += ePub
        symmetric.mixHash(ePub)
        symmetric.mixKey(ePub)  // psk modifier
      case .s:
        out += try symmetric.encryptAndHash(staticPublicKey)
      case .psk:
        guard let psk else { throw NoiseError("psk not set") }
        symmetric.mixKeyAndHash(psk)
      case .ee, .es, .se:
        symmetric.mixKey(try dh(token))
      }
    }
    out += try symmetric.encryptAndHash(payload)
    guard out.count <= Noise.maxMessageLength else { throw NoiseError("message too long") }
    advance()
    return out
  }

  public func readMessage(_ message: Bytes) throws -> Bytes {
    guard !isComplete, !isMyTurn else { throw NoiseError("not our turn to read") }
    guard message.count <= Noise.maxMessageLength else { throw NoiseError("message too long") }
    var pos = 0
    for token in Noise.pattern[messageIndex] {
      switch token {
      case .e:
        guard message.count - pos >= Noise.dhLength else { throw NoiseError("truncated message") }
        let re = Bytes(message[pos..<pos + Noise.dhLength])
        pos += Noise.dhLength
        remoteEphemeralKey = re
        symmetric.mixHash(re)
        symmetric.mixKey(re)
      case .s:
        let n = Noise.dhLength + (symmetric.cipher.hasKey ? Noise.tagLength : 0)
        guard message.count - pos >= n else { throw NoiseError("truncated message") }
        remoteStaticKey = try symmetric.decryptAndHash(Bytes(message[pos..<pos + n]))
        pos += n
      case .psk:
        guard let psk else { throw NoiseError("psk not set") }
        symmetric.mixKeyAndHash(psk)
      case .ee, .es, .se:
        symmetric.mixKey(try dh(token))
      }
    }
    if symmetric.cipher.hasKey && message.count - pos < Noise.tagLength {
      throw NoiseError("truncated message")
    }
    let payload = try symmetric.decryptAndHash(Bytes(message[pos...]))
    advance()
    return payload
  }

  private func advance() {
    messageIndex += 1
    guard isComplete else { return }
    let (c1, c2) = symmetric.split()
    (sendCipher, receiveCipher) = isInitiator ? (c1, c2) : (c2, c1)
    handshakeHash = symmetric.h
  }
}
