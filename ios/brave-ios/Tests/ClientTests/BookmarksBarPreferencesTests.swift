// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import XCTest

@testable import Brave

final class BookmarksBarPreferencesTests: XCTestCase {

  override func tearDown() {
    Preferences.General.showBookmarksBar.reset()
    super.tearDown()
  }

  func testBookmarksBarIsEnabledByDefault() {
    Preferences.General.showBookmarksBar.reset()

    XCTAssertTrue(Preferences.General.showBookmarksBar.value)
  }

  func testBookmarksBarCanBeDisabled() {
    Preferences.General.showBookmarksBar.value = false

    XCTAssertFalse(Preferences.General.showBookmarksBar.value)
  }
}
