// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

#if canImport(CryptoKit)
import CryptoKit
#else
import Crypto
#endif

/// Incremental SHA-256, for hashing items before offering them.
public struct SHA256Digest {
  private var hasher = SHA256()

  public init() {}

  public mutating func update(_ data: Bytes) {
    hasher.update(data: data)
  }

  public func finalize() -> Bytes {
    Bytes(hasher.finalize())
  }

  public static func hash(_ data: Bytes) -> Bytes {
    CryptoPrimitives.sha256(data)
  }
}

/// The only file that talks to CryptoKit / swift-crypto.
enum CryptoPrimitives {
  static let chaChaPolyTagLength = 16

  static func sha256(_ data: Bytes) -> Bytes {
    Bytes(SHA256.hash(data: data))
  }

  static func hmacSHA256(key: Bytes, data: Bytes) -> Bytes {
    Bytes(HMAC<SHA256>.authenticationCode(for: data, using: SymmetricKey(data: key)))
  }

  static func hkdfSHA256(ikm: Bytes, salt: Bytes, info: Bytes, length: Int = 32) -> Bytes {
    let key = HKDF<SHA256>.deriveKey(
      inputKeyMaterial: SymmetricKey(data: ikm),
      salt: salt,
      info: info,
      outputByteCount: length
    )
    return key.withUnsafeBytes { Bytes($0) }
  }

  static func x25519PublicKey(privateKey: Bytes) throws -> Bytes {
    let key = try Curve25519.KeyAgreement.PrivateKey(rawRepresentation: privateKey)
    return Bytes(key.publicKey.rawRepresentation)
  }

  static func x25519(privateKey: Bytes, publicKey: Bytes) throws -> Bytes {
    let key = try Curve25519.KeyAgreement.PrivateKey(rawRepresentation: privateKey)
    let secret = try key.sharedSecretFromKeyAgreement(
      with: Curve25519.KeyAgreement.PublicKey(rawRepresentation: publicKey)
    )
    let bytes = secret.withUnsafeBytes { Bytes($0) }
    guard bytes.contains(where: { $0 != 0 }) else { throw NoiseError("low-order public key") }
    return bytes
  }

  /// Returns ciphertext followed by the 16-byte tag.
  static func chaChaPolySeal(key: Bytes, nonce: Bytes, ad: Bytes, plaintext: Bytes) throws
    -> Bytes
  {
    let box = try ChaChaPoly.seal(
      plaintext,
      using: SymmetricKey(data: key),
      nonce: ChaChaPoly.Nonce(data: nonce),
      authenticating: ad
    )
    return Bytes(box.ciphertext) + Bytes(box.tag)
  }

  static func chaChaPolyOpen(key: Bytes, nonce: Bytes, ad: Bytes, ciphertext: Bytes) throws
    -> Bytes
  {
    guard ciphertext.count >= chaChaPolyTagLength else {
      throw NoiseError("ciphertext shorter than the tag")
    }
    let split = ciphertext.count - chaChaPolyTagLength
    let box = try ChaChaPoly.SealedBox(
      nonce: ChaChaPoly.Nonce(data: nonce),
      ciphertext: ciphertext[..<split],
      tag: ciphertext[split...]
    )
    return Bytes(try ChaChaPoly.open(box, using: SymmetricKey(data: key), authenticating: ad))
  }

  static func randomBytes(_ count: Int) -> Bytes {
    var generator = SystemRandomNumberGenerator()
    return (0..<count).map { _ in UInt8.random(in: .min ... .max, using: &generator) }
  }
}
