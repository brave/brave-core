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
    // if no url, host will be an empty string. Shouldn't occur so we can ignore.
    for (host, domains) in groupedByHost where !host.isEmpty {

      // priority for migrations:
      // 1) secure domain with setting explicitly enabled/disabled by user
      // 2) insecure domain with setting explicitly enabled/disabled by user
      // 3) default value (doesn't need assigned)

      // Shields Enabled / Disabled
      let domainsWithExplicitAllOff = domains.filter { $0.shield_allOff != nil }
      let shieldEnabledDomainToMigrate = domainsWithExplicitAllOff.preferredForMigration()
      // only assign if an explicit value assigned, else default is used
      if let shieldEnabledDomainToMigrate,
        let urlString = shieldEnabledDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let shieldsEnabled: Bool = !shieldEnabledDomainToMigrate.areAllShieldsOff
        setBraveShieldsEnabled(shieldsEnabled, for: urlToMigrate)
        for url in urlToMigrate.subdomainURLsForMigration() {
          setBraveShieldsEnabled(shieldsEnabled, for: url)
        }
      }

      // ShieldLevel / AdBlockMode
      let domainsWithExplicitShieldLevel = domains.filter {
        $0.shield_blockAdsAndTrackingLevel != nil
      }
      let shieldLevelDomainToMigrate = domainsWithExplicitShieldLevel.preferredForMigration()
      // only assign if an explicit value assigned, else default is used
      if let shieldLevelDomainToMigrate,
        let urlString = shieldLevelDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let shieldLevel: ShieldLevel = shieldLevelDomainToMigrate.domainBlockAdsAndTrackingLevel
        setAdBlockMode(shieldLevel.adBlockMode, for: urlToMigrate)
        for url in urlToMigrate.subdomainURLsForMigration() {
          setAdBlockMode(shieldLevel.adBlockMode, for: url)
        }
      }

      // Block Fingerprinting
      let domainsWithExplicitFingerprintingProtection = domains.filter {
        $0.shield_fpProtection != nil
      }
      let fingerprintingProtectionDomainToMigrate =
        domainsWithExplicitFingerprintingProtection.preferredForMigration()
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
        for url in urlToMigrate.subdomainURLsForMigration() {
          setFingerprintMode(fingerPrintingProtection ? .standardMode : .allowMode, for: url)
        }
      }

      // Block Scripts
      let domainsWithExplicitBlockScripts = domains.filter { $0.shield_noScript != nil }
      let blockScriptsDomainToMigrate = domainsWithExplicitBlockScripts.preferredForMigration()
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
        for url in urlToMigrate.subdomainURLsForMigration() {
          setBlockScriptsEnabled(blockScripts, for: url)
        }
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
    // if no url, domain will be an empty string. Shouldn't occur so we can ignore.
    for (domain, domains) in groupedByDomain where !domain.isEmpty {

      let domainsWithExplicitShredLevel = domains.filter { $0.shield_shredLevel != nil }
      let shredLevelDomainToMigrate = domainsWithExplicitShredLevel.preferredForMigration()
      // only assign if an explicit value assigned, else default is used
      if let shredLevelDomainToMigrate,
        let urlString = shredLevelDomainToMigrate.url,
        let urlToMigrate = URL(string: urlString)
      {
        let autoShredMode = shredLevelDomainToMigrate.shredLevel.autoShredMode
        self.setAutoShredMode(autoShredMode, for: urlToMigrate)
        // subdomainURLsForMigration() is not neded for Auto Shred because the
        // domain pattern used for AutoShredMode removes the subdomains.
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
  fileprivate func preferredForMigration() -> Domain? {
    self.min(by: { lhs, rhs in
      let isLHSHttps = lhs.url?.caseInsensitiveHasPrefix("https://") == true
      let isRHSHttps = rhs.url?.caseInsensitiveHasPrefix("https://") == true
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

extension URL {

  /// Adds the given subdomain if it's not already the prefix of the host.
  private func addingSubdomainIfNeeded(subdomain: String) -> URL? {
    guard let host,
      host.hasPrefix(subdomain + ".") == false,
      var components = URLComponents(url: self, resolvingAgainstBaseURL: false)
    else { return nil }
    components.host = subdomain + "." + host
    return components.url
  }

  /// The subdomain URLs we should migrate to for the current URL.
  /// On iOS, the `domainURL` used for `Domain` model strips `www|m|mobile`
  /// from the `URL`, so these subdomains are treated as the same, but with
  /// content settings they are treated separately. For migration, we migrate
  /// to these trivial subdomains so there is no perceived data loss (of site-
  /// specific Shield setings) to the user.
  /// - returns: an array of URLs with `www|m|mobile` subdomain added to the
  /// host (if needed).
  func subdomainURLsForMigration() -> [URL] {
    [
      addingSubdomainIfNeeded(subdomain: "www"),
      addingSubdomainIfNeeded(subdomain: "m"),
      addingSubdomainIfNeeded(subdomain: "mobile"),
    ].compactMap { $0 }
  }
}

extension String {
  fileprivate func caseInsensitiveHasPrefix(_ prefix: String) -> Bool {
    range(of: prefix, options: [.anchored, .caseInsensitive]) != nil
  }
}
