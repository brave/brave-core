// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

public enum AdvertiserState: Equatable, Sendable {
  case stopped
  case starting
  /// Registered with Bonjour and accepting connections.
  case advertising
  /// Network.framework is waiting. `localNetworkDenied` is set when the
  /// error is DNS-SD PolicyDenied, which is how a Local Network denial
  /// shows up.
  case waiting(localNetworkDenied: Bool)
  case failed(String)
}

/// Accepts connections and advertises the DNS-SD service `_brave-xfer._tcp`
/// with a TXT record (design plan §3.4). On iOS this is an `NWListener`.
public protocol TransferAdvertiser: AnyObject {
  var state: AdvertiserState { get }
  var onStateChange: ((AdvertiserState) -> Void)? { get set }
  /// A new inbound connection, not yet started.
  var onConnection: ((ByteStream) -> Void)? { get set }
  func start(txtRecord: [String: String])
  func updateTXTRecord(_ txtRecord: [String: String])
  func stop()
}

public enum DiscoveryConstants {
  /// The DNS-SD service type, without the domain.
  public static let serviceType = "_brave-xfer._tcp"
  /// A random instance name per listening session; no device name is ever
  /// advertised in plaintext.
  public static func randomInstanceName() -> String {
    "brave-" + Bytes((0..<6).map { _ in UInt8.random(in: 0...255) }).hexString
  }
}

public enum TagSchedule {
  /// Seconds until the discovery tag changes. A small margin past the
  /// boundary keeps the new tag from being computed a moment too early.
  public static func secondsUntilNextEpoch(unixTime: TimeInterval) -> TimeInterval {
    let epoch = TimeInterval(ChainKeys.epochSeconds)
    let elapsed = unixTime.truncatingRemainder(dividingBy: epoch)
    return epoch - elapsed + 1
  }
}

/// A hint that a peer is physically near (design plan §6). Only used to rank
/// or highlight devices, never for trust or auto-accept.
public struct ProximityHint: Equatable, Sendable {
  public var peerHint: String
  public var rssi: Int
  /// Seconds since the Unix epoch.
  public var timestamp: TimeInterval

  public init(peerHint: String, rssi: Int, timestamp: TimeInterval) {
    self.peerHint = peerHint
    self.rssi = rssi
    self.timestamp = timestamp
  }
}

public protocol ProximityProvider: AnyObject {
  var onProximity: ((ProximityHint) -> Void)? { get set }
  func start()
  func stop()
}
