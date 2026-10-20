// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import BraveTransferEngine
import Foundation
import UIKit

/// Brave Transfer for one process: a `TransferService` on its own serial
/// queue, with Bonjour, the app-group keychain and stores, and (in the app)
/// Photos and Files saving plugged in. Used by the app and by the share
/// extension, so it does not depend on BraveCore.
///
/// `service` is confined to `queue`; use `perform` from other threads.
/// Observers added with `addObserver` are called on `queue`.
public final class TransferEngine {
  public let role: TransferServiceConfiguration.Role
  public let queue: DispatchQueue
  public let appGroup: TransferAppGroup
  public let keyStore: TransferKeyStore
  public let localNetwork: LocalNetworkAuthorization
  public let service: TransferService

  private let trustSource: SnapshotTrustSource
  private let advertiser: BonjourAdvertiser
  private var changeObserver: UnsafeMutableRawPointer?

  public enum Error: Swift.Error {
    case staticKeyUnavailable
  }

  /// - Parameters:
  ///   - downloadsDirectory: where received files go (app only).
  ///   - inboxURL: the inbox file (app only).
  @MainActor
  public init(
    role: TransferServiceConfiguration.Role,
    appGroup: TransferAppGroup,
    downloadsDirectory: (() throws -> URL)? = nil,
    inboxURL: URL? = nil
  ) throws {
    self.role = role
    self.appGroup = appGroup
    queue = DispatchQueue(label: "com.brave.transfer.engine", qos: .userInitiated)
    keyStore = TransferKeyStore(keychain: TransferKeychain(accessGroup: appGroup.identifier))
    localNetwork = LocalNetworkAuthorization(defaults: appGroup.defaults)
    guard let staticPrivateKey = keyStore.staticPrivateKey() else {
      throw Error.staticKeyUnavailable
    }
    let scheduler = DispatchQueueScheduler(queue: queue)
    trustSource = SnapshotTrustSource(
      snapshot: ChainSnapshot.read(from: appGroup.chainSnapshotURL) ?? .inactive,
      rootKey: keyStore.rootKey
    )
    advertiser = BonjourAdvertiser(queue: queue)

    var fileSink: IncomingFileSink?
    var inbox: InboxStore?
    if role == .app, let downloadsDirectory, let inboxURL {
      fileSink = TemporaryFileSink(
        temporaryDirectory: FileManager.default.temporaryDirectory
          .appendingPathComponent("BraveTransfer", isDirectory: true),
        finalizer: ReceivedItemFinalizer(downloadsDirectory: downloadsDirectory),
        scheduler: scheduler
      )
      inbox = FileInboxStore(url: inboxURL)
    }

    let idiom = UIDevice.current.userInterfaceIdiom
    var configuration = TransferServiceConfiguration(
      role: role,
      localDeviceType: idiom == .pad ? .tablet : .phone,
      fallbackDeviceName: UIDevice.current.name
    )
    if role == .shareExtension {
      configuration.maxSendBytes = HandoffPolicy().maxDirectBytes
    }
    let settingsStore: TransferSettingsStore =
      appGroup.defaults.map { UserDefaultsSettingsStore(defaults: $0) } ?? InMemorySettingsStore()
    service = TransferService(
      configuration: configuration,
      scheduler: scheduler,
      staticPrivateKey: staticPrivateKey,
      trustSource: trustSource,
      trustStore: FileTrustStore(url: appGroup.trustStoreURL),
      settingsStore: settingsStore,
      advertiser: advertiser,
      fileSink: fileSink,
      inbox: inbox
    )
    let serviceHandler = advertiser.onStateChange
    advertiser.onStateChange = { [weak self] state in
      self?.localNetwork.record(state)
      serviceHandler?(state)
    }
    observeOtherProcess()
  }

  deinit {
    CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDarwinNotifyCenter(),
      Unmanaged.passUnretained(self).toOpaque()
    )
  }

  /// Runs `body` on the engine queue.
  public func perform(_ body: @escaping (TransferService) -> Void) {
    queue.async { [service] in body(service) }
  }

  public func addObserver(_ observer: TransferServiceObserver) {
    perform { $0.addObserver(observer) }
  }

  public func removeObserver(_ observer: TransferServiceObserver) {
    perform { $0.removeObserver(observer) }
  }

  /// Re-reads the chain snapshot, root key and settings. Called when the
  /// other process posts `TransferAppGroup.didChangeNotification`, and by the
  /// app after exporting a new snapshot.
  public func reload() {
    perform { [self] service in
      trustSource.update(
        snapshot: ChainSnapshot.read(from: appGroup.chainSnapshotURL) ?? .inactive,
        rootKey: keyStore.rootKey
      )
      service.settingsChanged()
    }
  }

  /// Restarts advertising, for example after returning to the foreground
  /// when the listener failed while suspended.
  public func restartListening() {
    perform { service in
      guard service.isListeningAllowed else { return }
      service.setListeningAllowed(false)
      service.setListeningAllowed(true)
    }
  }

  private func observeOtherProcess() {
    CFNotificationCenterAddObserver(
      CFNotificationCenterGetDarwinNotifyCenter(),
      Unmanaged.passUnretained(self).toOpaque(),
      { _, observer, _, _, _ in
        guard let observer else { return }
        Unmanaged<TransferEngine>.fromOpaque(observer).takeUnretainedValue().reload()
      },
      TransferAppGroup.didChangeNotification as CFString,
      nil,
      .deliverImmediately
    )
  }
}
