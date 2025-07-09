// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Foundation
import Web

@MainActor
class ShieldsSettingsViewModel: ObservableObject {
  private let tab: any TabState
  private let domain: Domain
  private let url: URL
  private let braveShieldsUtils: BraveShieldsUtilsIOS

  @Published var stats: TPPageStats
  @Published var shieldsEnabled: Bool {
    didSet {
      guard !isUpdatingState else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsUtils.setBraveShieldsEnabled(
          shieldsEnabled,
          for: url,
          isPrivate: isPrivateBrowsing
        )
      } else {
        domain.shield_allOff = NSNumber(booleanLiteral: !shieldsEnabled)
      }
      updateState()
    }
  }
  @Published var blockAdsAndTrackingLevel: ShieldLevel {
    didSet {
      guard !isUpdatingState else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsUtils.setAdBlockMode(
          blockAdsAndTrackingLevel.adBlockMode,
          for: url,
          isPrivate: isPrivateBrowsing
        )
      } else {
        domain.domainBlockAdsAndTrackingLevel = blockAdsAndTrackingLevel
      }
      updateState()
    }
  }
  @Published var blockScripts: Bool {
    didSet {
      guard !isUpdatingState else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsUtils.setBlockScriptsEnabled(
          blockScripts,
          for: url,
          isPrivate: isPrivateBrowsing
        )
      } else {
        domain.shield_noScript = NSNumber(booleanLiteral: blockScripts)
      }
      updateState()
    }
  }
  @Published var fingerprintProtection: Bool {
    didSet {
      guard !isUpdatingState else { return }
      if FeatureList.kBraveShieldsContentSettings.enabled {
        braveShieldsUtils.setBlockFingerprintingEnabled(
          fingerprintProtection,
          for: url,
          isPrivate: isPrivateBrowsing
        )
      } else {
        domain.shield_fpProtection = NSNumber(booleanLiteral: fingerprintProtection)
      }
      updateState()
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
    if FeatureList.kBraveShieldsContentSettings.enabled {
      self.shieldsEnabled = braveShieldsUtils.isBraveShieldsEnabled(
        for: url,
        isPrivate: tab.isPrivate
      )
      self.blockAdsAndTrackingLevel =
        braveShieldsUtils.adBlockMode(for: url, isPrivate: tab.isPrivate).shieldLevel
      self.blockScripts = braveShieldsUtils.isShieldExpected(
        url: url,
        isPrivate: tab.isPrivate,
        shield: .noScript,
        considerAllShieldsOption: true
      )
      self.fingerprintProtection = braveShieldsUtils.isShieldExpected(
        url: url,
        isPrivate: tab.isPrivate,
        shield: .fpProtection,
        considerAllShieldsOption: true
      )
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

  /// Update our Published properties value(s) without affecting the underlying's source of truth value.
  private func updateState() {
    isUpdatingState = true
    defer { isUpdatingState = false }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      self.shieldsEnabled = braveShieldsUtils.isBraveShieldsEnabled(
        for: url,
        isPrivate: tab.isPrivate
      )
      self.blockAdsAndTrackingLevel =
        braveShieldsUtils.adBlockMode(for: url, isPrivate: tab.isPrivate).shieldLevel
      self.blockScripts = braveShieldsUtils.isShieldExpected(
        url: url,
        isPrivate: tab.isPrivate,
        shield: .noScript,
        considerAllShieldsOption: true
      )
      self.fingerprintProtection = braveShieldsUtils.isShieldExpected(
        url: url,
        isPrivate: tab.isPrivate,
        shield: .fpProtection,
        considerAllShieldsOption: true
      )
    } else {
      shieldsEnabled = !domain.areAllShieldsOff
      blockAdsAndTrackingLevel = domain.domainBlockAdsAndTrackingLevel
      fingerprintProtection = domain.isShieldExpected(
        .fpProtection,
        considerAllShieldsOption: true
      )
      blockScripts = domain.isShieldExpected(.noScript, considerAllShieldsOption: true)
    }
  }
}
