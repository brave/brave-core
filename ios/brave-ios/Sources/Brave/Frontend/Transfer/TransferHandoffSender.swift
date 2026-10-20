// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransfer
import BraveTransferEngine
import Foundation
import Strings
import UIKit

/// Sends items the share extension handed to the app through the outbox
/// (`brave://transfer/send?outbox=<id>`, design plan §5.3). The app sends
/// while it is in the foreground and a computer is connected, then deletes
/// the outbox entry.
@MainActor
final class TransferHandoffSender {
  private unowned let coordinator: TransferAppCoordinator
  private var pending: UUID?
  private var isPreparing = false
  private var isChoosingDevice = false
  private var sending: (outbox: UUID, transferID: String)?

  init(coordinator: TransferAppCoordinator) {
    self.coordinator = coordinator
  }

  private var outbox: Outbox { coordinator.engine.appGroup.outbox }

  func handle(_ id: UUID) {
    guard outbox.manifest(for: id) != nil else {
      TransferAppCoordinator.log.error("Hand-off outbox \(id.uuidString) is missing")
      return
    }
    pending = id
    coordinator.markSetupDone()
    coordinator.router?.openTransferPage()
    coordinator.engine.perform { [weak self] service in
      let devices = service.devices
      DispatchQueue.main.async {
        MainActor.assumeIsolated { self?.devicesChanged(devices) }
      }
    }
  }

  func devicesChanged(_ devices: [TransferDevice]) {
    guard let id = pending, !isPreparing, !isChoosingDevice, sending == nil,
      let manifest = outbox.manifest(for: id)
    else { return }
    let connected = devices.filter(\.isConnected)
    if let wanted = manifest.deviceID, connected.contains(where: { $0.id == wanted }) {
      send(id, to: wanted)
    } else if connected.count == 1 {
      send(id, to: connected[0].id)
    } else if connected.count > 1 {
      chooseDevice(for: id, among: connected)
    }
  }

  func transferUpdated(_ transfer: TransferInfo) {
    guard let sending, sending.transferID == transfer.id, transfer.state.isFinal else { return }
    self.sending = nil
    switch transfer.state {
    case .failed:
      // Keep the entry; it is retried when a computer connects again and
      // purged after a day.
      pending = sending.outbox
    default:
      outbox.remove(sending.outbox)
    }
  }

  private func chooseDevice(for id: UUID, among devices: [TransferDevice]) {
    guard let presenter = coordinator.router?.transferPresentingViewController else { return }
    isChoosingDevice = true
    let alert = UIAlertController(
      title: Strings.BraveTransfer.shareTitle,
      message: nil,
      preferredStyle: .actionSheet
    )
    for device in devices {
      alert.addAction(
        UIAlertAction(title: device.name, style: .default) { [weak self] _ in
          self?.isChoosingDevice = false
          self?.send(id, to: device.id)
        }
      )
    }
    alert.addAction(
      UIAlertAction(title: Strings.BraveTransfer.shareCancel, style: .cancel) { [weak self] _ in
        self?.isChoosingDevice = false
        self?.pending = nil
        self?.outbox.remove(id)
      }
    )
    if let popover = alert.popoverPresentationController {
      popover.sourceView = presenter.view
      popover.sourceRect = CGRect(
        x: presenter.view.bounds.midX,
        y: presenter.view.bounds.midY,
        width: 0,
        height: 0
      )
      popover.permittedArrowDirections = []
    }
    var top = presenter
    while let presented = top.presentedViewController {
      top = presented
    }
    top.present(alert, animated: true)
  }

  private func send(_ id: UUID, to deviceID: String) {
    pending = nil
    isPreparing = true
    let outbox = self.outbox
    let engine = coordinator.engine
    DispatchQueue.global(qos: .userInitiated).async { [weak self] in
      let items = try? outbox.outgoingItems(for: id)
      engine.perform { service in
        let result = items.map { service.send($0, to: deviceID) } ?? .failed
        let transferID = result == .success ? service.lastTransferID : nil
        DispatchQueue.main.async {
          MainActor.assumeIsolated {
            guard let self else { return }
            self.isPreparing = false
            if let transferID {
              self.sending = (id, transferID)
            } else if result == .deviceUnavailable {
              self.pending = id
            } else {
              TransferAppCoordinator.log.error("Could not send hand-off \(id.uuidString)")
              outbox.remove(id)
            }
          }
        }
      }
    }
  }
}
