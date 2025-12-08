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
class BraveShieldsTabHelperTests: CoreDataTestCase {

  let url = URL(string: "https://brave.com")!

  override func setUp() {
    super.setUp()
    Preferences.Shields.blockScripts.reset()
    Preferences.Shields.fingerprintingProtection.reset()
    Preferences.Shields.blockAdsAndTrackingLevelRaw.reset()
    Preferences.Shields.shredLevelRaw.reset()
  }

  /// Test `isBraveShieldsEnabled(for:)` with
  /// `isBraveShieldsContentSettingsEnabled` flag disabled.
  func testIsBraveShieldsEnabled() {
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._isBraveShieldsEnabled = { _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
      return false
    }
    testBraveShieldsSettings._setBraveShieldsEnabled = { _, _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` disabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: false
    )

    // Verify initial values
    XCTAssertTrue(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertTrue(!domain.areAllShieldsOff)
    // Update value
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    }
    // Verify updated values
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertFalse(!domain.areAllShieldsOff)
  }

  /// Test `isBraveShieldsEnabled(for:)` with
  /// `isBraveShieldsContentSettingsEnabled` flag enabled.
  func testIsBraveShieldsEnabledContentSettings() {
    var isBraveShieldsEnabled = true

    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._isBraveShieldsEnabled = { url in
      XCTAssertEqual(url, self.url)
      return isBraveShieldsEnabled
    }
    testBraveShieldsSettings._setBraveShieldsEnabled = { enabled, url in
      XCTAssertEqual(url, self.url)
      XCTAssertFalse(enabled)
      isBraveShieldsEnabled = enabled
    }

    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` enabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: true
    )

    // Verify initial value
    XCTAssertTrue(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    // Update value
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    // Verify updated value
    XCTAssertFalse(isBraveShieldsEnabled)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
  }

  /// Test `shieldLevel(for:considerAllShieldsOption:)` with
  /// `isBraveShieldsContentSettingsEnabled` flag disabled.
  func testShieldLevel() {
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._adBlockMode = { _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
      return .standard
    }
    testBraveShieldsSettings._setAdBlockMode = { _, _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` disabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: false
    )

    // Verify initial values
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: false),
      .standard
    )
    XCTAssertNil(domain.shield_blockAdsAndTrackingLevel)
    // Update value
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setShieldLevel(.aggressive, for: url)
    }
    // Verify updated values
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: false),
      .aggressive
    )
    XCTAssertEqual(domain.shield_blockAdsAndTrackingLevel, ShieldLevel.aggressive.rawValue)
    // Verify `considerAllShieldsOption`
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: true),
      .disabled
    )

    // Verify `considerAlwaysAggressiveETLDs` is respected
    let alwaysAggressiveURL = URL(string: "https://m.youtube.com")!
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ),
      .standard
    )
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: true
      ),
      .aggressive
    )
    braveShieldsTabHelper.setShieldLevel(.disabled, for: alwaysAggressiveURL)
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: true
      ),
      .disabled
    )
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ),
      .disabled
    )
  }

  /// Test `shieldLevel(for:considerAllShieldsOption:)` with
  /// `isBraveShieldsContentSettingsEnabled` flag enabled.
  func testShieldLevelContentSettings() {
    var adBlockMode: BraveShields.AdBlockMode = .standard
    var isBraveShieldsEnabled = true

    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._adBlockMode = { url in
      return adBlockMode
    }
    testBraveShieldsSettings._setAdBlockMode = { mode, url in
      XCTAssertEqual(mode, .aggressive)
      adBlockMode = mode
    }
    testBraveShieldsSettings._isBraveShieldsEnabled = { _ in
      return isBraveShieldsEnabled
    }

    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` enabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: true
    )

    // Verify initial value
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: false),
      .standard
    )
    // Update value
    braveShieldsTabHelper.setShieldLevel(.aggressive, for: url)
    // Verify updated values
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: false),
      .aggressive
    )
    // Verify `considerAllShieldsOption`
    isBraveShieldsEnabled = false
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: true),
      .disabled
    )

    // Verify `considerAlwaysAggressiveETLDs` is respected
    let alwaysAggressiveURL = URL(string: "https://m.youtube.com")!
    // reset TestBraveShieldsSettings values
    isBraveShieldsEnabled = true
    adBlockMode = .standard
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ),
      .standard
    )
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: true
      ),
      .aggressive
    )
    adBlockMode = .allow  // equivalent to ShieldLevel.disabled
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: true
      ),
      .disabled
    )
    XCTAssertEqual(
      braveShieldsTabHelper.shieldLevel(
        for: alwaysAggressiveURL,
        considerAllShieldsOption: true,
        considerAlwaysAggressiveETLDs: false
      ),
      .disabled
    )
  }

  /// Test `isShieldExpected(for:shield:considerAllShieldsOption:)` for Block
  /// Scripts with `isBraveShieldsContentSettingsEnabled` flag disabled.
  func testBlockScriptsEnabled() {
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._isBlockScriptsEnabled = { _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
      return false
    }
    testBraveShieldsSettings._setBlockScriptsEnabled = { _, _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` disabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: false
    )

    // Verify initial values
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: false
      ),
      false
    )
    XCTAssertNil(domain.shield_noScript)
    // Update value
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setBlockScriptsEnabled(true, for: url)
    }
    // Verify updated values
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: false
      ),
      true
    )
    XCTAssertEqual(domain.shield_noScript, NSNumber(booleanLiteral: true))
    // Verify `considerAllShieldsOption`
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: true
      ),
      false
    )
  }

  /// Test `isShieldExpected(for:shield:considerAllShieldsOption:)` for Block
  /// Scripts with `isBraveShieldsContentSettingsEnabled` flag enabled.
  func testBlockScriptsEnabledContentSettings() {
    var isBlockScriptsEnabled = false

    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._isBlockScriptsEnabled = { url in
      XCTAssertEqual(url, self.url)
      return isBlockScriptsEnabled
    }
    testBraveShieldsSettings._setBlockScriptsEnabled = { enabled, url in
      XCTAssertEqual(url, self.url)
      XCTAssertTrue(enabled)
      isBlockScriptsEnabled = enabled
    }
    testBraveShieldsSettings._isBraveShieldsEnabled = { _ in
      // disabled so we can test `considerAllShieldsOption: true`
      return false
    }

    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` enabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: true
    )

    // Verify initial values
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: false
      ),
      false
    )
    // Update value
    braveShieldsTabHelper.setBlockScriptsEnabled(true, for: url)
    // Verify updated values
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: false
      ),
      true
    )
    // Verify `considerAllShieldsOption`
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertEqual(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .noScript,
        considerAllShieldsOption: true
      ),
      false
    )
  }

  /// Test `isShieldExpected(for:shield:considerAllShieldsOption:)` for Block
  /// Fingerprinting with `isBraveShieldsContentSettingsEnabled` flag disabled.
  func testBlockFingerprintingEnabled() {
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._fingerprintMode = { _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
      return .standardMode
    }
    testBraveShieldsSettings._setFingerprintMode = { _, _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` disabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: false
    )

    // Verify initial values
    XCTAssertTrue(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: false
      )
    )
    XCTAssertNil(domain.shield_fpProtection)
    // Update value
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setBlockFingerprintingEnabled(false, for: url)
    }
    // Verify updated values
    XCTAssertFalse(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: false
      )
    )
    XCTAssertEqual(domain.shield_fpProtection, NSNumber(booleanLiteral: false))

    // Reset back to enabled so we can test `considerAllShieldsOption`
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setBlockFingerprintingEnabled(true, for: url)
    }
    XCTAssertTrue(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: false
      )
    )
    // Verify `considerAllShieldsOption`
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertFalse(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: true
      )
    )
  }

  /// Test `isShieldExpected(for:shield:considerAllShieldsOption:)` for Block
  /// Fingerprinting with `isBraveShieldsContentSettingsEnabled` flag enabled.
  func testBlockFingerprintingEnabledContentSettings() {
    var fingerPrintMode: BraveShields.FingerprintMode = .standardMode

    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._fingerprintMode = { url in
      XCTAssertEqual(url, self.url)
      return fingerPrintMode
    }
    testBraveShieldsSettings._setFingerprintMode = { mode, url in
      XCTAssertEqual(url, self.url)
      XCTAssertEqual(mode, .allowMode)
      fingerPrintMode = mode
    }
    testBraveShieldsSettings._isBraveShieldsEnabled = { _ in
      // disabled so we can test `considerAllShieldsOption: true`
      return false
    }

    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` enabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: true
    )

    // Verify initial values
    XCTAssertTrue(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: false
      )
    )
    // Update value
    braveShieldsTabHelper.setBlockFingerprintingEnabled(false, for: url)
    // Verify updated values
    XCTAssertFalse(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: false
      )
    )
    // Verify `considerAllShieldsOption`
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    XCTAssertFalse(
      braveShieldsTabHelper.isShieldExpected(
        for: url,
        shield: .fpProtection,
        considerAllShieldsOption: true
      )
    )
  }

  /// Test `shredLevel(for:)` with `isBraveShieldsContentSettingsEnabled` flag disabled.
  func testShredLevel() {
    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._fingerprintMode = { _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
      return .standardMode
    }
    testBraveShieldsSettings._setFingerprintMode = { _, _ in
      XCTFail("BraveShieldsSettings should not be called when feature flag is disabled")
    }
    let domain = Domain.getOrCreate(forUrl: url, persistent: true)
    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` disabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: false
    )

    // Verify initial values
    XCTAssertEqual(
      braveShieldsTabHelper.shredLevel(for: url, considerAllShieldsOption: false),
      .never
    )
    XCTAssertNil(domain.shield_shredLevel)
    // Update value
    backgroundSaveAndWaitForExpectation {
      braveShieldsTabHelper.setShredLevel(.appExit, for: url)
    }
    // Verify updated values
    XCTAssertEqual(
      braveShieldsTabHelper.shredLevel(for: url, considerAllShieldsOption: false),
      .appExit
    )
    XCTAssertEqual(domain.shield_shredLevel, SiteShredLevel.appExit.rawValue)
  }

  func testShredLevelContentSetting() {
    var autoShredMode: BraveShields.AutoShredMode = .never

    let testBraveShieldsSettings = TestBraveShieldsSettings()
    testBraveShieldsSettings._autoShredMode = { url in
      XCTAssertEqual(url, self.url)
      return autoShredMode
    }
    testBraveShieldsSettings._setAutoShredMode = { mode, url in
      XCTAssertEqual(url, self.url)
      XCTAssertEqual(mode, .appExit)
      autoShredMode = mode
    }
    testBraveShieldsSettings._isShieldsDisabledOnAnyHostMatchingDomain = { url in
      // return true (Shields disabled on a host matching the given domain)
      // so we can verify `considerAllShieldsOption` will result in `.never`
      return true
    }

    let tabState = TabStateFactory.create(with: .init(braveCore: nil))
    // Test with `isBraveShieldsContentSettingsEnabled` enabled
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings,
      isBraveShieldsContentSettingsEnabled: true
    )

    // Verify initial values
    XCTAssertEqual(
      braveShieldsTabHelper.shredLevel(for: url, considerAllShieldsOption: false),
      .never
    )
    // Update value
    braveShieldsTabHelper.setShredLevel(.appExit, for: url)
    // Verify updated value
    XCTAssertEqual(
      braveShieldsTabHelper.shredLevel(for: url, considerAllShieldsOption: false),
      .appExit
    )

    // Verify `considerAllShieldsOption` for same url returns `.never`
    XCTAssertEqual(
      braveShieldsTabHelper.shredLevel(for: url, considerAllShieldsOption: true),
      .never
    )
  }
}
