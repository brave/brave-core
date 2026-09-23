// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import TestHelpers
import Web
import XCTest

@testable import BraveShields
@testable import Data

@MainActor
class BraveShieldsSettingsMigrationTests: CoreDataTestCase {

  let shieldsEnabledURL = URL(string: "https://shields-enabled.com")!
  let adBlockModeURL = URL(string: "https://ad-block-mode.com")!
  let blockScriptsURL = URL(string: "https://block-scripts.com.com")!
  let fingerprintProtectionURL = URL(string: "https://fingerprint-protection.com")!
  let autoShredModeURL = URL(string: "https://auto-shred-mode.com")!

  /// Test that the current global / default settings are mapped to the
  /// respective `BraveShieldsSettings` respective `set*` methods.
  func testGlobalMigrations() {
    // setup initial values
    Preferences.Shields.blockAdsAndTrackingLevel = .aggressive
    Preferences.Shields.blockScripts.value = true
    Preferences.Shields.fingerprintingProtection.value = false
    Preferences.Shields.shredLevel = .whenSiteClosed

    // verify expected migrations
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._setDefaultAdBlockMode = { adBlockMode in
      XCTAssertEqual(adBlockMode, .aggressive)
    }
    testBraveShieldsSettings._setBlockScriptsEnabledByDefault = { enabled in
      XCTAssertTrue(enabled)
    }
    testBraveShieldsSettings._setDefaultFingerprintMode = { fingerprintMode in
      XCTAssertEqual(fingerprintMode, .allowMode)
    }
    testBraveShieldsSettings._setDefaultAutoShredMode = { autoShredMode in
      XCTAssertEqual(autoShredMode, .lastTabClosed)
    }
    testBraveShieldsSettings.migrateGlobalSettings()
  }

  /// Verify the basic migrations from `Domain` are mapped to
  /// `BraveShieldsSettings` respective `set*` methods.
  func testSiteSpecificMigration() {
    // setup initial values
    let shieldsEnabledDomain = Domain.getOrCreate(forUrl: shieldsEnabledURL, persistent: true)
    shieldsEnabledDomain.shield_allOff = NSNumber(booleanLiteral: true)
    let adBlockModeDomain = Domain.getOrCreate(forUrl: adBlockModeURL, persistent: true)
    adBlockModeDomain.domainBlockAdsAndTrackingLevel = .aggressive
    let blockScriptsDomain = Domain.getOrCreate(forUrl: blockScriptsURL, persistent: true)
    blockScriptsDomain.shield_noScript = NSNumber(booleanLiteral: true)
    let fingerprintProtectionDomain = Domain.getOrCreate(
      forUrl: fingerprintProtectionURL,
      persistent: true
    )
    fingerprintProtectionDomain.shield_fpProtection = NSNumber(booleanLiteral: false)
    let autoShredModeDomain = Domain.getOrCreate(forUrl: fingerprintProtectionURL, persistent: true)
    autoShredModeDomain.shredLevel = .whenSiteClosed

    // verify expected migrations
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    var setShieldsEnabledURLs = [URL]()
    testBraveShieldsSettings._setBraveShieldsEnabled = { enabled, url in
      XCTAssertFalse(enabled)
      setShieldsEnabledURLs.append(url)
    }
    var setAdBlockModeURLs = [URL]()
    testBraveShieldsSettings._setAdBlockMode = { adBlockMode, url in
      XCTAssertEqual(adBlockMode, .aggressive)
      setAdBlockModeURLs.append(url)
    }
    var setBlockScriptsURLs = [URL]()
    testBraveShieldsSettings._setBlockScriptsEnabled = { enabled, url in
      XCTAssertTrue(enabled)
      setBlockScriptsURLs.append(url)
    }
    var setFingerprintModeURLs = [URL]()
    testBraveShieldsSettings._setFingerprintMode = { fingerprintMode, url in
      XCTAssertEqual(fingerprintMode, .allowMode)
      setFingerprintModeURLs.append(url)
    }
    testBraveShieldsSettings._setAutoShredMode = { autoShredMode, url in
      XCTAssertEqual(autoShredMode, .lastTabClosed)
      XCTAssertEqual(url.domainURL.absoluteString, autoShredModeDomain.url ?? "")
    }

    let allDomains = Domain.allDomainsWithExlicitShieldSettings()
    testBraveShieldsSettings.migrateShieldsToContentSettings(for: allDomains)

    // verify `set*` was called for each of the subdomainURLsForMigration
    XCTAssertEqual(
      setShieldsEnabledURLs,
      [shieldsEnabledURL] + shieldsEnabledURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setAdBlockModeURLs,
      [adBlockModeURL] + adBlockModeURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setBlockScriptsURLs,
      [blockScriptsURL] + blockScriptsURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setFingerprintModeURLs,
      [fingerprintProtectionURL] + fingerprintProtectionURL.subdomainURLsForMigration()
    )
  }

  /// Some shields settings are stored on 2 different CoreData `Domain` models,
  /// but would map to only 1 Content Setting pattern due to using the domain
  /// pattern.
  /// For example, Auto Shred Mode would be stored twice for
  ///  `account.brave.com` & `profile.brave.com` with CoreData, but both would
  ///  be stored on `brave.com` for Content Settings.
  func testSiteSpecificMigrationDomainPattern() {
    // these 3 URLs are all treated as the same for AutoShred (domain pattern)
    let autoShredAppExitURL = URL(string: "https://a.brave.com")!
    let autoShredNeverURL = URL(string: "https://b.brave.com")!
    let autoShredDefaultURL = URL(string: "https://c.brave.com")!

    let autoShredAppExitDomain = Domain.getOrCreate(
      forUrl: autoShredAppExitURL,
      persistent: true
    )
    autoShredAppExitDomain.shredLevel = .appExit
    let autoShredNeverDomain = Domain.getOrCreate(
      forUrl: autoShredNeverURL,
      persistent: true
    )
    autoShredNeverDomain.shredLevel = .whenSiteClosed
    _ = Domain.getOrCreate(
      forUrl: autoShredDefaultURL,
      persistent: true
    )

    // verify expected migrations
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._setAutoShredMode = { autoShredMode, url in
      // sorted alphabetically, so expecting a.brave.com to take precedence.
      XCTAssertEqual(autoShredMode, .appExit)
      XCTAssertEqual(url.domainURL.absoluteString, autoShredAppExitDomain.url ?? "")
    }

    let allDomains = Domain.allDomainsWithExlicitShieldSettings()
    testBraveShieldsSettings.migrateShieldsToContentSettings(for: allDomains)
  }

  /// Verify that secure Domain setting is preferred over insecure.
  /// `Domain` models will be created for both secure & insecure if user
  /// doesn't enter a scheme when submitting a URL. Howver, shields content
  /// settings treat secure & insecure as the same. HTTPS upgrades are enabled
  /// by default, so users are more likely to land on a secure domain and have
  /// changed Shield settings on the secure version.
  func testSiteSpecificMigrationHttpsPreferred() {
    // setup initial values
    let shieldsEnabledDomain = Domain.getOrCreate(forUrl: shieldsEnabledURL, persistent: true)
    shieldsEnabledDomain.shield_allOff = NSNumber(booleanLiteral: true)
    let insecureShieldsEnabledDomain = Domain.getOrCreate(
      forUrl: shieldsEnabledURL.insecureURL,
      persistent: true
    )
    insecureShieldsEnabledDomain.shield_allOff = NSNumber(booleanLiteral: false)

    let adBlockModeDomain = Domain.getOrCreate(forUrl: adBlockModeURL, persistent: true)
    adBlockModeDomain.domainBlockAdsAndTrackingLevel = .aggressive
    let insecureAdBlockModeDomain = Domain.getOrCreate(
      forUrl: adBlockModeURL.insecureURL,
      persistent: true
    )
    insecureAdBlockModeDomain.domainBlockAdsAndTrackingLevel = .disabled

    let blockScriptsDomain = Domain.getOrCreate(forUrl: blockScriptsURL, persistent: true)
    blockScriptsDomain.shield_noScript = NSNumber(booleanLiteral: true)
    let insecureBlockScriptsDomain = Domain.getOrCreate(
      forUrl: blockScriptsURL.insecureURL,
      persistent: true
    )
    insecureBlockScriptsDomain.shield_noScript = NSNumber(booleanLiteral: false)

    let fingerprintProtectionDomain = Domain.getOrCreate(
      forUrl: fingerprintProtectionURL,
      persistent: true
    )
    fingerprintProtectionDomain.shield_fpProtection = NSNumber(booleanLiteral: false)
    let insecureFingerprintProtectionDomain = Domain.getOrCreate(
      forUrl: fingerprintProtectionURL.insecureURL,
      persistent: true
    )
    insecureFingerprintProtectionDomain.shield_fpProtection = NSNumber(booleanLiteral: true)

    let autoShredModeDomain = Domain.getOrCreate(forUrl: fingerprintProtectionURL, persistent: true)
    autoShredModeDomain.shredLevel = .whenSiteClosed
    let insecureAutoShredModeDomain = Domain.getOrCreate(
      forUrl: fingerprintProtectionURL.insecureURL,
      persistent: true
    )
    insecureAutoShredModeDomain.shredLevel = .appExit

    // verify expected migrations (https preferred)
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    var setShieldsEnabledURLs = [URL]()
    testBraveShieldsSettings._setBraveShieldsEnabled = { enabled, url in
      XCTAssertFalse(enabled)
      XCTAssertTrue(url.absoluteString.hasPrefix("https://"))
      setShieldsEnabledURLs.append(url)
    }
    var setAdBlockModeURLs = [URL]()
    testBraveShieldsSettings._setAdBlockMode = { adBlockMode, url in
      XCTAssertEqual(adBlockMode, .aggressive)
      XCTAssertTrue(url.absoluteString.hasPrefix("https://"))
      setAdBlockModeURLs.append(url)
    }
    var setBlockScriptsURLs = [URL]()
    testBraveShieldsSettings._setBlockScriptsEnabled = { enabled, url in
      XCTAssertTrue(enabled)
      XCTAssertTrue(url.absoluteString.hasPrefix("https://"))
      setBlockScriptsURLs.append(url)
    }
    var setFingerprintModeURLs = [URL]()
    testBraveShieldsSettings._setFingerprintMode = { fingerprintMode, url in
      XCTAssertEqual(fingerprintMode, .allowMode)
      XCTAssertTrue(url.absoluteString.hasPrefix("https://"))
      setFingerprintModeURLs.append(url)
    }
    testBraveShieldsSettings._setAutoShredMode = { autoShredMode, url in
      XCTAssertEqual(autoShredMode, .lastTabClosed)
      XCTAssertTrue(url.absoluteString.hasPrefix("https://"))
      XCTAssertEqual(url.domainURL.absoluteString, autoShredModeDomain.url ?? "")
    }

    let allDomains = Domain.allDomainsWithExlicitShieldSettings()
    testBraveShieldsSettings.migrateShieldsToContentSettings(for: allDomains)

    // verify `set*` was called for each of the subdomainURLsForMigration()
    XCTAssertEqual(
      setShieldsEnabledURLs,
      [shieldsEnabledURL] + shieldsEnabledURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setAdBlockModeURLs,
      [adBlockModeURL] + adBlockModeURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setBlockScriptsURLs,
      [blockScriptsURL] + blockScriptsURL.subdomainURLsForMigration()
    )
    XCTAssertEqual(
      setFingerprintModeURLs,
      [fingerprintProtectionURL] + fingerprintProtectionURL.subdomainURLsForMigration()
    )
  }
}

extension URL {
  fileprivate var insecureURL: URL {
    var components = URLComponents(url: self, resolvingAgainstBaseURL: false)!
    components.scheme = "http"
    return components.url ?? self
  }
}
