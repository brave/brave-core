// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Brave

class DefaultBrowserHelperTests: XCTestCase {
  override func setUp() async throws {
    Preferences.General.lastHTTPURLOpenedDate.reset()
  }

  func testUserRecentlyOpenedURL() {
    let date = Date()
    Preferences.General.lastHTTPURLOpenedDate.value = date
    let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()
    XCTAssertTrue(isLikelyDefault)
  }

  func testUserNeverOpenedURL() {
    // User never opened a link
    let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()
    XCTAssertFalse(isLikelyDefault)
  }

  func testUserOpenedURL8DaysAgo() {
    let date = Date()
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingTimeInterval(-8.days)
    let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()
    XCTAssertTrue(isLikelyDefault)
  }

  func testUserOpenedURL30DaysAgo() {
    let date = Date()
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingTimeInterval(-30.days)
    let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()
    XCTAssertFalse(isLikelyDefault)
  }
}
