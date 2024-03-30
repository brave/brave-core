// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit
import XCTest

@testable import Brave

class ContentBlockerManagerTests: XCTestCase {
  @MainActor private lazy var ruleStore: WKContentRuleListStore = {
    let testBundle = Bundle.module
    let bundleURL = testBundle.bundleURL
    return WKContentRuleListStore(url: bundleURL)!
  }()

  func testContentBlockerManager() async throws {
    // Given
    let bundle = Bundle.module
    let resourceURL = bundle.url(forResource: "content-blocking", withExtension: "json")!
    let encodedContentRuleList = try String(contentsOf: resourceURL, encoding: .utf8)
    let filterListUUID = UUID().uuidString
    let filterListCustomUUID = UUID().uuidString
    let manager = await self.makeManager()

    // When
    for generalType in ContentBlockerManager.GenericBlocklistType.allCases {
      do {
        try await manager.compileBundledRuleList(for: generalType)
      } catch {
        XCTFail(error.localizedDescription)
      }
    }

    let filterListType = ContentBlockerManager.BlocklistType.filterList(
      componentId: filterListUUID,
      isAlwaysAggressive: false
    )
    try await manager.compile(
      encodedContentRuleList: encodedContentRuleList,
      for: filterListType,
      version: "0",
      modes: filterListType.allowedModes
    )
    let customListType = ContentBlockerManager.BlocklistType.customFilterList(
      uuid: filterListCustomUUID
    )
    try await manager.compile(
      encodedContentRuleList: encodedContentRuleList,
      for: customListType,
      version: "0",
      modes: customListType.allowedModes
    )

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
      try await manager.removeRuleLists(
        for: .filterList(componentId: filterListUUID, isAlwaysAggressive: false)
      )
      try await manager.removeRuleLists(for: .customFilterList(uuid: filterListCustomUUID))
    } catch {
      XCTFail(error.localizedDescription)
    }
  }

  func testRuleConversion() async throws {
    guard
      let filterListURL = Bundle.module.url(
        forResource: "iodkpdagapdfkphljnddpjlldadblomo",
        withExtension: "txt"
      )
    else {
      XCTFail("Expected resource not found")
      return
    }
    let manager = await makeManager()

    await manager.compileRuleList(
      at: filterListURL,
      for: .customFilterList(uuid: "iodkpdagapdfkphljnddpjlldadblomo"),
      version: "0",
      modes: ContentBlockerManager.BlockingMode.allCases
    )
  }

  @MainActor private func makeManager() -> ContentBlockerManager {
    return ContentBlockerManager(
      ruleStore: ruleStore,
      container: UserDefaults(suiteName: "tests") ?? .standard
    )
  }
}
