// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
import BraveShields
@testable import Brave

final class PageDataTests: XCTestCase {
  @MainActor func testBasicExample() throws {
    // Given
    // Page data with empty ad-block stats
    let mainFrameURL = URL(string: "http://example.com")!
    let subFrameURL = URL(string: "http://example.com/1p/subframe")!
    let upgradedMainFrameURL = URL(string: "https://example.com")!
    var pageData = PageData(mainFrameURL: mainFrameURL, adBlockStats: AdBlockStats())
    let expectation = expectation(description: "")
    
    Task { @MainActor in
      // When
      // We get the script types for the main frame
      let domain = pageData.domain(persistent: false)
      let mainFrameRequestTypes = await pageData.makeUserScriptTypes(domain: domain)
      
      // Then
      // We get only entries of the main frame
      // NOTE: If we were to add some engines we might see additional types
      let expectedMainFrameTypes: Set<UserScriptType> = [
        .siteStateListener, .nacl, .farblingProtection(etld: "example.com"), .gpc(ShieldPreferences.enableGPC.value)
      ]
      XCTAssertEqual(mainFrameRequestTypes, expectedMainFrameTypes)
      
      // When
      // Nothing has changed
      let unchangedScriptTypes = await pageData.makeUserScriptTypes(domain: domain)
      
      // Then
      // We get the same result as before
      XCTAssertEqual(mainFrameRequestTypes, unchangedScriptTypes)
      
      // When
      // Added sub frame
      pageData.addSubframeURL(forRequestURL: subFrameURL, isForMainFrame: false)
      
      // Then
      // We get no aditional scripts for sub-frame
      // NOTE: This is because we have no engines on AdBlockStats.
      // If we were to add some engines we might see additional types
      let addedSubFrameFrameRequestTypes = await pageData.makeUserScriptTypes(domain: domain)
      let expectedMainAndSubFrameTypes: Set<UserScriptType> = [
        .siteStateListener, .nacl, .farblingProtection(etld: "example.com"), .gpc(ShieldPreferences.enableGPC.value)
      ]
      XCTAssertEqual(expectedMainAndSubFrameTypes, addedSubFrameFrameRequestTypes)
      
      // When
      // Upgraded main frame
      let isUpgradedMainFrame = pageData.upgradeFrameURL(forResponseURL: upgradedMainFrameURL, isForMainFrame: true)
      
      // Then
      // We get the same result as before
      XCTAssertTrue(isUpgradedMainFrame)
      let upgradedMainFrameRequestTypes = await pageData.makeUserScriptTypes(domain: domain)
      XCTAssertEqual(upgradedMainFrameRequestTypes, unchangedScriptTypes)
      
      // When
      // Upgraded subframe
      let isUpgradedSubFrame = pageData.upgradeFrameURL(forResponseURL: subFrameURL, isForMainFrame: true)
      
      // Then
      // We get the same result as before
      XCTAssertTrue(isUpgradedSubFrame)
      let upgradedSubFrameFrameRequestTypes = await pageData.makeUserScriptTypes(domain: domain)
      XCTAssertEqual(upgradedSubFrameFrameRequestTypes, unchangedScriptTypes)
      
      expectation.fulfill()
    }
    
    waitForExpectations(timeout: 10)
  }
}
