// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences

extension BraveShieldsSettings {
  /// Returns the `AutoShredMode` for the given url, optionally checking if
  /// Shields is disabled on any host that matches the domain pattern.
  /// For example, this could occur when Shields is disabled on `one.brave.com`,
  /// but enabled on `two.brave.com` and Auto Shred default is set to app exit.
  /// - parameter url: The URL to fetch the `AutoShredMode` for.
  /// - parameter considerAllShieldsOption: Flag to determine if we check if
  /// Shields is disabled on any host matching the domain pattern.
  /// - returns: The `AutoShredMode` for the given URL.
  public func autoShredMode(
    for url: URL?,
    considerAllShieldsOption: Bool
  ) -> BraveShields.AutoShredMode {
    if considerAllShieldsOption, isShieldsDisabledOnAnyHostMatchingDomain(of: url) {
      return .never
    }
    return autoShredMode(for: url)
  }

  // MARK: Migration

  @MainActor public func migrateGlobalSettings() {
    self.defaultAdBlockMode = Preferences.Shields.blockAdsAndTrackingLevel.adBlockMode
    self.defaultFingerprintMode =
      Preferences.Shields.fingerprintingProtection.value ? .standardMode : .allowMode
    self.isBlockScriptsEnabledByDefault = Preferences.Shields.blockScripts.value
    self.defaultAutoShredMode = Preferences.Shields.shredLevel.autoShredMode
  }

  @MainActor public func migrateShieldsToContentSettings(for domains: [Domain]) {
    // First, migrate shield settings that use host pattern.
    // Shields enabled, AdBlockMode, Block Scripts, Block Fingerprinting all
    // use host pattern to store content settings. This differs from `Domain`
    // which considers the scheme in the key (`domainURL`). As such, we may
    // have 2 `Domain` models that point to 1 host in content settings, for
    // example http://account.brave.com & https://accounts.brave.com.
    // However, `Domain` will consider https://www.brave.com the same as
    // https://brave.com, where content settings will not.
    let groupedByHost: [String: [Domain]] = Dictionary(grouping: domains) { domain in
      guard let urlString = domain.url, let url = URL(string: urlString) else { return "" }
      return hostPatternFromURL(url)
    }
    for (host, domains) in groupedByHost {
      // if no url, host will be an empty string. Shouldn't occur so we can ignore.
      guard !host.isEmpty else { continue }

      // priority for migrations:
      // 1) secure domain with setting explicitly enabled/disabled by user
      // 2) insecure domain with setting explicitly enabled/disabled by user
      // 3) default value (doesn't need assigned)

      // Shields Enabled / Disabled
      let domainsWithExplicitAllOff = domains.filter { $0.shield_allOff != nil }
      let shieldEnabledDomainToMigrate = domainsWithExplicitAllOff.sortedForMigration().first
      // only assign if an explicit value assigned, else default is used
      if let shieldEnabledDomainToMigrate,
        let urlString = shieldEnabledDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let shieldsEnabled: Bool = !shieldEnabledDomainToMigrate.areAllShieldsOff
        setBraveShieldsEnabled(shieldsEnabled, for: urlToMigrate)
      }

      // ShieldLevel / AdBlockMode
      let domainsWithExplicitShieldLevel = domains.filter {
        $0.shield_blockAdsAndTrackingLevel != nil
      }
      let shieldLevelDomainToMigrate = domainsWithExplicitShieldLevel.sortedForMigration().first
      // only assign if an explicit value assigned, else default is used
      if let shieldLevelDomainToMigrate,
        let urlString = shieldLevelDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let shieldLevel: ShieldLevel = shieldLevelDomainToMigrate.domainBlockAdsAndTrackingLevel
        setAdBlockMode(shieldLevel.adBlockMode, for: urlToMigrate)
      }

      // Block Fingerprinting
      let domainsWithExplicitFingerprintingProtection = domains.filter {
        $0.shield_fpProtection != nil
      }
      let fingerprintingProtectionDomainToMigrate =
        domainsWithExplicitFingerprintingProtection.sortedForMigration().first
      // only assign if an explicit value assigned, else default is used
      if let fingerprintingProtectionDomainToMigrate,
        let urlString = fingerprintingProtectionDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let fingerPrintingProtection: Bool =
          fingerprintingProtectionDomainToMigrate.isShieldExpected(
            .fpProtection,
            considerAllShieldsOption: false
          )
        setFingerprintMode(fingerPrintingProtection ? .standardMode : .allowMode, for: urlToMigrate)
      }

      // Block Scripts
      let domainsWithExplicitBlockScripts = domains.filter { $0.shield_noScript != nil }
      let blockScriptsDomainToMigrate = domainsWithExplicitBlockScripts.sortedForMigration().first
      // only assign if an explicit value assigned, else default is used
      if let blockScriptsDomainToMigrate,
        let urlString = blockScriptsDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let blockScripts: Bool = blockScriptsDomainToMigrate.isShieldExpected(
          .noScript,
          considerAllShieldsOption: false
        )
        setBlockScriptsEnabled(blockScripts, for: urlToMigrate)
      }
    }

    // Second, migrate shield settings that use domain pattern.
    // Auto-Shred level uses domain pattern to store content settings.
    // This differs from `Domain` which includes the scheme in the key
    // (`domainURL`). As such, we may have 2 `Domain` models that point
    // to 1 domain in content settings, for example
    // https://brave.com & https://accounts.brave.com.
    let groupedByDomain: [String: [Domain]] = Dictionary(grouping: domains) { domain in
      guard let urlString = domain.url, let url = URL(string: urlString) else { return "" }
      return domainPatternFromURL(url)
    }
    for (domain, domains) in groupedByDomain {
      // if no url, domain will be an empty string. Shouldn't occur so we can ignore.
      guard !domain.isEmpty else { continue }

      let domainsWithExplicitShredLevel = domains.filter { $0.shield_shredLevel != nil }
      let shredLevelDomainToMigrate = domainsWithExplicitShredLevel.sortedForMigration().first
      // only assign if an explicit value assigned, else default is used
      if let shredLevelDomainToMigrate,
        let urlString = shredLevelDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let autoShredMode = shredLevelDomainToMigrate.shredLevel.autoShredMode
        self.setAutoShredMode(autoShredMode, for: urlToMigrate)
      }
    }
  }

  private func hostPatternFromURL(_ url: URL) -> String {
    return (url as NSURL).hostPatternString
  }

  private func domainPatternFromURL(_ url: URL) -> String {
    return (url as NSURL).domainPatternString
  }
}

extension Array where Element == Domain {
  fileprivate func sortedForMigration() -> [Domain] {
    sorted(by: { lhs, rhs in
      let isLHSHttps = lhs.url?.hasPrefix("https://") == true
      let isRHSHttps = rhs.url?.hasPrefix("https://") == true
      // prioritize https
      if isLHSHttps && !isRHSHttps {
        return true
      } else if isRHSHttps && !isLHSHttps {
        return false
      }
      // otherwise alphabetical sort
      return lhs.url?.localizedCaseInsensitiveCompare(rhs.url ?? "") == .orderedAscending
    })
  }
}
