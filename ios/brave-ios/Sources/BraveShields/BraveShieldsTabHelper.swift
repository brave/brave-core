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
  private let braveShieldsUtils: BraveShieldsUtilsIOS

  public init(
    tab: some TabState,
    braveShieldsUtils: BraveShieldsUtilsIOS
  ) {
    self.tab = tab
    self.braveShieldsUtils = braveShieldsUtils
  }

  public func isBraveShieldsEnabled(for url: URL?, isPrivate: Bool) -> Bool {
    guard let url = url ?? tab?.visibleURL else { return false }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      return braveShieldsUtils.isBraveShieldsEnabled(for: url, isPrivate: isPrivate)
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      return !domain.areAllShieldsOff
    }
  }

  public func setBraveShieldsEnabled(_ isEnabled: Bool, for url: URL?, isPrivate: Bool) {
    guard let url = url ?? tab?.visibleURL else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBraveShieldsEnabled(isEnabled, for: url, isPrivate: isPrivate)
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      domain.shield_allOff = NSNumber(booleanLiteral: !isEnabled)
    }
  }

  public func shieldLevel(
    for url: URL?,
    isPrivate: Bool,
    considerAllShieldsOption: Bool
  ) -> ShieldLevel {
    guard let url = url ?? tab?.visibleURL else { return .disabled }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      return braveShieldsUtils.adBlockMode(for: url, isPrivate: isPrivate).shieldLevel
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      if considerAllShieldsOption {
        return domain.globalBlockAdsAndTrackingLevel
      } else {
        return domain.domainBlockAdsAndTrackingLevel
      }
    }
  }

  public func setShieldLevel(_ shieldLevel: ShieldLevel, for url: URL?, isPrivate: Bool) {
    guard let url = url ?? tab?.visibleURL else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setAdBlockMode(shieldLevel.adBlockMode, for: url, isPrivate: isPrivate)
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      domain.domainBlockAdsAndTrackingLevel = shieldLevel
    }
  }

  public func setBlockScriptsEnabled(_ isEnabled: Bool, for url: URL?, isPrivate: Bool) {
    guard let url = url ?? tab?.visibleURL else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBlockScriptsEnabled(isEnabled, for: url, isPrivate: isPrivate)
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      domain.shield_noScript = NSNumber(booleanLiteral: isEnabled)
    }
  }

  public func setBlockFingerprintingEnabled(
    _ isEnabled: Bool,
    for url: URL?,
    isPrivate: Bool
  ) {
    guard let url = url ?? tab?.visibleURL else { return }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      braveShieldsUtils.setBlockFingerprintingEnabled(isEnabled, for: url, isPrivate: isPrivate)
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      domain.shield_fpProtection = NSNumber(booleanLiteral: isEnabled)
    }
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  public func isShieldExpected(
    for url: URL?,
    isPrivate: Bool,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    guard let url = url ?? tab?.visibleURL else { return false }
    if FeatureList.kBraveShieldsContentSettings.enabled {
      if considerAllShieldsOption && !isBraveShieldsEnabled(for: url, isPrivate: isPrivate) {
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
    } else {
      let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
      return domain.isShieldExpected(shield, considerAllShieldsOption: considerAllShieldsOption)
    }
  }

  public func shredLevel(
    for url: URL?,
    isPrivate: Bool
  ) -> SiteShredLevel {
    guard let url = url ?? tab?.visibleURL else { return .never }
    // TODO: Support AutoShred via content settings brave-browser#47753
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    return domain.shredLevel
  }

  public func setShredLevel(
    _ shredLevel: SiteShredLevel,
    for url: URL?,
    isPrivate: Bool
  ) {
    guard let url = url ?? tab?.visibleURL else { return }
    // TODO: Support AutoShred via content settings brave-browser#47753
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivate)
    domain.shredLevel = shredLevel
  }

  // MARK: Migration

  private func migrateShieldsToContentSettings() {
    guard FeatureList.kBraveShieldsContentSettings.enabled else { return }
    let domains: [Domain] = []
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
        self.setBraveShieldsEnabled(shieldsEnabled, for: url, isPrivate: false)
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
        self.setShieldLevel(shieldLevel, for: url, isPrivate: false)
      }

      // Block Fingerprinting
      let domainsWithExplicitBlockFingerprinting = domains.filter { $0.shield_fpProtection != nil }
      let secureDomainsWithExplicitBlockFingerprinting =
        domainsWithExplicitBlockFingerprinting.secureDomains()
      let insecureDomainsWithExplicitBlockFingerprinting =
        domainsWithExplicitBlockFingerprinting.insecureDomains()
      let blockFingerprintingDomainToMigrate =
        secureDomainsWithExplicitBlockFingerprinting.first
        ?? insecureDomainsWithExplicitBlockFingerprinting.first
      // only assign if an explicit value assigned, else default is used
      if let blockFingerprintingDomainToMigrate {
        let blockFingerprinting: Bool = blockFingerprintingDomainToMigrate.isShieldExpected(
          .fpProtection,
          considerAllShieldsOption: false
        )
        self.setBlockFingerprintingEnabled(blockFingerprinting, for: url, isPrivate: false)
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
        self.setBlockScriptsEnabled(blockScripts, for: url, isPrivate: false)
      }
    }

    // TODO: migrate Auto-Shred content settings brave-browser#47753
    // Second, migrate shield settings that use domain pattern.
    // Auto-Shred level uses domain pattern to store content settings.
    // This differs from `Domain` which includes the scheme in the key
    // (`domainURL`). As such, we may have 2 `Domain` models that point
    // to 1 domain in content settings, for example
    // https://brave.com & https://accounts.brave.com.
  }

  private func migrateShieldsToCoreData() {
    guard !FeatureList.kBraveShieldsContentSettings.enabled else { return }
  }

  private func hostPatternFromURL(_ url: URL) -> String {
    return ""
  }

  private func domainPatternFromURL(_ url: URL) -> String {
    return ""
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
