// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Preferences

// https://github.com/brave/brave-ios/issues/7611
/// A list of etld+1s that are always aggressive
private let alwaysAggressiveETLDs: Set<String> = ["youtube.com"]

extension Domain {
  @MainActor var areAllShieldsOff: Bool {
    return shield_allOff?.boolValue ?? false
  }

  /// The shred level for this current domain
  ///
  /// When getting, it will return the global shred level if the domain level is not set
  @MainActor public var shredLevel: SiteShredLevel {
    get {
      guard let shredLevel = self.shield_shredLevel else {
        return ShieldPreferences.shredLevel
      }
      return SiteShredLevel(rawValue: shredLevel) ?? ShieldPreferences.shredLevel
    }

    set {
      shield_shredLevel = newValue.rawValue
    }
  }

  /// The shield level for this current domain (standard/aggressive/disabled).
  @MainActor var domainBlockAdsAndTrackingLevel: ShieldLevel {
    get {
      guard let level = self.shield_blockAdsAndTrackingLevel else {
        return ShieldPreferences.blockAdsAndTrackingLevel
      }
      return ShieldLevel(rawValue: level) ?? ShieldPreferences.blockAdsAndTrackingLevel
    }

    set {
      shield_blockAdsAndTrackingLevel = newValue.rawValue
    }
  }

  /// Moves data from the old `shield_adblockAndTp` to the new `shield_blockAdsAndTrackingLevel`
  @MainActor public func migrateShieldLevel() {
    guard let isEnabled = shield_adblockAndTp?.boolValue else { return }
    domainBlockAdsAndTrackingLevel = isEnabled ? .standard : .disabled
  }

  /// Return the shield level for this domain.
  ///
  /// - Warning: This does not consider the "all off" setting
  /// This also takes into consideration certain domains that are always aggressive.
  @MainActor var globalBlockAdsAndTrackingLevel: ShieldLevel {
    guard !areAllShieldsOff else { return .disabled }
    let globalLevel = domainBlockAdsAndTrackingLevel

    switch globalLevel {
    case .standard:
      guard let urlString = self.url else { return globalLevel }
      guard let url = URL(string: urlString) else { return globalLevel }
      guard let etldPlusOne = url.baseDomain else { return globalLevel }

      if alwaysAggressiveETLDs.contains(etldPlusOne) {
        return .aggressive
      } else {
        return globalLevel
      }
    case .disabled, .aggressive:
      return globalLevel
    }
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  @MainActor func isShieldExpected(
    _ shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    let isShieldOn =
      switch shield {
      case .allOff:
        self.shield_allOff?.boolValue ?? false
      case .fpProtection:
        self.shield_fpProtection?.boolValue
          ?? Preferences.Shields.fingerprintingProtection.value
      case .noScript:
        self.shield_noScript?.boolValue ?? Preferences.Shields.blockScripts.value
      }

    let isAllShieldsOff = self.shield_allOff?.boolValue ?? false
    let isSpecificShieldOn = isShieldOn
    return considerAllShieldsOption ? !isAllShieldsOff && isSpecificShieldOn : isSpecificShieldOn
  }

  @MainActor public class func totalDomainsWithAdblockShieldsLoweredFromGlobal() -> Int {
    guard ShieldPreferences.blockAdsAndTrackingLevel.isEnabled,
      let domains = Domain.allDomainsWithExplicitShieldLevel()
    else {
      return 0  // Can't be lower than disabled
    }

    return domains.filter({
      $0.domainBlockAdsAndTrackingLevel.strength
        < ShieldPreferences.blockAdsAndTrackingLevel.strength
    }).count
  }

  @MainActor public class func totalDomainsWithAdblockShieldsIncreasedFromGlobal() -> Int {
    guard ShieldPreferences.blockAdsAndTrackingLevel != .aggressive,
      let domains = Domain.allDomainsWithExplicitShieldLevel()
    else {
      return 0  // Can't be higher than aggressive
    }
    return domains.filter({
      $0.domainBlockAdsAndTrackingLevel.strength
        > ShieldPreferences.blockAdsAndTrackingLevel.strength
    }).count
  }

  @MainActor public class func allDomainsWithShredLevelAppExit() -> [Domain]? {
    let allExplicitlySet = Domain.allDomainsWithAutoShredLevel(SiteShredLevel.appExit.rawValue)
    guard ShieldPreferences.shredLevel.shredOnAppExit else { return allExplicitlySet }
    // Default value is SiteShredLevel.appExit, include all with default value nil
    return (allExplicitlySet ?? []) + (Domain.allDomainsWithAutoShredLevel("nil") ?? [])
  }
}
