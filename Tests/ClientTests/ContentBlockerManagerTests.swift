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
  
  func testContentBlockerManager() throws {
    // Given
    let bundle = Bundle.module
    let resourceURL = bundle.url(forResource: "content-blocking", withExtension: "json")!
    let encodedContentRuleList = try String(contentsOf: resourceURL, encoding: .utf8)
    let expectation = XCTestExpectation(description: "Test loading resources")
    let manager = ContentBlockerManager(ruleStore: ruleStore)
    let filterListUUID = UUID().uuidString
    let filterListCustomUUID = UUID().uuidString
    
    Task { @MainActor in
      // When
      for generalType in ContentBlockerManager.GenericBlocklistType.allCases {
        do {
          try await manager.compileBundledRuleList(for: generalType)
        } catch {
          XCTFail(error.localizedDescription)
        }
      }
      
      do {
        let filterListType = ContentBlockerManager.BlocklistType.filterList(componentId: filterListUUID, isAlwaysAggressive: false)
        try await manager.compile(encodedContentRuleList: encodedContentRuleList, for: filterListType, modes: filterListType.allowedModes)
        let customListType = ContentBlockerManager.BlocklistType.customFilterList(uuid: filterListCustomUUID)
        try await manager.compile(encodedContentRuleList: encodedContentRuleList, for: customListType, modes: customListType.allowedModes)
      } catch {
        XCTFail(error.localizedDescription)
      }
      
      // Then
      // Check for loading the cached results
      for generalType in ContentBlockerManager.GenericBlocklistType.allCases {
        do {
          let blocklistType = ContentBlockerManager.BlocklistType.generic(generalType)
          
          for mode in blocklistType.allowedModes {
            let cachedType = try await manager.ruleList(for: blocklistType, mode: mode)
            XCTAssertNotNil(cachedType)
          }
        } catch {
          XCTFail(error.localizedDescription)
        }
      }
      
      // Then
      // Check for loading the uncached result from the rule store
      for generalType in ContentBlockerManager.GenericBlocklistType.allCases {
        do {
          let blocklistType = ContentBlockerManager.BlocklistType.generic(generalType)
          
          for mode in blocklistType.allowedModes {
            let cachedType = try await manager.ruleList(for: blocklistType, mode: mode)
            XCTAssertNotNil(cachedType)
          }
        } catch {
          XCTFail(error.localizedDescription)
        }
      }
      
      // Check removing the filter lists
      do {
        try await manager.removeRuleLists(for: .filterList(componentId: filterListUUID, isAlwaysAggressive: false))
        try await manager.removeRuleLists(for: .customFilterList(uuid: filterListCustomUUID))
      } catch {
        XCTFail(error.localizedDescription)
      }
      
      expectation.fulfill()
    }
    
    wait(for: [expectation], timeout: 10)
  }
}
