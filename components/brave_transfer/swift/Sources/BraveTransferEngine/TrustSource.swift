// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

/// A device in this device's Brave Sync chain.
public struct ChainDevice: Codable, Equatable, Sendable {
  public var guid: String
  public var name: String
  public var type: TransferDeviceType

  public init(guid: String, name: String, type: TransferDeviceType) {
    self.guid = guid
    self.name = name
    self.type = type
  }
}

/// Where Sync-chain trust comes from (design plan §3.3).
public protocol TransferTrustSource: AnyObject {
  /// Whether Sync is active. Chain trust only applies while it is.
  var isSyncActive: Bool { get }
  /// Keys derived from the chain's sync code, or nil while Sync is not active.
  var chainKeys: ChainKeys? { get }
  /// This device's sync device GUID and name, empty while Sync is not active.
  var localDeviceGUID: String { get }
  var localDeviceName: String { get }
  /// The other devices in the chain.
  var chainDevices: [ChainDevice] { get }
  /// Called on the engine's scheduler when any of the above changes.
  var onChange: (() -> Void)? { get set }
}

/// The Sync state the main app mirrors into the app group (`devices.json`),
/// so that the share extension, which cannot load BraveCore, sees the same
/// chain as the app. `K_root` is not part of it; it lives in the shared
/// keychain and is deleted whenever Sync is not active.
public struct ChainSnapshot: Codable, Equatable, Sendable {
  public var isSyncActive: Bool
  public var localDeviceGUID: String
  public var localDeviceName: String
  public var devices: [ChainDevice]

  public init(
    isSyncActive: Bool, localDeviceGUID: String, localDeviceName: String,
    devices: [ChainDevice]
  ) {
    self.isSyncActive = isSyncActive
    self.localDeviceGUID = localDeviceGUID
    self.localDeviceName = localDeviceName
    self.devices = devices
  }

  public static let inactive = ChainSnapshot(
    isSyncActive: false, localDeviceGUID: "", localDeviceName: "", devices: [])

  /// Builds a snapshot from `BraveSyncAPI.getDeviceListJSON()`: an array of
  /// `DeviceInfo::ToValue()` dictionaries plus `guid` and `isCurrentDevice`.
  /// Returns nil when the JSON cannot be parsed.
  public static func fromSyncDeviceList(json: Data, isSyncActive: Bool) -> ChainSnapshot? {
    guard let list = try? JSONSerialization.jsonObject(with: json) as? [[String: Any]] else {
      return nil
    }
    var snapshot = ChainSnapshot(
      isSyncActive: isSyncActive, localDeviceGUID: "", localDeviceName: "", devices: [])
    for entry in list {
      guard let guid = entry["guid"] as? String, !guid.isEmpty else { continue }
      let name = entry["name"] as? String ?? ""
      if entry["isCurrentDevice"] as? Bool == true {
        snapshot.localDeviceGUID = guid
        snapshot.localDeviceName = name
        continue
      }
      snapshot.devices.append(
        ChainDevice(guid: guid, name: name, type: deviceType(entry["type"] as? String)))
    }
    return snapshot
  }

  /// Maps `DeviceInfo::GetDeviceTypeString()`.
  static func deviceType(_ value: String?) -> TransferDeviceType {
    switch value {
    case "desktop_or_laptop": return .desktop
    case "phone": return .phone
    case "tablet": return .tablet
    default: return .unknown
    }
  }

  public func write(to url: URL) throws {
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys]
    try FileManager.default.createDirectory(
      at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
    try encoder.encode(self).write(to: url, options: .atomic)
  }

  public static func read(from url: URL) -> ChainSnapshot? {
    guard let data = try? Data(contentsOf: url) else { return nil }
    return try? JSONDecoder().decode(ChainSnapshot.self, from: data)
  }
}

/// A trust source backed by a `ChainSnapshot` and a root key provider. The app
/// updates both from BraveSyncAPI and calls `update`; the share extension
/// loads them once from the app group and keychain.
public final class SnapshotTrustSource: TransferTrustSource {
  private var snapshot: ChainSnapshot
  private var keys: ChainKeys?
  public var onChange: (() -> Void)?

  public init(snapshot: ChainSnapshot, rootKey: Bytes?) {
    self.snapshot = snapshot
    keys = rootKey.flatMap { try? ChainKeys(rootKey: $0) }
  }

  public func update(snapshot: ChainSnapshot, rootKey: Bytes?) {
    let newKeys = rootKey.flatMap { try? ChainKeys(rootKey: $0) }
    guard snapshot != self.snapshot || newKeys != keys else { return }
    self.snapshot = snapshot
    keys = newKeys
    onChange?()
  }

  /// Sync counts as active only when the snapshot says so and the root key is
  /// present, so a stale snapshot without a key never enables chain trust.
  public var isSyncActive: Bool { snapshot.isSyncActive && keys != nil }
  public var chainKeys: ChainKeys? { isSyncActive ? keys : nil }
  public var localDeviceGUID: String { isSyncActive ? snapshot.localDeviceGUID : "" }
  public var localDeviceName: String { isSyncActive ? snapshot.localDeviceName : "" }
  public var chainDevices: [ChainDevice] { isSyncActive ? snapshot.devices : [] }
}
