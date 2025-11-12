// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Preferences

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
        return Preferences.Shields.shredLevel
      }
      return SiteShredLevel(rawValue: shredLevel) ?? Preferences.Shields.shredLevel
    }

    set {
      shield_shredLevel = newValue.rawValue
    }
  }

  /// The shield level for this current domain (standard/aggressive/disabled).
  @MainActor var domainBlockAdsAndTrackingLevel: ShieldLevel {
    get {
      guard let level = self.shield_blockAdsAndTrackingLevel else {
        return Preferences.Shields.blockAdsAndTrackingLevel
      }
      return ShieldLevel(rawValue: level) ?? Preferences.Shields.blockAdsAndTrackingLevel
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

  /// Return the shield level for this domain, taking into account if all
  /// shields are disabled for this domain.
  @MainActor var globalBlockAdsAndTrackingLevel: ShieldLevel {
    guard !areAllShieldsOff else { return .disabled }
    return domainBlockAdsAndTrackingLevel
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
    guard Preferences.Shields.blockAdsAndTrackingLevel.isEnabled,
      let domains = Domain.allDomainsWithExplicitShieldLevel()
    else {
      return 0  // Can't be lower than disabled
    }

    return domains.filter({
      $0.domainBlockAdsAndTrackingLevel.strength
        < Preferences.Shields.blockAdsAndTrackingLevel.strength
    }).count
  }

  @MainActor public class func totalDomainsWithAdblockShieldsIncreasedFromGlobal() -> Int {
    guard Preferences.Shields.blockAdsAndTrackingLevel != .aggressive,
      let domains = Domain.allDomainsWithExplicitShieldLevel()
    else {
      return 0  // Can't be higher than aggressive
    }
    return domains.filter({
      $0.domainBlockAdsAndTrackingLevel.strength
        > Preferences.Shields.blockAdsAndTrackingLevel.strength
    }).count
  }

  public class func allDomainsWithShredLevelAppExit() -> [Domain] {
    let domainsWithExplicitAppExit =
      Domain.allDomainsWithAutoShredLevel(SiteShredLevel.appExit.rawValue) ?? []
    guard Preferences.Shields.shredLevel.shredOnAppExit else { return domainsWithExplicitAppExit }
    // Default value is SiteShredLevel.appExit, so fetch Domain's using default value
    let domainsWithDefaultShredLevel = Domain.allDomainsWithAutoShredLevel(nil) ?? []

    // A Domain may be created with non-secure scheme, then a separate Domain
    // may be created with the secure scheme after https upgrade.
    // WKWebsiteDataStore stores data without a scheme, so we should remove
    // duplicate Domain's from the domainsWithDefaultShredLevel if the user explicitly
    // assigned a shred level on it's baseDomain.
    guard let domainsWithExplicitShredLevel = Domain.allDomainsWithExplicitShredLevel(),
      !domainsWithExplicitShredLevel.isEmpty
    else {
      // no domains with explicit assigned shred level assigned,
      // so we can return all without shred level assigned
      return domainsWithExplicitAppExit + domainsWithDefaultShredLevel
    }
    let baseDomainsToExclude: Set<String> = Set(
      domainsWithExplicitShredLevel.compactMap({
        guard let urlString = $0.url, let url = URL(string: urlString) else {
          // if no url, not usable for shred
          return nil
        }
        return url.baseDomain
      })
    )
    let domainsWithDefaultShredLevelFiltered =
      domainsWithDefaultShredLevel
      .filter { domain in
        guard let urlString = domain.url, let url = URL(string: urlString),
          let baseDomain = url.baseDomain
        else {
          // if no url, not usable for shred
          return false
        }
        // exclude domain if on exclusion list
        return !baseDomainsToExclude.contains(baseDomain)
      }

    return domainsWithExplicitAppExit + domainsWithDefaultShredLevelFiltered
  }
}
