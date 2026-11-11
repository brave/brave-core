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

  /// Test `isBraveShieldsEnabled(for:)`.
  func testIsBraveShieldsEnabled() {
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

    let tabState = FakeTabState()
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings
    )

    // Verify initial value
    XCTAssertTrue(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
    // Update value
    braveShieldsTabHelper.setBraveShieldsEnabled(false, for: url)
    // Verify updated value
    XCTAssertFalse(isBraveShieldsEnabled)
    XCTAssertFalse(braveShieldsTabHelper.isBraveShieldsEnabled(for: url))
  }

  /// Test `shieldLevel(for:considerAllShieldsOption:)`.
  func testShieldLevel() {
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

    let tabState = FakeTabState()
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings
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
  /// Scripts.
  func testBlockScriptsEnabled() {
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

    let tabState = FakeTabState()
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings
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
  /// Fingerprinting.
  func testBlockFingerprintingEnabled() {
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

    let tabState = FakeTabState()
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings
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

  /// Test `shredLevel(for:considerAllShieldsOption:)`.
  func testShredLevel() {
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

    let tabState = FakeTabState()
    let braveShieldsTabHelper = BraveShieldsTabHelper(
      tab: tabState,
      braveShieldsSettings: testBraveShieldsSettings
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
