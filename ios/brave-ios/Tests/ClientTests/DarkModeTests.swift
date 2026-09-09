// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import TestHelpers
import UIKit
import XCTest

@testable import Brave

class DarkModeTests: XCTestCase {

  func testNightModeSettingFromStoredPreferences() {
    XCTAssertEqual(NightModeSetting(isEnabled: false, followsAppearance: false), .off)
    XCTAssertEqual(NightModeSetting(isEnabled: true, followsAppearance: false), .on)
    XCTAssertEqual(
      NightModeSetting(isEnabled: false, followsAppearance: true),
      .followAppearance
    )
    XCTAssertEqual(
      NightModeSetting(isEnabled: true, followsAppearance: true),
      .followAppearance
    )
  }

  func testNightModeSettingEffectiveState() {
    XCTAssertFalse(NightModeSetting.off.isEnabled(whenAppearanceIsDark: false))
    XCTAssertFalse(NightModeSetting.off.isEnabled(whenAppearanceIsDark: true))
    XCTAssertTrue(NightModeSetting.on.isEnabled(whenAppearanceIsDark: false))
    XCTAssertTrue(NightModeSetting.on.isEnabled(whenAppearanceIsDark: true))
    XCTAssertFalse(NightModeSetting.followAppearance.isEnabled(whenAppearanceIsDark: false))
    XCTAssertTrue(NightModeSetting.followAppearance.isEnabled(whenAppearanceIsDark: true))
  }

  func testNightModeBlockedURL() {
    let blockList = [
      "twitter.com",
      "m.twitter.com",
      "youtube.com",
      "m.youtube.com",
      "www.youtube.com",
      "x.com",
      "m.x.com",
      "search.brave.com",
      "m.search.brave.com",
      "www.search.brave.com",
    ]

    let allowList = [
      "twitter.brave.com",
      "m.twitter.brave.com",
      "m.brave.com",
      "brave.com",
      "talk.brave.com",
      "bbc.co.uk",
      "news.bbc.co.uk",
      "bbc.example.co.uk",
      "example.bbc.foo.co.uk",
      "example.bbc.foo.com",
    ]

    for urlString in blockList {
      guard let url = URL(string: "https://\(urlString)") else {
        XCTFail("Invalid URL: \(urlString)")
        continue
      }

      XCTAssertTrue(NightModeTabHelper.isNightModeBlockedURL(url))
    }

    for urlString in allowList {
      guard let url = URL(string: "https://\(urlString)") else {
        XCTFail("Invalid URL: \(urlString)")
        continue
      }

      XCTAssertFalse(NightModeTabHelper.isNightModeBlockedURL(url))
    }
  }
}
