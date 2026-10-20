// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveTransfer
import BraveTransferEngine
import BraveTransferUI
import Foundation
import Strings
import SwiftUI
import UIKit
import os

/// What the coordinator needs from a browser window.
@MainActor
protocol TransferAppRouting: AnyObject {
  /// Focuses an open brave://transfer tab or opens one (design plan §8,
  /// so the page is not opened in several tabs).
  func openTransferPage()
  func openTransferLink(_ url: URL)
  func openSyncSettings()
  var transferPresentingViewController: UIViewController { get }
}

/// Engine events for the brave://transfer page bridges. Always called on the
/// main thread; not `@MainActor` because the bridges also conform to the
/// nonisolated ObjC `TransferPageHandlerBridge`.
protocol TransferEventListener: AnyObject {
  func transferEvent(_ event: TransferAppEvent)
}

enum TransferAppEvent {
  case serviceState(TransferServiceState)
  case devices([TransferDevice])
  case offerReceived(IncomingOfferInfo)
  case offerRemoved(String)
  case transferUpdated(TransferInfo)
  case inboxChanged
  case settings(TransferSettings)
  case permissions(TransferPermissions)
}

/// Brave Transfer in the app, one per process: owns the engine, listens
/// while the app is in the foreground once the user has opened
/// brave://transfer (design plan §5.2), exports the Sync chain, and shows
/// the native Accept/Decline sheet and receive toast over any tab.
@MainActor
final class TransferAppCoordinator {
  static private(set) var shared: TransferAppCoordinator?

  /// Creates the coordinator the first time a window asks for it, if the
  /// BraveTransfer feature is enabled.
  static func sharedIfEnabled(syncAPI: BraveSyncAPI) -> TransferAppCoordinator? {
    guard FeatureList.kBraveTransfer.enabled else { return nil }
    if let shared {
      return shared
    }
    do {
      shared = try TransferAppCoordinator(syncAPI: syncAPI)
    } catch {
      log.error("Brave Transfer is unavailable: \(String(describing: error))")
    }
    return shared
  }

  static let log = Logger(subsystem: "com.brave.transfer", category: "app")
  /// How often transfers in flight are checked during the background-task
  /// grace period, which iOS limits to about 30 s.
  static let backgroundGraceCheckInterval: TimeInterval = 1

  let engine: TransferEngine
  weak var router: TransferAppRouting?

  private var exporter: SyncTrustExporter?
  private let engineObserver: EngineObserver
  private let listeners = NSHashTable<AnyObject>.weakObjects()
  private var backgroundTask: UIBackgroundTaskIdentifier = .invalid
  private var isInForeground: Bool
  private var shownOffer: (id: String, controller: UIViewController)?
  private var queuedOffers: [IncomingOfferInfo] = []
  private var toastView: UIView?
  private var notificationObservers: [NSObjectProtocol] = []
  private var temporaryDirectories: [String: URL] = [:]
  private(set) lazy var handoff = TransferHandoffSender(coordinator: self)

  /// Copies of items picked on brave://transfer, kept until they are sent.
  nonisolated static var pickedItemsDirectory: URL {
    FileManager.default.temporaryDirectory
      .appendingPathComponent("BraveTransferPicked", isDirectory: true)
  }

  private init(syncAPI: BraveSyncAPI) throws {
    let appGroup = try TransferAppGroup.current()
    let documents = try FileManager.default.url(
      for: .documentDirectory,
      in: .userDomainMask,
      appropriateFor: nil,
      create: true
    )
    engine = try TransferEngine(
      role: .app,
      appGroup: appGroup,
      downloadsDirectory: { documents.appendingPathComponent("Downloads", isDirectory: true) },
      inboxURL: appGroup.directory.appendingPathComponent("inbox.json")
    )
    engineObserver = EngineObserver()
    isInForeground = UIApplication.shared.applicationState != .background
    engineObserver.coordinator = self
    engine.addObserver(engineObserver)
    exporter = SyncTrustExporter(
      syncAPI: syncAPI,
      appGroup: appGroup,
      keyStore: engine.keyStore,
      onChange: { [weak self] in self?.engine.reload() }
    )
    observeLifecycle()
    appGroup.outbox.purge()
    try? FileManager.default.removeItem(at: Self.pickedItemsDirectory)
    updateListening()
  }

  /// Deletes `directory` once the transfer it feeds has ended.
  func removeWhenFinished(_ directory: URL, transferID: String) {
    temporaryDirectories[transferID] = directory
  }

  // MARK: Setup and listening

  var isSetupDone: Bool {
    engine.appGroup.defaults?.bool(forKey: TransferDefaultsKey.setupDone) ?? false
  }

  /// Called when a brave://transfer page connects. The first visit starts
  /// the listener, so the Local Network prompt only ever shows there
  /// (design plan D15).
  func markSetupDone() {
    guard !isSetupDone else { return }
    engine.appGroup.defaults?.set(true, forKey: TransferDefaultsKey.setupDone)
    updateListening()
  }

  private func updateListening() {
    let allowed = isSetupDone && isInForeground
    engine.perform { $0.setListeningAllowed(allowed) }
  }

  private func observeLifecycle() {
    let center = NotificationCenter.default
    notificationObservers = [
      center.addObserver(
        forName: UIApplication.willEnterForegroundNotification,
        object: nil,
        queue: .main
      ) { [weak self] _ in
        MainActor.assumeIsolated { self?.didEnterForeground() }
      },
      center.addObserver(
        forName: UIApplication.didEnterBackgroundNotification,
        object: nil,
        queue: .main
      ) { [weak self] _ in
        MainActor.assumeIsolated { self?.didEnterBackground() }
      },
    ]
  }

  private func didEnterForeground() {
    isInForeground = true
    endBackgroundTask()
    exporter?.export()
    engine.reload()
    updateListening()
    // The user may have changed a permission in Settings.
    dispatch(.permissions(permissions))
  }

  /// Stops listening. Transfers in flight get a background-task grace
  /// period; when it runs out they end as interrupted.
  private func didEnterBackground() {
    isInForeground = false
    engine.perform { [weak self] service in
      service.setListeningAllowed(false)
      let hasActive = service.hasActiveTransfers
      if !hasActive {
        service.closeAllConnections()
      }
      DispatchQueue.main.async {
        MainActor.assumeIsolated {
          if hasActive {
            self?.beginBackgroundTask()
          }
        }
      }
    }
  }

  private func beginBackgroundTask() {
    guard backgroundTask == .invalid else { return }
    backgroundTask = UIApplication.shared.beginBackgroundTask(withName: "BraveTransfer") {
      [weak self] in
      MainActor.assumeIsolated { self?.stopForBackground() }
    }
    checkBackgroundTransfers()
  }

  private func checkBackgroundTransfers() {
    guard backgroundTask != .invalid else { return }
    engine.perform { [weak self] service in
      let hasActive = service.hasActiveTransfers
      DispatchQueue.main.asyncAfter(
        deadline: .now() + (hasActive ? Self.backgroundGraceCheckInterval : 0)
      ) {
        MainActor.assumeIsolated {
          guard let self, self.backgroundTask != .invalid else { return }
          if hasActive {
            self.checkBackgroundTransfers()
          } else {
            self.stopForBackground()
          }
        }
      }
    }
  }

  private func stopForBackground() {
    if !isInForeground {
      engine.perform { service in
        service.setListeningAllowed(false)
        service.closeAllConnections()
      }
    }
    endBackgroundTask()
  }

  private func endBackgroundTask() {
    guard backgroundTask != .invalid else { return }
    let task = backgroundTask
    backgroundTask = .invalid
    // Let the engine finish closing before the app is suspended.
    engine.perform { _ in
      DispatchQueue.main.async { UIApplication.shared.endBackgroundTask(task) }
    }
  }

  // MARK: Listeners

  func addListener(_ listener: TransferEventListener) {
    listeners.add(listener)
  }

  func removeListener(_ listener: TransferEventListener) {
    listeners.remove(listener)
  }

  fileprivate func dispatch(_ event: TransferAppEvent) {
    for case let listener as TransferEventListener in listeners.allObjects {
      listener.transferEvent(event)
    }
    switch event {
    case .offerReceived(let offer):
      showOffer(offer)
    case .offerRemoved(let id):
      removeOffer(id)
    case .devices(let devices):
      handoff.devicesChanged(devices)
    case .transferUpdated(let transfer):
      handoff.transferUpdated(transfer)
      if transfer.state.isFinal,
        let directory = temporaryDirectories.removeValue(forKey: transfer.id)
      {
        try? FileManager.default.removeItem(at: directory)
      }
    default:
      break
    }
  }

  fileprivate func showReceivedToast(count: Int, from deviceName: String) {
    showToast(
      TransferToastView(
        message: TransferToastView.receivedMessage(count: count, from: deviceName),
        onView: { [weak self] in
          self?.hideToast()
          self?.router?.openTransferPage()
        }
      )
    )
  }

  fileprivate func showKeyMismatchToast(from deviceName: String) {
    showToast(
      TransferToastView(
        message: String.localizedStringWithFormat(
          Strings.BraveTransfer.keyMismatch,
          deviceName
        ),
        style: .warning
      )
    )
  }

  // MARK: Permissions

  var permissions: TransferPermissions {
    TransferPermissions(
      localNetwork: engine.localNetwork.lastKnownStatus,
      photos: ReceivedItemFinalizer.photosStatus
    )
  }

  /// Local Network first (starting the listener shows its prompt), then
  /// Photos add-only once that resolves, so only one system alert shows at
  /// a time (design plan §5.2).
  func requestPermissions(_ completion: @escaping (TransferPermissions) -> Void) {
    markSetupDone()
    engine.localNetwork.request { [weak self] _ in
      MainActor.assumeIsolated {
        guard let self else { return }
        let finish = { [weak self] in
          guard let self else { return }
          let permissions = self.permissions
          completion(permissions)
          self.dispatch(.permissions(permissions))
        }
        if ReceivedItemFinalizer.photosStatus == .notDetermined {
          ReceivedItemFinalizer.requestPhotos { _ in
            MainActor.assumeIsolated { finish() }
          }
        } else {
          finish()
        }
      }
    }
  }

  // MARK: Offer sheet

  private func showOffer(_ offer: IncomingOfferInfo) {
    if shownOffer != nil {
      queuedOffers.append(offer)
      return
    }
    guard let presenter = router?.transferPresentingViewController.topMostPresented else {
      return
    }
    let controller = UIHostingController(
      rootView: TransferOfferView(
        offer: offer,
        onAccept: { [weak self] in self?.respond(to: offer.id, accept: true) },
        onDecline: { [weak self] in self?.respond(to: offer.id, accept: false) }
      )
    )
    controller.modalPresentationStyle = .pageSheet
    controller.isModalInPresentation = true
    if let sheet = controller.sheetPresentationController {
      sheet.detents = [.medium(), .large()]
      sheet.prefersGrabberVisible = false
    }
    shownOffer = (offer.id, controller)
    presenter.present(controller, animated: true)
  }

  private func respond(to offerID: String, accept: Bool) {
    engine.perform { $0.respondToOffer(offerID, accept: accept) }
    removeOffer(offerID)
  }

  private func removeOffer(_ offerID: String) {
    queuedOffers.removeAll { $0.id == offerID }
    guard let shownOffer, shownOffer.id == offerID else { return }
    self.shownOffer = nil
    shownOffer.controller.dismiss(animated: true) { [weak self] in
      MainActor.assumeIsolated {
        guard let self, !self.queuedOffers.isEmpty else { return }
        self.showOffer(self.queuedOffers.removeFirst())
      }
    }
  }

  // MARK: Toast

  private func showToast(_ toast: TransferToastView) {
    hideToast()
    guard let window = router?.transferPresentingViewController.view.window else { return }
    let host = UIHostingController(rootView: toast)
    host.view.backgroundColor = .clear
    host.view.translatesAutoresizingMaskIntoConstraints = false
    window.addSubview(host.view)
    NSLayoutConstraint.activate([
      host.view.leadingAnchor.constraint(equalTo: window.safeAreaLayoutGuide.leadingAnchor),
      host.view.trailingAnchor.constraint(equalTo: window.safeAreaLayoutGuide.trailingAnchor),
      host.view.topAnchor.constraint(equalTo: window.safeAreaLayoutGuide.topAnchor, constant: 8),
    ])
    host.view.alpha = 0
    UIView.animate(withDuration: 0.2) { host.view.alpha = 1 }
    toastView = host.view
    UIAccessibility.post(notification: .announcement, argument: toast.message)
    DispatchQueue.main.asyncAfter(deadline: .now() + 4) { [weak self, weak view = host.view] in
      MainActor.assumeIsolated {
        guard let self, let view, self.toastView === view else { return }
        self.hideToast()
      }
    }
  }

  private func hideToast() {
    guard let view = toastView else { return }
    toastView = nil
    UIView.animate(withDuration: 0.2) {
      view.alpha = 0
    } completion: { _ in
      view.removeFromSuperview()
    }
  }
}

/// Receives engine callbacks on the engine queue, snapshots what the main
/// thread needs while still there, and hands it over.
private final class EngineObserver: TransferServiceObserver {
  weak var coordinator: TransferAppCoordinator?

  private func send(_ event: TransferAppEvent) {
    DispatchQueue.main.async { [weak self] in
      MainActor.assumeIsolated {
        self?.coordinator?.dispatch(event)
      }
    }
  }

  func transferServiceStateChanged(_ service: TransferService) {
    send(.serviceState(service.serviceState))
  }

  func transferServiceDevicesChanged(_ service: TransferService) {
    send(.devices(service.devices))
  }

  func transferService(_ service: TransferService, didReceiveOffer offer: IncomingOfferInfo) {
    send(.offerReceived(offer))
  }

  func transferService(_ service: TransferService, didRemoveOffer offerID: String) {
    send(.offerRemoved(offerID))
  }

  func transferService(_ service: TransferService, didUpdateTransfer transfer: TransferInfo) {
    send(.transferUpdated(transfer))
  }

  func transferServiceInboxChanged(_ service: TransferService) {
    send(.inboxChanged)
  }

  func transferServiceSettingsChanged(_ service: TransferService) {
    send(.settings(service.settings))
    send(.serviceState(service.serviceState))
  }

  func transferService(
    _ service: TransferService,
    didReceiveItems count: Int,
    from deviceName: String
  ) {
    DispatchQueue.main.async { [weak self] in
      MainActor.assumeIsolated {
        self?.coordinator?.showReceivedToast(count: count, from: deviceName)
      }
    }
  }

  func transferService(
    _ service: TransferService,
    didRejectKeyMismatchFrom deviceName: String
  ) {
    DispatchQueue.main.async { [weak self] in
      MainActor.assumeIsolated {
        self?.coordinator?.showKeyMismatchToast(from: deviceName)
      }
    }
  }
}

extension UIViewController {
  fileprivate var topMostPresented: UIViewController {
    var controller = self
    while let presented = controller.presentedViewController, !presented.isBeingDismissed {
      controller = presented
    }
    return controller
  }
}
