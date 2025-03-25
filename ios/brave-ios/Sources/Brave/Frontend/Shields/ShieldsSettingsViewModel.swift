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
  private var domain: Domain
  private let tab: Tab

  @Published var stats: TPPageStats
  @Published var shieldsEnabled: Bool {
    didSet {
      guard !isUpdatingState else { return }
      domain.shield_allOff = NSNumber(booleanLiteral: !shieldsEnabled)
      updateState()
    }
  }
  @Published var blockAdsAndTrackingLevel: ShieldLevel {
    didSet {
      guard !isUpdatingState else { return }
      domain.domainBlockAdsAndTrackingLevel = blockAdsAndTrackingLevel
    }
  }
  @Published var blockScripts: Bool {
    didSet {
      guard !isUpdatingState else { return }
      domain.shield_noScript = NSNumber(booleanLiteral: blockScripts)
    }
  }
  @Published var fingerprintProtection: Bool {
    didSet {
      guard !isUpdatingState else { return }
      domain.shield_fpProtection = NSNumber(booleanLiteral: fingerprintProtection)
    }
  }

  var isPrivateBrowsing: Bool {
    return tab.isPrivate
  }

  /// If we are updating our state values, we don't want to assign to the domain preference.
  private var isUpdatingState: Bool = false

  init(tab: Tab, domain: Domain) {
    self.domain = domain
    self.tab = tab
    self.shieldsEnabled = !domain.areAllShieldsOff
    self.blockAdsAndTrackingLevel = domain.domainBlockAdsAndTrackingLevel
    self.blockScripts = domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
    self.fingerprintProtection = domain.isShieldExpected(
      .fpProtection,
      considerAllShieldsOption: true
    )
    self.stats = tab.contentBlocker?.stats ?? .init()

    tab.contentBlocker?.statsDidChange = { [weak self, weak tab] _ in
      self?.stats = tab?.contentBlocker?.stats ?? .init()
    }
  }

  /// Update our properties value without affecting the Domain's value.
  private func updateState() {
    isUpdatingState = true
    defer { isUpdatingState = false }
    shieldsEnabled = !domain.areAllShieldsOff
    blockAdsAndTrackingLevel = domain.domainBlockAdsAndTrackingLevel
    fingerprintProtection = domain.isShieldExpected(
      .fpProtection,
      considerAllShieldsOption: true
    )
    blockScripts = domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
  }
}
