// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Foundation
import Network

/// iOS has no API to read the Local Network permission. This advertises a
/// throwaway `_brave-xfer._tcp` instance and browses for it: finding it means
/// access is granted, and a PolicyDenied error means it is denied. The first
/// run shows the system prompt. The last result is remembered so the page can
/// show it without probing again.
public final class LocalNetworkAuthorization {
  static let statusKey = "braveTransfer.localNetworkStatus"
  public static let timeout: TimeInterval = 30

  private let defaults: UserDefaults?
  private let queue = DispatchQueue(label: "com.brave.transfer.local-network-probe")
  private var listener: NWListener?
  private var browser: NWBrowser?
  private var timer: DispatchWorkItem?
  private var isProbing = false
  private var completions: [(PermissionStatus) -> Void] = []

  public init(defaults: UserDefaults?) {
    self.defaults = defaults
  }

  /// The last known status; `.notDetermined` until a probe or the advertiser
  /// reported one.
  public var lastKnownStatus: PermissionStatus {
    defaults.flatMap { PermissionStatus(rawValue: $0.integer(forKey: Self.statusKey)) }
      ?? .notDetermined
  }

  /// Records a status the advertiser observed while running.
  public func record(_ status: PermissionStatus) {
    guard status != .notDetermined else { return }
    defaults?.set(status.rawValue, forKey: Self.statusKey)
  }

  public func record(_ state: AdvertiserState) {
    switch state {
    case .advertising: record(.granted)
    case .waiting(localNetworkDenied: true): record(.denied)
    default: break
    }
  }

  /// Probes the permission, prompting if it was never asked. `completion`
  /// runs on the main queue.
  public func request(_ completion: @escaping (PermissionStatus) -> Void) {
    queue.async {
      self.completions.append(completion)
      guard !self.isProbing else { return }
      self.startProbe()
    }
  }

  private func startProbe() {
    isProbing = true
    let name = DiscoveryConstants.randomInstanceName()
    guard let listener = try? NWListener(using: BonjourAdvertiser.parameters()) else {
      finish(lastKnownStatus)
      return
    }
    listener.service = NWListener.Service(
      name: name,
      type: DiscoveryConstants.serviceType,
      domain: "local."
    )
    listener.newConnectionHandler = { $0.cancel() }
    listener.stateUpdateHandler = { [weak self] state in
      if case .waiting(let error) = state, BonjourAdvertiser.isLocalNetworkDenied(error) {
        self?.finish(.denied)
      } else if case .failed(let error) = state {
        self?.finish(BonjourAdvertiser.isLocalNetworkDenied(error) ? .denied : .notDetermined)
      }
    }

    let parameters = NWParameters()
    parameters.includePeerToPeer = false
    let browser = NWBrowser(
      for: .bonjour(type: DiscoveryConstants.serviceType, domain: "local."),
      using: parameters
    )
    browser.stateUpdateHandler = { [weak self] state in
      if case .waiting(let error) = state, BonjourAdvertiser.isLocalNetworkDenied(error) {
        self?.finish(.denied)
      } else if case .failed(let error) = state {
        self?.finish(BonjourAdvertiser.isLocalNetworkDenied(error) ? .denied : .notDetermined)
      }
    }
    browser.browseResultsChangedHandler = { [weak self] results, _ in
      let found = results.contains { result in
        if case .service(let serviceName, _, _, _) = result.endpoint {
          return serviceName == name
        }
        return false
      }
      if found {
        self?.finish(.granted)
      }
    }

    self.listener = listener
    self.browser = browser
    let timer = DispatchWorkItem { [weak self] in
      guard let self else { return }
      self.finish(self.lastKnownStatus)
    }
    self.timer = timer
    queue.asyncAfter(deadline: .now() + Self.timeout, execute: timer)
    listener.start(queue: queue)
    browser.start(queue: queue)
  }

  private func finish(_ status: PermissionStatus) {
    guard isProbing else { return }
    isProbing = false
    timer?.cancel()
    timer = nil
    listener?.cancel()
    listener = nil
    browser?.cancel()
    browser = nil
    record(status)
    let completions = self.completions
    self.completions = []
    DispatchQueue.main.async {
      completions.forEach { $0(status) }
    }
  }
}
