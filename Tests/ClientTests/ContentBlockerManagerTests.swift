// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
import Data
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
    let domain = Domain.getOrCreate(forUrl: URL(string: "https://example.com")!, persistent: false)
    
    Task.detached {
      // When
      await manager.loadBundledResources()
      await manager.set(resource: ContentBlockerManager.Resource(
        url: resourceURL,
        sourceType: downloadedSourceType
      ), for: downloadedRuleType)
      
      await manager.compilePendingResources()
      let returnedCachedRuleLists = await manager.ruleLists(for: domain)
      
      await MainActor.run {
        // Then
        // Check for the correct source and cached rule list
        for generalType in ContentBlockerManager.GeneralBlocklistTypes.allCases {
          let returnedSourceType = manager.sourceType(for: .general(generalType))
          
          switch generalType {
          case .blockAds:
            // Check for downloaded rule type
            XCTAssertEqual(returnedSourceType, downloadedSourceType)
            XCTAssertTrue(returnedCachedRuleLists.contains(where: {
              $0.identifier == ContentBlockerManager.BlocklistRuleType.general(generalType).identifier
            }))
          case .blockCookies, .blockTrackers:
            // Check for bundled rule type
            XCTAssertEqual(returnedSourceType, .bundled)
            XCTAssertNotNil(returnedSourceType)
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
        
        let returnedCachedRuleLists = manager.ruleLists(for: domain)
        XCTAssertTrue(returnedCachedRuleLists.contains(where: {
          $0.identifier == downloadedRuleType.identifier
        }))
        
        expectation.fulfill()
      }
    }
    
    wait(for: [expectation], timeout: 10)
  }
}
