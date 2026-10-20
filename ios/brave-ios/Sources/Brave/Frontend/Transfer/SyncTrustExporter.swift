// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveTransfer
import BraveTransferCore
import BraveTransferEngine
import Foundation
import os

/// Writes the Sync chain's device list and root key where the transfer
/// engine (in the app and in the share extension) reads them (design plan
/// §5.4). Only the app writes them, from the Sync observers. The root key
/// comes from `BraveSyncAPI.transferRootKey`, which is nil unless Sync is
/// active and never creates a sync code.
@MainActor
final class SyncTrustExporter {
  private let syncAPI: BraveSyncAPI
  private let appGroup: TransferAppGroup
  private let keyStore: TransferKeyStore
  private let onChange: () -> Void
  private var serviceObserver: AnyObject?
  private var deviceObserver: AnyObject?

  private static let log = Logger(subsystem: "com.brave.transfer", category: "sync")

  init(
    syncAPI: BraveSyncAPI,
    appGroup: TransferAppGroup,
    keyStore: TransferKeyStore,
    onChange: @escaping () -> Void
  ) {
    self.syncAPI = syncAPI
    self.appGroup = appGroup
    self.keyStore = keyStore
    self.onChange = onChange
    serviceObserver = syncAPI.addServiceStateObserver(
      { [weak self] in self?.export() },
      onServiceShutdown: { [weak self] in self?.clear() }
    )
    deviceObserver = syncAPI.addDeviceStateObserver { [weak self] in
      self?.export()
    }
    export()
  }

  func export() {
    let isActive = syncAPI.isSyncFeatureActive
    let rootKey = isActive ? syncAPI.transferRootKey.map { Bytes($0) } : nil
    var snapshot = ChainSnapshot.inactive
    if let rootKey, rootKey.count == 32, let json = syncAPI.getDeviceListJSON() {
      snapshot =
        ChainSnapshot.fromSyncDeviceList(json: Data(json.utf8), isSyncActive: true) ?? .inactive
    }
    write(snapshot: snapshot, rootKey: snapshot.isSyncActive ? rootKey : nil)
  }

  private func clear() {
    write(snapshot: .inactive, rootKey: nil)
  }

  private func write(snapshot: ChainSnapshot, rootKey: Bytes?) {
    let url = appGroup.chainSnapshotURL
    let snapshotChanged = ChainSnapshot.read(from: url) != snapshot
    if snapshotChanged {
      do {
        try snapshot.write(to: url)
      } catch {
        Self.log.error("Could not write the chain snapshot: \(error.localizedDescription)")
      }
    }
    let keyChanged = keyStore.setRootKey(rootKey)
    if snapshotChanged || keyChanged {
      onChange()
      TransferAppGroup.postDidChange()
    }
  }
}
