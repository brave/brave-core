// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Keys every device in one Brave Sync chain can derive (PROTOCOL.md §3).
///
/// `seed` is the 32-byte output of brave_sync::crypto::PassphraseToBytes32
/// for the chain's 24-word sync code. Converting the code is left to the
/// embedder (BraveCore on iOS), so this package needs no BIP39 word list.
public struct ChainKeys: Equatable {
  public static let epochSeconds: UInt64 = 900
  public static let tagLength = 8

  public let rootKey: Bytes
  public let psk: Bytes
  public let discoveryKey: Bytes

  public init(seed: Bytes) throws {
    guard seed.count == 32 else { throw ProtocolError("seed must be 32 bytes") }
    try self.init(
      rootKey: CryptoPrimitives.hkdfSHA256(
        ikm: seed, salt: Bytes("brave-transfer".utf8), info: Bytes("v1/root".utf8)))
  }

  public init(rootKey: Bytes) throws {
    guard rootKey.count == 32 else { throw ProtocolError("root key must be 32 bytes") }
    self.rootKey = rootKey
    psk = CryptoPrimitives.hkdfSHA256(ikm: rootKey, salt: [], info: Bytes("v1/chain-psk".utf8))
    discoveryKey = CryptoPrimitives.hkdfSHA256(
      ikm: rootKey, salt: [], info: Bytes("v1/discovery".utf8))
  }

  public static func epoch(unixTime: TimeInterval) -> UInt64 {
    unixTime <= 0 ? 0 : UInt64(unixTime) / epochSeconds
  }

  public func tag(epoch: UInt64) -> Bytes {
    let message = (0..<8).map { UInt8(truncatingIfNeeded: epoch >> (8 * UInt64(7 - $0))) }
    return Array(
      CryptoPrimitives.hmacSHA256(key: discoveryKey, data: message).prefix(Self.tagLength))
  }

  /// Tags for epochs t-1, t and t+1, which a responder accepts.
  public func acceptedTags(unixTime: TimeInterval) -> [Bytes] {
    let epoch = Self.epoch(unixTime: unixTime)
    return (epoch == 0 ? [0, 1] : [epoch - 1, epoch, epoch + 1]).map { tag(epoch: $0) }
  }

  /// DNS-SD TXT record entries for the current epoch.
  public func txtRecord(unixTime: TimeInterval) -> [String: String] {
    ["v": "1", "c": tag(epoch: Self.epoch(unixTime: unixTime)).hexString]
  }

  public func txtRecordMatches(_ record: [String: String], unixTime: TimeInterval) -> Bool {
    guard record["v"] == "1", let c = record["c"], let tag = Bytes(hexString: c) else {
      return false
    }
    return acceptedTags(unixTime: unixTime).contains(tag)
  }
}

public enum StaticKey {
  public static func generatePrivateKey() -> Bytes {
    CryptoPrimitives.randomBytes(32)
  }

  public static func publicKey(privateKey: Bytes) throws -> Bytes {
    try CryptoPrimitives.x25519PublicKey(privateKey: privateKey)
  }

  /// First 16 bytes of SHA-256(public key) as hex in groups of four, for
  /// showing to users.
  public static func fingerprint(publicKey: Bytes) -> String {
    let hex = Array(CryptoPrimitives.sha256(publicKey).prefix(16)).hexString
    return stride(from: 0, to: hex.count, by: 4).map { start -> String in
      let from = hex.index(hex.startIndex, offsetBy: start)
      return String(hex[from..<hex.index(from, offsetBy: 4)])
    }.joined(separator: " ")
  }
}
