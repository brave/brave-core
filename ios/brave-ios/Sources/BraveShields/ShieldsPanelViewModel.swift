// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Data
import Foundation
import Web

@MainActor
public class ShieldsPanelViewModel: ObservableObject {
  private let tab: any TabState

  @Published public var stats: TrackingProtectionPageStats
  /// The requests blocked on the current page. Only used for debugging.
  ///
  /// Deliberately not `@Published`: blocked requests are appended on every
  /// blocked request, and only the debug view subscribes to them.
  public let blockedRequests: AnyPublisher<[BlockedRequestInfo], Never>
  @Published public var shieldsEnabled: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBraveShieldsEnabled(shieldsEnabled, for: tab.visibleURL)
      updateState()
    }
  }
  @Published public var blockAdsAndTrackingLevel: ShieldLevel {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setShieldLevel(blockAdsAndTrackingLevel, for: tab.visibleURL)
    }
  }
  @Published public var blockScripts: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBlockScriptsEnabled(blockScripts, for: tab.visibleURL)
    }
  }
  @Published public var fingerprintProtection: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBlockFingerprintingEnabled(
        fingerprintProtection,
        for: tab.visibleURL
      )
    }
  }
  @Published public var autoShredLevel: SiteShredLevel {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setShredLevel(autoShredLevel, for: tab.visibleURL)
    }
  }
  /// A boolean value indicates to wether to show `Advanced controls`or not inside the shield panel
  /// with a default value true
  @Published public var advancedControlsEnabled: Bool = true

  public var isPrivateBrowsing: Bool {
    tab.isPrivate
  }

  public var visibleURL: URL? {
    tab.visibleURL
  }

  /// If we are updating our state values, we don't want to assign to the domain preference.
  private var isUpdatingState: Bool = false

  public init(
    tab: some TabState,
    stats: some Publisher<TrackingProtectionPageStats, Never>,
    blockedRequests: AnyPublisher<[BlockedRequestInfo], Never>,
    isAdvancedControlsEnabled: Bool
  ) {
    self.tab = tab
    self.blockedRequests = blockedRequests
    self.shieldsEnabled =
      tab.braveShieldsHelper?.isBraveShieldsEnabled(for: tab.visibleURL)
      ?? true
    // `considerAlwaysAggressiveETLDs` is `false` to mask that we may force
    // aggressive mode on some domains.
    self.blockAdsAndTrackingLevel =
      tab.braveShieldsHelper?.shieldLevel(
        for: tab.visibleURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ) ?? .standard
    self.blockScripts =
      tab.braveShieldsHelper?.isShieldExpected(
        for: tab.visibleURL,
        shield: .noScript,
        considerAllShieldsOption: true
      ) ?? false
    self.fingerprintProtection =
      tab.braveShieldsHelper?.isShieldExpected(
        for: tab.visibleURL,
        shield: .fpProtection,
        considerAllShieldsOption: true
      ) ?? true
    self.autoShredLevel =
      tab.braveShieldsHelper?.shredLevel(
        for: tab.visibleURL,
        considerAllShieldsOption: true
      ) ?? .never
    self.stats = .init()
    self.advancedControlsEnabled = isAdvancedControlsEnabled

    stats.receive(on: DispatchQueue.main).assign(to: &$stats)
  }

  /// Update our properties value without affecting the Domain's value.
  private func updateState() {
    isUpdatingState = true
    defer { isUpdatingState = false }
    shieldsEnabled = tab.braveShieldsHelper?.isBraveShieldsEnabled(for: tab.visibleURL) ?? true
    // `considerAlwaysAggressiveETLDs` is `false` to mask that we may force
    // aggressive mode on some domains.
    blockAdsAndTrackingLevel =
      tab.braveShieldsHelper?.shieldLevel(
        for: tab.visibleURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ) ?? .standard
    self.blockScripts =
      tab.braveShieldsHelper?.isShieldExpected(
        for: tab.visibleURL,
        shield: .noScript,
        considerAllShieldsOption: true
      ) ?? false
    self.fingerprintProtection =
      tab.braveShieldsHelper?.isShieldExpected(
        for: tab.visibleURL,
        shield: .fpProtection,
        considerAllShieldsOption: true
      ) ?? true
    self.autoShredLevel =
      tab.braveShieldsHelper?.shredLevel(
        for: tab.visibleURL,
        considerAllShieldsOption: true
      ) ?? .never
  }
}
