// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveTransfer
import BraveTransferEngine
import BraveTransferUI
import Foundation
import UIKit
@_spi(ChromiumWebViewAccess) import Web

/// The Swift side of brave://transfer (design plan §5.5). The C++ page
/// handler forwards every `PageHandler` call here, and this pushes engine
/// events to the page. One per Chromium tab, like `WalletWebUIHelper`; the
/// engine is the app-wide one in `TransferAppCoordinator`.
///
/// The bridge calls arrive on the main thread and completions run there.
final class TransferWebUIHelper: NSObject, TabObserver, TransferPageHandlerBridge,
  TransferEventListener
{
  private(set) weak var tab: (any TabState)?
  private let coordinator: TransferAppCoordinator
  private let engine: TransferEngine
  private var pages: [any BraveTransferPage] = []

  @MainActor
  init?(tab: some TabState, coordinator: TransferAppCoordinator?) {
    guard tab.isChromiumTab, !tab.isPrivate, let coordinator else { return nil }
    self.tab = tab
    self.coordinator = coordinator
    engine = coordinator.engine
    super.init()
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.transferPageHandler = self
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: Pages

  func pageDidConnect(_ page: any BraveTransferPage) {
    let isFirst = pages.isEmpty
    pages.append(page)
    MainActor.assumeIsolated {
      if isFirst {
        coordinator.addListener(self)
      }
      coordinator.markSetupDone()
    }
  }

  func pageDidDisconnect(_ page: any BraveTransferPage) {
    pages.removeAll { $0 === page }
    if pages.isEmpty {
      MainActor.assumeIsolated {
        coordinator.removeListener(self)
      }
    }
  }

  func transferEvent(_ event: TransferAppEvent) {
    for page in pages {
      switch event {
      case .serviceState(let state):
        page.onServiceStateChanged(state: .init(state))
      case .devices(let devices):
        page.onDevicesChanged(devices: devices.map(BraveTransfer.Device.init))
      case .offerReceived(let offer):
        page.onOfferReceived(offer: .init(offer))
      case .offerRemoved(let id):
        page.onOfferRemoved(offerId: id)
      case .transferUpdated(let transfer):
        page.onTransferUpdated(transfer: .init(transfer))
      case .inboxChanged:
        page.onInboxChanged()
      case .settings(let settings):
        page.onSettingsChanged(settings: .init(settings))
      case .permissions(let permissions):
        page.onPermissionsChanged(permissions: .init(permissions))
      }
    }
  }

  /// Runs `body` on the engine queue and `completion` with its result on the
  /// main thread.
  private func query<T>(
    _ body: @escaping (TransferService) -> T,
    _ completion: @escaping (T) -> Void
  ) {
    engine.perform { service in
      let value = body(service)
      DispatchQueue.main.async { completion(value) }
    }
  }

  private func inboxEntry(_ id: String, _ completion: @escaping (InboxEntry?) -> Void) {
    query({ $0.inboxEntry(id) }, completion)
  }

  // MARK: BraveTransferPageHandler

  func serviceState(completion: @escaping (BraveTransfer.ServiceState) -> Void) {
    query({ BraveTransfer.ServiceState($0.serviceState) }, completion)
  }

  func openSyncSettings() {
    MainActor.assumeIsolated {
      coordinator.router?.openSyncSettings()
    }
  }

  func devices(completion: @escaping ([BraveTransfer.Device]) -> Void) {
    query({ $0.devices.map(BraveTransfer.Device.init) }, completion)
  }

  func setDevicePolicy(
    deviceId: String,
    policy: BraveTransfer.DevicePolicy,
    completion: @escaping (Bool) -> Void
  ) {
    query({ $0.setDevicePolicy(DevicePolicy(policy), for: deviceId) }, completion)
  }

  func removeDevice(deviceId: String, completion: @escaping (Bool) -> Void) {
    query({ $0.removeDevice(deviceId) }, completion)
  }

  func sendText(
    deviceId: String,
    text: String,
    completion: @escaping (BraveTransfer.SendResult) -> Void
  ) {
    query({ BraveTransfer.SendResult($0.sendText(text, to: deviceId)) }, completion)
  }

  /// Desktop only: on iOS the page sends files through `pickAndSend`.
  func sendFiles(
    deviceId: String,
    files: [BraveTransfer.FileData],
    completion: @escaping (BraveTransfer.SendResult) -> Void
  ) {
    completion(.notSupported)
  }

  func pickAndSend(
    deviceId: String,
    completion: @escaping (BraveTransfer.SendResult) -> Void
  ) {
    MainActor.assumeIsolated {
      guard let presenter = coordinator.router?.transferPresentingViewController else {
        completion(.failed)
        return
      }
      var top = presenter
      while let presented = top.presentedViewController {
        top = presented
      }
      let directory = TransferAppCoordinator.pickedItemsDirectory
        .appendingPathComponent(UUID().uuidString, isDirectory: true)
      let engine = self.engine
      let coordinator = self.coordinator
      TransferItemPicker.present(from: top, into: directory) { items in
        guard let items, !items.isEmpty else {
          try? FileManager.default.removeItem(at: directory)
          completion(.cancelled)
          return
        }
        DispatchQueue.global(qos: .userInitiated).async {
          let outgoing = items.compactMap { try? $0.outgoingItem() }
          engine.perform { service in
            let result = outgoing.isEmpty ? .failed : service.send(outgoing, to: deviceId)
            let transferID = result == .success ? service.lastTransferID : nil
            DispatchQueue.main.async {
              MainActor.assumeIsolated {
                if let transferID {
                  coordinator.removeWhenFinished(directory, transferID: transferID)
                } else {
                  try? FileManager.default.removeItem(at: directory)
                }
                completion(BraveTransfer.SendResult(result))
              }
            }
          }
        }
      }
    }
  }

  func cancelTransfer(transferId: String) {
    engine.perform { $0.cancelTransfer(transferId) }
  }

  func transfers(completion: @escaping ([BraveTransfer.Transfer]) -> Void) {
    query({ $0.transferList.map(BraveTransfer.Transfer.init) }, completion)
  }

  func pendingOffers(completion: @escaping ([BraveTransfer.IncomingOffer]) -> Void) {
    query({ $0.pendingOfferList.map(BraveTransfer.IncomingOffer.init) }, completion)
  }

  func respondToOffer(offerId: String, accept: Bool, completion: @escaping (Bool) -> Void) {
    query({ $0.respondToOffer(offerId, accept: accept) }, completion)
  }

  func inbox(
    offset: UInt32,
    count: UInt32,
    completion: @escaping ([BraveTransfer.InboxItem], Bool) -> Void
  ) {
    query(
      { $0.inboxPage(offset: Int(offset), count: Int(count)) },
      { page in completion(page.entries.map(BraveTransfer.InboxItem.init), page.hasMore) }
    )
  }

  /// Files open in the Files app; items saved to Photos open Photos (with
  /// add-only access the asset itself cannot be linked, design plan §5.2).
  func openItem(itemId: String, completion: @escaping (Bool) -> Void) {
    inboxEntry(itemId) { entry in
      guard let entry else {
        completion(false)
        return
      }
      if entry.savedToPhotos, let photos = URL(string: "photos-redirect://") {
        UIApplication.shared.open(photos) { completion($0) }
        return
      }
      guard let path = entry.filePath, !entry.isFileMissing,
        let url = Self.filesAppURL(forPath: path)
      else {
        completion(false)
        return
      }
      UIApplication.shared.open(url) { completion($0) }
    }
  }

  func showItemInFolder(itemId: String, completion: @escaping (Bool) -> Void) {
    inboxEntry(itemId) { entry in
      guard let path = entry?.filePath,
        let url = Self.filesAppURL(forPath: (path as NSString).deletingLastPathComponent)
      else {
        completion(false)
        return
      }
      UIApplication.shared.open(url) { completion($0) }
    }
  }

  func openLink(itemId: String, completion: @escaping (Bool) -> Void) {
    inboxEntry(itemId) { [weak self] entry in
      MainActor.assumeIsolated {
        guard let entry, entry.kind == .link,
          let url = URL(string: entry.text),
          let scheme = url.scheme?.lowercased(), scheme == "http" || scheme == "https",
          let router = self?.coordinator.router
        else {
          completion(false)
          return
        }
        router.openTransferLink(url)
        completion(true)
      }
    }
  }

  func clearHistory(completion: @escaping (Bool) -> Void) {
    query({ $0.clearHistory() }, completion)
  }

  func settings(completion: @escaping (BraveTransfer.Settings) -> Void) {
    query({ BraveTransfer.Settings($0.settings) }, completion)
  }

  func updateSettings(_ settings: BraveTransfer.Settings, completion: @escaping (Bool) -> Void) {
    let newSettings = settings.engineSettings
    query(
      { service in
        service.settings = newSettings
        return true
      },
      { success in
        TransferAppGroup.postDidChange()
        completion(success)
      }
    )
  }

  /// Desktop only: on iOS files always go to Files › Brave › Downloads.
  func chooseSaveDirectory(completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func permissions(completion: @escaping (BraveTransfer.Permissions) -> Void) {
    MainActor.assumeIsolated {
      completion(.init(coordinator.permissions))
    }
  }

  func requestPermissions(completion: @escaping (BraveTransfer.Permissions) -> Void) {
    MainActor.assumeIsolated {
      coordinator.requestPermissions { completion(.init($0)) }
    }
  }

  func openSystemSettings() {
    guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
    UIApplication.shared.open(url)
  }

  /// `shareddocuments://<path>` opens the Files app at a path in the app's
  /// container, as `openBraveDownloadsFolder` does.
  private static func filesAppURL(forPath path: String) -> URL? {
    var components = URLComponents(url: URL(fileURLWithPath: path), resolvingAgainstBaseURL: false)
    components?.scheme = "shareddocuments"
    return components?.url
  }
}

extension TabDataValues {
  private struct TransferWebUIHelperKey: TabDataKey {
    static var defaultValue: TransferWebUIHelper? = nil
  }

  var transferWebUIHelper: TransferWebUIHelper? {
    get { self[TransferWebUIHelperKey.self] }
    set { self[TransferWebUIHelperKey.self] = newValue }
  }
}
