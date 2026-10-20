// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Connection setup and framing (PROTOCOL.md §4 and §6), without I/O.
///
/// Every message on the stream, handshake or transport, is a 2-byte
/// big-endian length followed by that many bytes.
public enum TransferProtocol {
  public static let version: UInt32 = 1
  public static let prologue = Bytes("brave-transfer/1".utf8)
  public static let maxWireMessage = 65535
  public static let maxFramePlaintext = maxWireMessage - Noise.tagLength
  public static let maxControlFrame = 16384
  public static let maxDeviceNameBytes = 128
  public static let maxGUIDBytes = 128

  public static func lengthPrefixed(_ message: Bytes) throws -> Bytes {
    guard message.count <= maxWireMessage else { throw ProtocolError("message too long") }
    return [UInt8(message.count >> 8), UInt8(message.count & 0xff)] + message
  }
}

public struct ProtocolError: Error, CustomStringConvertible {
  public let description: String
  public init(_ description: String) { self.description = description }
}

/// Splits a byte stream into length-prefixed messages.
public struct MessageReader {
  private var buffer = Bytes()

  public init() {}

  public mutating func feed(_ data: Bytes) -> [Bytes] {
    buffer += data
    var out: [Bytes] = []
    var start = 0
    while buffer.count - start >= 2 {
      let length = Int(buffer[start]) << 8 | Int(buffer[start + 1])
      guard buffer.count - start - 2 >= length else { break }
      out.append(Bytes(buffer[(start + 2)..<(start + 2 + length)]))
      start += 2 + length
    }
    buffer.removeFirst(start)
    return out
  }

  public var hasPartialMessage: Bool { !buffer.isEmpty }
}

func validatePeerInfo(_ info: PeerInfo) throws {
  guard info.protocolVersion == TransferProtocol.version else {
    throw ProtocolError("unsupported protocol version")
  }
  guard info.deviceName.utf8.count <= TransferProtocol.maxDeviceNameBytes else {
    throw ProtocolError("device name too long")
  }
  guard info.syncDeviceGUID.utf8.count <= TransferProtocol.maxGUIDBytes else {
    throw ProtocolError("guid too long")
  }
}

/// Transport phase: encrypts and decrypts Frames.
public final class SecureChannel {
  private let sendCipher: CipherState
  private let receiveCipher: CipherState
  public let handshakeHash: Bytes
  public let remoteStaticKey: Bytes
  public let remoteInfo: PeerInfo
  /// The initiator only knows the responder holds the PSK once a transport
  /// message from it decrypts (PROTOCOL.md §4.3). It must not send frames
  /// before that.
  public private(set) var isConfirmed: Bool

  init(handshake: NoiseHandshake, remoteInfo: PeerInfo) {
    sendCipher = handshake.sendCipher!
    receiveCipher = handshake.receiveCipher!
    handshakeHash = handshake.handshakeHash!
    remoteStaticKey = handshake.remoteStaticKey!
    self.remoteInfo = remoteInfo
    isConfirmed = !handshake.isInitiator
  }

  public var sendKey: Bytes { sendCipher.key! }
  public var receiveKey: Bytes { receiveCipher.key! }

  /// Returns the length-prefixed ciphertext, ready to write.
  public func seal(_ frame: Frame) throws -> Bytes {
    let plaintext = frame.serialized()
    let limit =
      frame.isChunk ? TransferProtocol.maxFramePlaintext : TransferProtocol.maxControlFrame
    guard plaintext.count <= limit else { throw ProtocolError("frame too large") }
    return try TransferProtocol.lengthPrefixed(
      sendCipher.encrypt(ad: [], plaintext: plaintext))
  }

  /// `message` is one transport message without its length prefix.
  public func open(_ message: Bytes) throws -> Frame {
    let plaintext: Bytes
    do {
      plaintext = try receiveCipher.decrypt(ad: [], ciphertext: message)
    } catch {
      throw ProtocolError("transport decryption failed")
    }
    isConfirmed = true
    let frame: Frame
    do {
      frame = try Frame(serialized: plaintext)
    } catch let error as DecodeError {
      throw ProtocolError("bad frame: \(error)")
    }
    if !frame.isChunk && plaintext.count > TransferProtocol.maxControlFrame {
      throw ProtocolError("control frame too large")
    }
    return frame
  }
}

/// Desktop side: dials after seeing a matching discovery tag.
public final class InitiatorHandshake {
  private let handshake: NoiseHandshake
  private let localInfo: PeerInfo
  private let trustTag: Bytes
  public private(set) var remoteInfo: PeerInfo?

  public init(
    staticPrivateKey: Bytes, psk: Bytes, trustTag: Bytes, localInfo: PeerInfo,
    ephemeralPrivateKey: Bytes? = nil
  ) throws {
    guard trustTag.count == ChainKeys.tagLength else { throw ProtocolError("bad tag") }
    handshake = try NoiseHandshake(
      initiator: true, prologue: TransferProtocol.prologue, staticPrivateKey: staticPrivateKey,
      psk: psk, ephemeralPrivateKey: ephemeralPrivateKey)
    self.localInfo = localInfo
    self.trustTag = trustTag
  }

  public func message1() throws -> Bytes {
    try handshake.writeMessage(
      payload: ClientHello(protocolVersion: TransferProtocol.version, trustTag: trustTag)
        .serialized())
  }

  @discardableResult
  public func readMessage2(_ message: Bytes) throws -> PeerInfo {
    let info: PeerInfo
    do {
      info = try PeerInfo(serialized: handshake.readMessage(message))
    } catch {
      throw ProtocolError("bad handshake message 2: \(error)")
    }
    try validatePeerInfo(info)
    remoteInfo = info
    return info
  }

  public func message3() throws -> Bytes {
    try handshake.writeMessage(payload: localInfo.serialized())
  }

  public func channel() throws -> SecureChannel {
    guard handshake.isComplete, let remoteInfo else {
      throw ProtocolError("handshake not complete")
    }
    return SecureChannel(handshake: handshake, remoteInfo: remoteInfo)
  }
}

/// iPhone side: listens and advertises tags. `pskForTag` returns the PSK
/// for a tag it accepts, or nil.
public final class ResponderHandshake {
  private let handshake: NoiseHandshake
  private let localInfo: PeerInfo
  private let pskForTag: (Bytes) -> Bytes?
  public private(set) var clientHello: ClientHello?
  public private(set) var remoteInfo: PeerInfo?

  public init(
    staticPrivateKey: Bytes, localInfo: PeerInfo, ephemeralPrivateKey: Bytes? = nil,
    pskForTag: @escaping (Bytes) -> Bytes?
  ) throws {
    handshake = try NoiseHandshake(
      initiator: false, prologue: TransferProtocol.prologue, staticPrivateKey: staticPrivateKey,
      ephemeralPrivateKey: ephemeralPrivateKey)
    self.localInfo = localInfo
    self.pskForTag = pskForTag
  }

  /// Accepts tags for the chain's current and adjacent epochs.
  public convenience init(
    staticPrivateKey: Bytes, localInfo: PeerInfo, keys: ChainKeys, unixTime: TimeInterval
  ) throws {
    let tags = keys.acceptedTags(unixTime: unixTime)
    try self.init(staticPrivateKey: staticPrivateKey, localInfo: localInfo) {
      tags.contains($0) ? keys.psk : nil
    }
  }

  @discardableResult
  public func readMessage1(_ message: Bytes) throws -> ClientHello {
    let hello: ClientHello
    do {
      hello = try ClientHello(serialized: handshake.readMessage(message))
    } catch {
      throw ProtocolError("bad handshake message 1: \(error)")
    }
    guard hello.protocolVersion == TransferProtocol.version else {
      throw ProtocolError("unsupported protocol version")
    }
    guard let psk = pskForTag(hello.trustTag) else { throw ProtocolError("unknown trust tag") }
    try handshake.setPSK(psk)
    clientHello = hello
    return hello
  }

  public func message2() throws -> Bytes {
    try handshake.writeMessage(payload: localInfo.serialized())
  }

  @discardableResult
  public func readMessage3(_ message: Bytes) throws -> PeerInfo {
    let info: PeerInfo
    do {
      info = try PeerInfo(serialized: handshake.readMessage(message))
    } catch {
      throw ProtocolError("bad handshake message 3: \(error)")
    }
    try validatePeerInfo(info)
    remoteInfo = info
    return info
  }

  public func channel() throws -> SecureChannel {
    guard handshake.isComplete, let remoteInfo else {
      throw ProtocolError("handshake not complete")
    }
    return SecureChannel(handshake: handshake, remoteInfo: remoteInfo)
  }
}
