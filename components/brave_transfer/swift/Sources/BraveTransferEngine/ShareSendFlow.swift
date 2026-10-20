// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

/// The "Brave Transfer" share extension's flow (design plan §5.3): advertise,
/// list the computers that connect, send to the one the user picks, and fall
/// back to handing the items to the app when sending directly will not work.
/// Confined to the service's scheduler; `onChange` runs there too.
public final class ShareSendFlow: TransferServiceObserver {
  public enum Phase: Equatable, Sendable {
    /// Waiting for `begin`, while the extension reads the shared items.
    case preparing
    /// Advertising; no computer has connected yet.
    case searching
    /// At least one computer is connected.
    case choosing
    /// Waiting for the computer to accept, then sending.
    case sending(transferID: String)
    /// The transfer ended: completed, declined, expired, cancelled or failed.
    case finished(TransferState)
    /// The items should go to the main app instead.
    case handoff(HandoffPolicy.Reason)
  }

  public struct Snapshot: Equatable, Sendable {
    public var phase: Phase
    /// Connected computers only; the extension cannot send to others.
    public var devices: [TransferDevice]
    public var transfer: TransferInfo?
    /// Set once the items are hashed and can be sent.
    public var isReadyToSend: Bool
    /// The last send that was refused before it started.
    public var lastSendResult: SendResult?
  }

  public let policy: HandoffPolicy
  public var onChange: ((Snapshot) -> Void)?

  private let service: TransferService
  private let scheduler: TransferScheduler
  private var noDeviceTimer: TransferTimer?
  private var items: [OutgoingItem]?
  private var lastSendResult: SendResult?
  public private(set) var phase: Phase = .preparing

  public init(
    service: TransferService, scheduler: TransferScheduler, policy: HandoffPolicy = .init()
  ) {
    self.service = service
    self.scheduler = scheduler
    self.policy = policy
    service.addObserver(self)
  }

  deinit {
    noDeviceTimer?.cancel()
  }

  public var snapshot: Snapshot {
    var transfer: TransferInfo?
    if case .sending(let id) = phase {
      transfer = service.transfer(id)
    } else if case .finished = phase, let id = service.lastTransferID {
      transfer = service.transfer(id)
    }
    return Snapshot(
      phase: phase, devices: connectedDevices, transfer: transfer, isReadyToSend: items != nil,
      lastSendResult: lastSendResult)
  }

  private var connectedDevices: [TransferDevice] {
    service.devices.filter(\.isConnected)
  }

  /// Starts once the size of the shared items is known, before they are
  /// hashed. `localNetwork` is the last known Local Network status: the
  /// extension cannot prompt for it (TN3179), so an undetermined or denied
  /// status goes to the app.
  public func begin(totalBytes: UInt64, localNetwork: PermissionStatus) {
    guard phase == .preparing else { return }
    if let reason = policy.reasonBeforeSending(totalBytes: totalBytes) {
      handOff(reason)
      return
    }
    guard localNetwork == .granted || localNetwork == .limited else {
      handOff(.localNetworkUnavailable)
      return
    }
    guard service.serviceState.syncEnabled, service.settings.autoTrustSyncChain else {
      handOff(.notSetUp)
      return
    }
    phase = .searching
    service.setListeningAllowed(true)
    noDeviceTimer = scheduler.schedule(after: policy.noDeviceTimeout) { [weak self] in
      self?.noDeviceTimerFired()
    }
    devicesChanged()
  }

  /// The items, once hashed. The user can pick a computer before this.
  public func setItems(_ items: [OutgoingItem]) {
    switch phase {
    case .sending, .finished, .handoff:
      return
    case .preparing, .searching, .choosing:
      break
    }
    guard !items.isEmpty else {
      phase = .finished(.failed)
      stopListening()
      notifyChange()
      return
    }
    self.items = items
    let total = items.reduce(UInt64(0)) { $0 + $1.size }
    if let reason = policy.reasonBeforeSending(totalBytes: total) {
      handOff(reason)
      return
    }
    notifyChange()
  }

  @discardableResult
  public func send(to deviceID: String) -> SendResult {
    guard phase == .choosing, let items else { return .failed }
    let result = service.send(items, to: deviceID)
    lastSendResult = result == .success ? nil : result
    switch result {
    case .success:
      if let id = service.lastTransferID {
        phase = .sending(transferID: id)
        if let transfer = service.transfer(id), transfer.state.isFinal {
          phase = .finished(transfer.state)
        }
      }
    case .tooLarge:
      handOff(.tooLarge)
      return result
    default:
      break
    }
    notifyChange()
    return result
  }

  /// "Continue in Brave".
  public func continueInApp() {
    switch phase {
    case .handoff, .finished, .sending:
      return
    default:
      handOff(.userRequested)
    }
  }

  /// The user closed the sheet. A transfer in flight is cancelled; otherwise
  /// listening stops and connections close.
  public func cancel() {
    noDeviceTimer?.cancel()
    noDeviceTimer = nil
    if case .sending(let id) = phase {
      service.cancelTransfer(id)
      return
    }
    stopListening()
  }

  private func handOff(_ reason: HandoffPolicy.Reason) {
    phase = .handoff(reason)
    noDeviceTimer?.cancel()
    noDeviceTimer = nil
    stopListening()
    notifyChange()
  }

  private func stopListening() {
    service.setListeningAllowed(false)
    service.closeAllConnections()
  }

  private func noDeviceTimerFired() {
    noDeviceTimer = nil
    if phase == .searching {
      handOff(.noDevice)
    }
  }

  private func devicesChanged() {
    switch phase {
    case .searching where !connectedDevices.isEmpty:
      phase = .choosing
      noDeviceTimer?.cancel()
      noDeviceTimer = nil
    case .choosing where connectedDevices.isEmpty:
      phase = .searching
    default:
      break
    }
    notifyChange()
  }

  private func notifyChange() {
    onChange?(snapshot)
  }

  // MARK: TransferServiceObserver

  public func transferServiceDevicesChanged(_ service: TransferService) {
    devicesChanged()
  }

  public func transferServiceStateChanged(_ service: TransferService) {
    if case .waiting(localNetworkDenied: true) = service.advertiserState {
      switch phase {
      case .searching, .choosing:
        handOff(.localNetworkUnavailable)
      default:
        break
      }
    }
  }

  public func transferService(
    _ service: TransferService, didUpdateTransfer transfer: TransferInfo
  ) {
    guard case .sending(let id) = phase, id == transfer.id else { return }
    if transfer.state.isFinal {
      phase = .finished(transfer.state)
      // The connection stays open so the last frames (Done or Cancel) are
      // flushed; it closes when the extension exits.
      scheduler.post { [weak self] in self?.service.setListeningAllowed(false) }
    }
    notifyChange()
  }
}
