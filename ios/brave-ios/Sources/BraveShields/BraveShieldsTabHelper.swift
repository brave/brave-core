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
  private let braveShieldsUtils: any BraveShieldsUtils

  public init(
    tab: some TabState,
    braveShieldsUtils: any BraveShieldsUtils
  ) {
    self.tab = tab
    self.braveShieldsUtils = braveShieldsUtils
  }

  public func isBraveShieldsEnabled(for url: URL?) -> Bool {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return false }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      return braveShieldsUtils.isBraveShieldsEnabled(for: url, isPrivate: isPrivate)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    return !domain.areAllShieldsOff
  }

  public func setBraveShieldsEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBraveShieldsEnabled(isEnabled, for: url, isPrivate: isPrivate)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_allOff = NSNumber(booleanLiteral: !isEnabled)
  }

  public func shieldLevel(for url: URL?, considerAllShieldsOption: Bool) -> ShieldLevel {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return .disabled }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      return braveShieldsUtils.adBlockMode(for: url, isPrivate: isPrivate).shieldLevel
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
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setAdBlockMode(shieldLevel.adBlockMode, for: url, isPrivate: isPrivate)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.domainBlockAdsAndTrackingLevel = shieldLevel
  }

  public func setBlockScriptsEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBlockScriptsEnabled(isEnabled, for: url, isPrivate: isPrivate)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_noScript = NSNumber(booleanLiteral: isEnabled)
  }

  public func setBlockFingerprintingEnabled(_ isEnabled: Bool, for url: URL?) {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBlockFingerprintingEnabled(isEnabled, for: url, isPrivate: isPrivate)
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shield_fpProtection = NSNumber(booleanLiteral: isEnabled)
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  public func isShieldExpected(
    for url: URL?,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    guard let url = url ?? tab?.visibleURL, let isPrivate = tab?.isPrivate else { return false }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      if considerAllShieldsOption && !isBraveShieldsEnabled(for: url) {
        // Shields is disabled for this url
        return false
      }
      switch shield {
      case .allOff:
        return braveShieldsUtils.isBraveShieldsEnabled(for: url, isPrivate: isPrivate)
      case .fpProtection:
        return braveShieldsUtils.isBlockFingerprintingEnabled(for: url, isPrivate: isPrivate)
      case .noScript:
        return braveShieldsUtils.isBlockScriptsEnabled(for: url, isPrivate: isPrivate)
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
  }
}
