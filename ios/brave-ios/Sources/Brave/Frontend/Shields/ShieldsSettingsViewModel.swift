// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Data
import Foundation
import Web

@MainActor
class ShieldsSettingsViewModel: ObservableObject {
  private let tab: any TabState

  @Published var stats: TPPageStats
  @Published var shieldsEnabled: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBraveShieldsEnabled(shieldsEnabled, for: tab.visibleURL)
      updateState()
    }
  }
  @Published var blockAdsAndTrackingLevel: ShieldLevel {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setShieldLevel(blockAdsAndTrackingLevel, for: tab.visibleURL)
    }
  }
  @Published var blockScripts: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBlockScriptsEnabled(blockScripts, for: tab.visibleURL)
    }
  }
  @Published var fingerprintProtection: Bool {
    didSet {
      guard !isUpdatingState else { return }
      tab.braveShieldsHelper?.setBlockFingerprintingEnabled(
        fingerprintProtection,
        for: tab.visibleURL
      )
    }
  }

  var isPrivateBrowsing: Bool {
    return tab.isPrivate
  }

  /// If we are updating our state values, we don't want to assign to the domain preference.
  private var isUpdatingState: Bool = false

  init(tab: some TabState) {
    self.tab = tab
    self.shieldsEnabled =
      tab.braveShieldsHelper?.isBraveShieldsEnabled(for: tab.visibleURL)
      ?? true
    self.blockAdsAndTrackingLevel =
      tab.braveShieldsHelper?.shieldLevel(for: tab.visibleURL, considerAllShieldsOption: true)
      ?? .standard
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
    self.stats = tab.contentBlocker?.stats ?? .init()

    tab.contentBlocker?.statsDidChange = { [weak self, weak tab] _ in
      self?.stats = tab?.contentBlocker?.stats ?? .init()
    }
  }

  /// Update our properties value without affecting the Domain's value.
  private func updateState() {
    isUpdatingState = true
    defer { isUpdatingState = false }
    shieldsEnabled = tab.braveShieldsHelper?.isBraveShieldsEnabled(for: tab.visibleURL) ?? true
    blockAdsAndTrackingLevel =
      tab.braveShieldsHelper?.shieldLevel(
        for: tab.visibleURL,
        considerAllShieldsOption: false
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
  }
}
