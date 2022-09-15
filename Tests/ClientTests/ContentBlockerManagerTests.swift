// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
@testable import Brave

class ContentBlockerManagerTests: XCTestCase {
  private lazy var ruleStore: WKContentRuleListStore = {
    let testBundle = Bundle.module
    let bundleURL = testBundle.bundleURL
    return WKContentRuleListStore(url: bundleURL)!
  }()
  
  func testCompilePendingResources() throws {
    // Given
    let bundle = Bundle.module
    let resourceURL = bundle.url(forResource: "content-blocking", withExtension: "json")!
    let downloadedRuleType = ContentBlockerManager.BlocklistRuleType.general(.blockAds)
    let downloadedSourceType = ContentBlockerManager.BlocklistSourceType.downloaded(version: "123")
    let expectation = XCTestExpectation(description: "Test loading resources")
    let manager = ContentBlockerManager(ruleStore: ruleStore)
    
    Task.detached {
      // When
      await manager.loadBundledResources()
      await manager.set(resource: ContentBlockerManager.Resource(
        url: resourceURL,
        sourceType: downloadedSourceType
      ), for: downloadedRuleType)
      
      await manager.compilePendingResources()
      
      await MainActor.run {
        // Then
        // Check for the correct source and cached rule list
        for generalType in ContentBlockerManager.GeneralBlocklistTypes.allCases {
          let returnedSourceType = manager.sourceType(for: .general(generalType))
          let returnedCachedRuleList = manager.cachedRuleList(for: .general(generalType))
          switch generalType {
          case .blockAds:
            // Check for downloaded rule type
            XCTAssertEqual(returnedSourceType, downloadedSourceType)
            XCTAssertNotNil(returnedCachedRuleList)
          case .blockCookies, .blockTrackers:
            // Check for bundled rule type
            XCTAssertEqual(returnedSourceType, .bundled)
            XCTAssertNotNil(returnedCachedRuleList)
          case .upgradeHTTP:
            if #available(iOS 15, *) {
              // We don't compile this except below iOS 15
              XCTAssertNil(returnedSourceType)
              XCTAssertNil(returnedCachedRuleList)
            } else {
              XCTAssertEqual(returnedSourceType, .bundled)
              XCTAssertNotNil(returnedCachedRuleList)
            }
          }
        }
      }
      
      // Check we go back to the bundled rule type if we remove the downloaded one
      await manager.removeResource(for: downloadedRuleType)
      await manager.compilePendingResources()
      
      Task { @MainActor in
        XCTAssertEqual(
          manager.sourceType(for: downloadedRuleType), .bundled
        )
        
        XCTAssertNotNil(
          manager.cachedRuleList(for: .general(.blockAds))
        )
        
        expectation.fulfill()
      }
    }
    
    wait(for: [expectation], timeout: 10)
  }
}
