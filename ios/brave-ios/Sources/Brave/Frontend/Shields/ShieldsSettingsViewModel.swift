// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Foundation
import Web

extension BraveShields {
  static let isUseContentSettingsForShieldsEnabled = true
}

@MainActor
class ShieldsSettingsViewModel: ObservableObject {
  private let tab: any TabState
  private let domain: Domain
  private let url: URL
  private let braveShieldsUtils: BraveShieldsUtilsIOS

  @Published var stats: TPPageStats
  @Published var shieldsEnabled: Bool {
    didSet {
      if BraveShields.isUseContentSettingsForShieldsEnabled {
        guard oldValue != shieldsEnabled else { return }
        braveShieldsUtils.setBraveShieldsEnabled(shieldsEnabled, for: url)
      } else {
        guard !isUpdatingState else { return }
        domain.shield_allOff = NSNumber(booleanLiteral: !shieldsEnabled)
        updateState()
      }
    }
  }
  @Published var blockAdsAndTrackingLevel: ShieldLevel {
    didSet {
      if BraveShields.isUseContentSettingsForShieldsEnabled {
        guard oldValue != blockAdsAndTrackingLevel else { return }
        braveShieldsUtils.setAdBlockMode(blockAdsAndTrackingLevel.adBlockMode, for: url)
      } else {
        guard !isUpdatingState else { return }
        domain.domainBlockAdsAndTrackingLevel = blockAdsAndTrackingLevel
      }
    }
  }
  @Published var blockScripts: Bool {
    didSet {
      if BraveShields.isUseContentSettingsForShieldsEnabled {
        guard oldValue != blockScripts else { return }
        braveShieldsUtils.setBlockScriptsEnabled(blockScripts, for: url)
      } else {
        guard !isUpdatingState else { return }
        domain.shield_noScript = NSNumber(booleanLiteral: blockScripts)
      }
    }
  }
  @Published var fingerprintProtection: Bool {
    didSet {
      if BraveShields.isUseContentSettingsForShieldsEnabled {
        guard oldValue != fingerprintProtection else { return }
        braveShieldsUtils.setBlockFingerprintingEnabled(fingerprintProtection, for: url)
      } else {
        guard !isUpdatingState else { return }
        domain.shield_fpProtection = NSNumber(booleanLiteral: fingerprintProtection)
      }
    }
  }

  var isPrivateBrowsing: Bool {
    return tab.isPrivate
  }

  /// If we are updating our state values, we don't want to assign to the domain preference.
  private var isUpdatingState: Bool = false

  init(tab: some TabState, domain: Domain, url: URL, braveShieldsUtils: BraveShieldsUtilsIOS) {
    self.tab = tab
    self.domain = domain
    self.url = url
    self.braveShieldsUtils = braveShieldsUtils
    if BraveShields.isUseContentSettingsForShieldsEnabled {
      self.shieldsEnabled = braveShieldsUtils.isBraveShieldsEnabled(for: url)
      self.blockAdsAndTrackingLevel = braveShieldsUtils.adBlockMode(for: url).shieldLevel
      self.blockScripts = braveShieldsUtils.isShieldExpected(
        url: url,
        shield: .noScript,
        considerAllShieldsOption: true
      )
      self.fingerprintProtection = braveShieldsUtils.isShieldExpected(
        url: url,
        shield: .fpProtection,
        considerAllShieldsOption: true
      )
      self.stats = .init()
    } else {
      self.shieldsEnabled = !domain.areAllShieldsOff
      self.blockAdsAndTrackingLevel = domain.domainBlockAdsAndTrackingLevel
      self.blockScripts = domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
      self.fingerprintProtection = domain.isShieldExpected(
        .fpProtection,
        considerAllShieldsOption: true
      )
    }
    self.stats = tab.contentBlocker?.stats ?? .init()

    tab.contentBlocker?.statsDidChange = { [weak self, weak tab] _ in
      self?.stats = tab?.contentBlocker?.stats ?? .init()
    }
  }

  /// Update our properties value without affecting the Domain's value.
  private func updateState() {
    guard !BraveShields.isUseContentSettingsForShieldsEnabled else { return }
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
