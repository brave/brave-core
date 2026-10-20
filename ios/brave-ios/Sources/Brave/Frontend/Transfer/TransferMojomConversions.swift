// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveTransferEngine
import Foundation

// Converts between BraveTransferEngine's Swift models and the ObjC mojom
// types generated from brave_transfer.mojom. The enums have the same cases
// in the same order on both sides.

extension BraveTransfer.ServiceState {
  convenience init(_ state: TransferServiceState) {
    self.init(syncEnabled: state.syncEnabled, discoveryActive: state.discoveryActive)
  }
}

extension BraveTransfer.Device {
  convenience init(_ device: TransferDevice) {
    self.init(
      id: device.id,
      name: device.name,
      type: BraveTransfer.DeviceType(rawValue: device.type.rawValue) ?? .unknown,
      source: BraveTransfer.TrustSource(rawValue: device.source.rawValue) ?? .syncChain,
      isConnected: device.isConnected,
      lastSeenTime: device.lastSeenTime,
      keyFingerprint: device.keyFingerprint,
      policy: BraveTransfer.DevicePolicy(rawValue: device.policy.rawValue) ?? .default
    )
  }
}

extension BraveTransfer.TransferItem {
  convenience init(_ item: TransferItemInfo) {
    self.init(
      id: item.id,
      kind: BraveTransfer.ItemKind(rawValue: item.kind.rawValue) ?? .file,
      name: item.name,
      mimeType: item.mimeType,
      size: item.size,
      text: item.text
    )
  }
}

extension BraveTransfer.IncomingOffer {
  convenience init(_ offer: IncomingOfferInfo) {
    self.init(
      id: offer.id,
      deviceId: offer.deviceID,
      deviceName: offer.deviceName,
      items: offer.items.map(BraveTransfer.TransferItem.init),
      expiryTime: offer.expiryTime
    )
  }
}

extension BraveTransfer.Transfer {
  convenience init(_ transfer: TransferInfo) {
    self.init(
      id: transfer.id,
      deviceId: transfer.deviceID,
      deviceName: transfer.deviceName,
      direction: BraveTransfer.TransferDirection(rawValue: transfer.direction.rawValue) ?? .send,
      state: BraveTransfer.TransferState(rawValue: transfer.state.rawValue) ?? .failed,
      items: transfer.items.map(BraveTransfer.TransferItem.init),
      bytesTransferred: transfer.bytesTransferred,
      bytesTotal: transfer.bytesTotal
    )
  }
}

extension BraveTransfer.InboxItem {
  convenience init(_ entry: InboxEntry) {
    self.init(
      id: String(entry.id),
      receivedTime: entry.receivedTime,
      deviceName: entry.deviceName,
      kind: BraveTransfer.ItemKind(rawValue: entry.kind.rawValue) ?? .file,
      text: entry.text,
      fileName: entry.fileName,
      mimeType: entry.mimeType,
      size: entry.size,
      thumbnailUrl: entry.thumbnailURL,
      fileMissing: entry.isFileMissing,
      savedToFallback: entry.savedToFallback
    )
  }
}

extension BraveTransfer.Settings {
  /// iOS has no save-location setting (design plan §5.2); the page shows the
  /// location read-only.
  static let saveLocation = "Files › Brave › Downloads"

  convenience init(_ settings: TransferSettings) {
    self.init(
      receiveMode: BraveTransfer.ReceiveMode(rawValue: settings.receiveMode.rawValue)
        ?? .trustedDevices,
      autoAcceptTrusted: settings.autoAcceptTrusted,
      autoTrustSyncChain: settings.autoTrustSyncChain,
      notifyOnReceive: settings.notifyOnReceive,
      saveDirectory: Self.saveLocation,
      defaultSaveDirectory: Self.saveLocation
    )
  }

  var engineSettings: TransferSettings {
    var settings = TransferSettings()
    settings.receiveMode = ReceiveMode(rawValue: receiveMode.rawValue) ?? .trustedDevices
    settings.autoAcceptTrusted = autoAcceptTrusted
    settings.autoTrustSyncChain = autoTrustSyncChain
    settings.notifyOnReceive = notifyOnReceive
    return settings
  }
}

extension BraveTransfer.Permissions {
  convenience init(_ permissions: TransferPermissions) {
    self.init(
      localNetwork: BraveTransfer.PermissionStatus(rawValue: permissions.localNetwork.rawValue)
        ?? .notDetermined,
      photos: BraveTransfer.PermissionStatus(rawValue: permissions.photos.rawValue)
        ?? .notDetermined
    )
  }
}

extension BraveTransfer.SendResult {
  init(_ result: SendResult) {
    self = BraveTransfer.SendResult(rawValue: result.rawValue) ?? .failed
  }
}

extension DevicePolicy {
  init(_ policy: BraveTransfer.DevicePolicy) {
    self = DevicePolicy(rawValue: policy.rawValue) ?? .default
  }
}
