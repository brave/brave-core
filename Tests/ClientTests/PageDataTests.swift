// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
@testable import Brave

final class PageDataTests: XCTestCase {
  func testBasicExample() throws {
    // Given
    // Page data with empty ad-block stats
    let mainFrameURL = URL(string: "http://example.com")!
    let subFrameURL = URL(string: "http://example.com/1p/subframe")!
    let upgradedMainFrameURL = URL(string: "https://example.com")!
    let upgradedSubFrameURL = URL(string: "https://example.com/1p/subframe")!
    var pageData = PageData(mainFrameURL: mainFrameURL, adBlockStats: AdBlockStats())
    
    // When
    // We get the script types for the main frame
    let mainFrameRequestTypes = pageData.makeUserScriptTypes(
      forRequestURL: mainFrameURL, isForMainFrame: true, persistentDomain: false
    )
    
    // Then
    // We get only entries of the main frame
    let expectedMainFrameTypes: Set<UserScriptType> = [
      .siteStateListener, .nacl, .farblingProtection(etld: "example.com")
    ]
    XCTAssertEqual(mainFrameRequestTypes, expectedMainFrameTypes)
    
    // When
    // Nothing has changed
    let unchangedScriptTypes = pageData.makeUserScriptTypes(
      forRequestURL: mainFrameURL, isForMainFrame: true, persistentDomain: false
    )
    
    // Then
    // We get the same result as before
    XCTAssertEqual(mainFrameRequestTypes, unchangedScriptTypes)
  }
}
