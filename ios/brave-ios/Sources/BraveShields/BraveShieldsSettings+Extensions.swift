// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences

// MARK: Migration

extension BraveShieldsSettings {

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
      if host == "" { continue }
      // Using groupedByHost, so each `Domain` in domains has the same host.
      guard let urlString = domains.first?.url, let url = URL(string: urlString) else { continue }

      // priority for migrations:
      // 1) secure domain with setting explicitly enabled/disabled by user
      // 2) insecure domain with setting explicitly enabled/disabled by user
      // 3) default value (doesn't need assigned)

      // Shields Enabled / Disabled
      let domainsWithExplicitAllOff = domains.filter { $0.shield_allOff != nil }
      let secureDomainsWithExplicitAllOff = domainsWithExplicitAllOff.secureDomains()
      let insecureDomainsWithExplicitAllOff = domainsWithExplicitAllOff.insecureDomains()
      let shieldEnabledDomainToMigrate =
        secureDomainsWithExplicitAllOff.first ?? insecureDomainsWithExplicitAllOff.first
      // only assign if an explicit value assigned, else default is used
      if let shieldEnabledDomainToMigrate {
        let shieldsEnabled: Bool = !shieldEnabledDomainToMigrate.areAllShieldsOff
        setBraveShieldsEnabled(shieldsEnabled, for: url)
      }

      // ShieldLevel / AdBlockMode
      let domainsWithExplicitShieldLevel = domains.filter {
        $0.shield_blockAdsAndTrackingLevel != nil
      }
      let secureDomainsWithExplicitShieldLevel = domainsWithExplicitShieldLevel.secureDomains()
      let insecureDomainsWithExplicitShieldLevel = domainsWithExplicitShieldLevel.insecureDomains()
      let shieldLevelDomainToMigrate =
        secureDomainsWithExplicitShieldLevel.first ?? insecureDomainsWithExplicitShieldLevel.first
      // only assign if an explicit value assigned, else default is used
      if let shieldLevelDomainToMigrate {
        let shieldLevel: ShieldLevel = shieldLevelDomainToMigrate.domainBlockAdsAndTrackingLevel
        setAdBlockMode(shieldLevel.adBlockMode, for: url)
      }

      // Block Fingerprinting
      let domainsWithExplicitFingerprintingProtection = domains.filter {
        $0.shield_fpProtection != nil
      }
      let secureDomainsWithExplicitFingerprintingProtection =
        domainsWithExplicitFingerprintingProtection.secureDomains()
      let insecureDomainsWithExplicitFingerprintingProtection =
        domainsWithExplicitFingerprintingProtection.insecureDomains()
      let fingerprintingProtectionDomainToMigrate =
        secureDomainsWithExplicitFingerprintingProtection.first
        ?? insecureDomainsWithExplicitFingerprintingProtection.first
      // only assign if an explicit value assigned, else default is used
      if let fingerprintingProtectionDomainToMigrate {
        let fingerPrintingProtection: Bool =
          fingerprintingProtectionDomainToMigrate.isShieldExpected(
            .fpProtection,
            considerAllShieldsOption: false
          )
        setFingerprintMode(fingerPrintingProtection ? .standardMode : .allowMode, for: url)
      }

      // Block Scripts
      let domainsWithExplicitBlockScripts = domains.filter { $0.shield_noScript != nil }
      let secureDomainsWithExplicitBlockScripts = domainsWithExplicitBlockScripts.secureDomains()
      let insecureDomainsWithExplicitBlockScripts =
        domainsWithExplicitBlockScripts.insecureDomains()
      let blockScriptsDomainToMigrate =
        secureDomainsWithExplicitBlockScripts.first ?? insecureDomainsWithExplicitBlockScripts.first
      // only assign if an explicit value assigned, else default is used
      if let blockScriptsDomainToMigrate {
        let blockScripts: Bool = blockScriptsDomainToMigrate.isShieldExpected(
          .noScript,
          considerAllShieldsOption: false
        )
        setBlockScriptsEnabled(blockScripts, for: url)
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
      // if no url, host will be an empty string. Shouldn't occur so we can ignore.
      if domain == "" { continue }
      // Using groupedByHost, so each `Domain` in domains has the same host.
      guard let urlString = domains.first?.url, let url = URL(string: urlString) else { continue }

      let domainsWithExplicitShredLevel = domains.filter { $0.shield_shredLevel != nil }
      let secureDomainsWithExplicitShredLevel = domainsWithExplicitShredLevel.secureDomains()
      let insecureDomainsWithExplicitShredLevel = domainsWithExplicitShredLevel.insecureDomains()
      // prefer https Shield setting
      let shredLevelDomainToMigrate =
        secureDomainsWithExplicitShredLevel.first ?? insecureDomainsWithExplicitShredLevel.first
      // only assign if an explicit value assigned, else default is used
      if let shredLevelDomainToMigrate {
        let autoShredMode = shredLevelDomainToMigrate.shredLevel.autoShredMode
        self.setAutoShredMode(autoShredMode, for: url)
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
  fileprivate func secureDomains() -> [Domain] {
    filter {
      $0.url?.hasPrefix("https://") ?? false
    }
  }
  fileprivate func insecureDomains() -> [Domain] {
    filter {
      $0.url?.hasPrefix("http://") ?? false
    }
  }
}
