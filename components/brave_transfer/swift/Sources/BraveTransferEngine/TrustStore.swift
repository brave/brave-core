// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

/// What this device remembers about a peer, keyed by sync device GUID. The
/// same fields as the desktop's `brave_transfer.trusted_devices` pref.
public struct TrustedDeviceRecord: Codable, Equatable, Sendable {
  /// The peer's static public key, pinned on first successful contact.
  public var staticKey: Bytes?
  public var name: String
  public var policy: DevicePolicy
  /// Milliseconds since the Unix epoch.
  public var lastSeen: Double
  /// Set by "Remove": the device is rejected until it leaves and rejoins the
  /// chain (which drops the record).
  public var removed: Bool

  public init(
    staticKey: Bytes? = nil, name: String = "", policy: DevicePolicy = .default,
    lastSeen: Double = 0, removed: Bool = false
  ) {
    self.staticKey = staticKey
    self.name = name
    self.policy = policy
    self.lastSeen = lastSeen
    self.removed = removed
  }
}

public protocol TrustStore: AnyObject {
  var records: [String: TrustedDeviceRecord] { get }
  func record(for guid: String) -> TrustedDeviceRecord?
  func update(_ guid: String, _ change: (inout TrustedDeviceRecord) -> Void)
  func remove(_ guids: [String])
}

/// Pins kept in memory and, if `url` is set, in a JSON file (on iOS,
/// `transfer/trust.json` in the app group, so the app and the extension share
/// pins). Writes are atomic. The file holds a handful of devices and is
/// re-read on every access, so pins made by one process are seen by the
/// other.
public final class FileTrustStore: TrustStore {
  private let url: URL?
  private var cache: [String: TrustedDeviceRecord] = [:]

  public init(url: URL?) {
    self.url = url
    reload()
  }

  public var records: [String: TrustedDeviceRecord] {
    reload()
    return cache
  }

  public func record(for guid: String) -> TrustedDeviceRecord? {
    records[guid]
  }

  public func update(_ guid: String, _ change: (inout TrustedDeviceRecord) -> Void) {
    reload()
    var record = cache[guid] ?? TrustedDeviceRecord()
    change(&record)
    cache[guid] = record
    save()
  }

  public func remove(_ guids: [String]) {
    reload()
    var changed = false
    for guid in guids where cache.removeValue(forKey: guid) != nil {
      changed = true
    }
    if changed {
      save()
    }
  }

  private func reload() {
    guard let url else { return }
    guard let data = try? Data(contentsOf: url),
      let decoded = try? JSONDecoder().decode([String: TrustedDeviceRecord].self, from: data)
    else {
      cache = [:]
      return
    }
    cache = decoded
  }

  private func save() {
    guard let url else { return }
    do {
      try FileManager.default.createDirectory(
        at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
      let encoder = JSONEncoder()
      encoder.outputFormatting = [.sortedKeys]
      try encoder.encode(cache).write(to: url, options: .atomic)
    } catch {
      // Pins stay in memory for this session; the next save retries.
    }
  }
}
