// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import Web

extension TabDataValues {
  private struct BraveShieldsTabHelperKey: TabDataKey {
    static var defaultValue: BraveShieldsTabHelper?
  }
  public var braveShieldsHelper: BraveShieldsTabHelper? {
    get { self[BraveShieldsTabHelperKey.self] }
    set { self[BraveShieldsTabHelperKey.self] = newValue }
  }
}

@MainActor
public class BraveShieldsTabHelper {
  private weak var tab: (any TabState)?
  private let braveShieldsSettings: any BraveShieldsSettings
  private let isBraveShieldsContentSettingsEnabled: Bool

  public init(
    tab: some TabState,
    braveShieldsSettings: any BraveShieldsSettings,
    isBraveShieldsContentSettingsEnabled: Bool = FeatureList.kBraveShieldsContentSettings.enabled
  ) {
    self.tab = tab
    self.braveShieldsSettings = braveShieldsSettings
    self.isBraveShieldsContentSettingsEnabled = isBraveShieldsContentSettingsEnabled
  }

  public func isBraveShieldsEnabled(for url: URL?) -> Bool {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return false }
    if isBraveShieldsContentSettingsEnabled {
      return braveShieldsSettings.isBraveShieldsEnabled(for: url)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    return !domain.areAllShieldsOff
  }

  public func setBraveShieldsEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if isBraveShieldsContentSettingsEnabled {
      braveShieldsSettings.setBraveShieldsEnabled(isEnabled, for: url)
    }
    // Also assign to Domain until deprecated so reverse migration is required
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_allOff = NSNumber(booleanLiteral: !isEnabled)
    DataController.performOnMainContext { context in
      try? context.save()
    }
  }

  public func shieldLevel(for url: URL?, considerAllShieldsOption: Bool) -> ShieldLevel {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return .disabled }
    if isBraveShieldsContentSettingsEnabled {
      if considerAllShieldsOption && !isBraveShieldsEnabled(for: url) {
        return .disabled
      }
      return braveShieldsSettings.adBlockMode(for: url).shieldLevel
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    if considerAllShieldsOption {
      return domain.globalBlockAdsAndTrackingLevel
    } else {
      return domain.domainBlockAdsAndTrackingLevel
    }
  }

  public func setShieldLevel(_ shieldLevel: ShieldLevel, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if isBraveShieldsContentSettingsEnabled {
      braveShieldsSettings.setAdBlockMode(shieldLevel.adBlockMode, for: url)
    }
    // Also assign to Domain until deprecated so reverse migration is required
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.domainBlockAdsAndTrackingLevel = shieldLevel
    DataController.performOnMainContext { context in
      try? context.save()
    }
  }

  public func setBlockScriptsEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if isBraveShieldsContentSettingsEnabled {
      braveShieldsSettings.setBlockScriptsEnabled(isEnabled, for: url)
    }
    // Also assign to Domain until deprecated so reverse migration is required
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_noScript = NSNumber(booleanLiteral: isEnabled)
    DataController.performOnMainContext { context in
      try? context.save()
    }
  }

  public func setBlockFingerprintingEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if isBraveShieldsContentSettingsEnabled {
      braveShieldsSettings.setFingerprintMode(isEnabled ? .standardMode : .allowMode, for: url)
    }
    // Also assign to Domain until deprecated so reverse migration is required
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_fpProtection = NSNumber(booleanLiteral: isEnabled)
    DataController.performOnMainContext { context in
      try? context.save()
    }
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  public func isShieldExpected(
    for url: URL?,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return false }
    if isBraveShieldsContentSettingsEnabled {
      if considerAllShieldsOption && !isBraveShieldsEnabled(for: url) {
        // Shields is disabled for this url
        return false
      }
      switch shield {
      case .allOff:
        return braveShieldsSettings.isBraveShieldsEnabled(for: url)
      case .fpProtection:
        return braveShieldsSettings.fingerprintMode(for: url) == .standardMode
      case .noScript:
        return braveShieldsSettings.isBlockScriptsEnabled(for: url)
      }
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    return domain.isShieldExpected(shield, considerAllShieldsOption: considerAllShieldsOption)
  }

  public func shredLevel(
    for url: URL?
  ) -> SiteShredLevel {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return .never }
    // TODO: Support AutoShred via content settings brave-browser#47753
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    return domain.shredLevel
  }

  public func setShredLevel(
    _ shredLevel: SiteShredLevel,
    for url: URL?
  ) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    // TODO: Support AutoShred via content settings brave-browser#47753
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shredLevel = shredLevel
    DataController.performOnMainContext { context in
      try? context.save()
    }
  }
}
