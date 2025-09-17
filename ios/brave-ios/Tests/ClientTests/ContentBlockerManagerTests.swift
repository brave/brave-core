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

    let filterListType = ContentBlockerManager.BlocklistType.engineSource(
      .filterList(componentId: filterListUUID),
      engineType: .standard
    )
    try await manager.compile(
      encodedContentRuleList: encodedContentRuleList,
      for: filterListType,
      version: "0",
      modes: filterListType.allowedModes
    )
    let customListType = ContentBlockerManager.BlocklistType.engineSource(
      .filterListURL(uuid: filterListCustomUUID),
      engineType: .standard
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
      try await manager.removeRuleLists(for: filterListType)
      try await manager.removeRuleLists(for: customListType)
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

    try await manager.compileRuleList(
      at: filterListURL,
      for: .engineSource(
        .filterListURL(uuid: "iodkpdagapdfkphljnddpjlldadblomo"),
        engineType: .standard
      ),
      version: "0",
      modes: ContentBlockerManager.BlockingMode.allCases
    )
  }

  func testRulesTestingSuccess() async {
    let filterSet = [
      "! This is a network rule",
      "||example.com^",
      "! This is an empty line",
      "",
      "! This is a cosmetic filter",
      "example.com,example.net##h1",
    ]

    let manager = await makeManager()
    let result = await manager.testRules(
      forFilterSet: filterSet.joined(separator: "\n")
    )
    XCTAssertNil(result)
  }

  @MainActor private func makeManager(
    userDefaults: UserDefaults? = UserDefaults(suiteName: "tests")
  ) -> ContentBlockerManager {
    return ContentBlockerManager(
      ruleStore: ruleStore,
      container: userDefaults ?? .standard
    )
  }

  @MainActor func testMissingModesFilterListGroup() async throws {
    let blocklistTypeStandard: ContentBlockerManager.BlocklistType = .engineGroup(
      id: "standard",
      engineType: .standard
    )
    let blocklistTypeStandardAggressive: ContentBlockerManager.BlocklistType = .engineGroup(
      id: "standard",
      engineType: .aggressive
    )
    // assign mock existing versions to our UserDefaults test suite
    let existingVersion = "2025-03-07T18:43:06Z,1.0.71"
    let existingVersions: [String: String] = [
      blocklistTypeStandard.makeIdentifier(for: .standard): existingVersion,
      blocklistTypeStandardAggressive.makeIdentifier(for: .aggressive): existingVersion,
    ]
    let userDefaults = UserDefaults(suiteName: "tests")
    userDefaults?.set(existingVersions, forKey: "content-blocker.versions")

    let contentBlockerManager = makeManager(userDefaults: userDefaults)

    // assign mock cached rule lists (required so we can compare versions)
    let ruleList = try await makeMockRuleList(id: #function)
    let cachedList: ContentBlockerManager.CompileResult = .success(ruleList)
    contentBlockerManager.setTestingCachedRuleLists([
      blocklistTypeStandard.makeIdentifier(for: .standard): cachedList,
      blocklistTypeStandardAggressive.makeIdentifier(for: .aggressive): cachedList,
    ])

    // Test no updates needed (no missing modes)
    let missingModesStandard = await contentBlockerManager.missingModes(
      for: blocklistTypeStandard,
      version: existingVersion
    )
    XCTAssertTrue(missingModesStandard.isEmpty)
    let missingModesAggressive = await contentBlockerManager.missingModes(
      for: blocklistTypeStandardAggressive,
      version: existingVersion
    )
    XCTAssertTrue(missingModesAggressive.isEmpty)

    // Test update needed (>0 missing modes)
    let newVersion = "2025-03-07T18:43:06Z,1.0.72"
    XCTAssertNotEqual(existingVersion, newVersion)
    let missingModesStandard2 = await contentBlockerManager.missingModes(
      for: blocklistTypeStandard,
      version: newVersion
    )
    XCTAssertFalse(missingModesStandard2.isEmpty)
    let missingModesAggressive2 = await contentBlockerManager.missingModes(
      for: blocklistTypeStandardAggressive,
      version: newVersion
    )
    XCTAssertFalse(missingModesAggressive2.isEmpty)

    let newVersionDate = "2025-03-08T18:43:06Z,1.0.71"
    XCTAssertNotEqual(existingVersion, newVersionDate)
    let missingModesStandard3 = await contentBlockerManager.missingModes(
      for: blocklistTypeStandard,
      version: newVersionDate
    )
    XCTAssertFalse(missingModesStandard3.isEmpty)
    let missingModesAggressive3 = await contentBlockerManager.missingModes(
      for: blocklistTypeStandardAggressive,
      version: newVersionDate
    )
    XCTAssertFalse(missingModesAggressive3.isEmpty)
  }

  @MainActor func testMissingModesFilterListComponent() async throws {
    let blocklistTypeFilterListAggressive: ContentBlockerManager.BlocklistType = .engineSource(
      .filterList(componentId: "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"),
      engineType: .aggressive
    )
    let existingVersion = "1.0.7341"
    let existingVersions: [String: String] = [
      blocklistTypeFilterListAggressive.makeIdentifier(for: .aggressive): existingVersion
    ]
    let userDefaults = UserDefaults(suiteName: "tests")
    userDefaults?.set(existingVersions, forKey: "content-blocker.versions")
    let contentBlockerManager = makeManager(userDefaults: userDefaults)

    let ruleList = try await makeMockRuleList(id: #function)

    contentBlockerManager.setTestingCachedRuleLists([
      blocklistTypeFilterListAggressive.makeIdentifier(for: .aggressive): .success(ruleList)
    ])

    // Test no updates needed
    let missingModes = await contentBlockerManager.missingModes(
      for: blocklistTypeFilterListAggressive,
      version: existingVersion
    )
    XCTAssertTrue(missingModes.isEmpty)

    // Test update needed (>0 missing modes)
    let newVersion = "1.0.7342"
    XCTAssertNotEqual(existingVersion, newVersion)
    let missingModesStandard2 = await contentBlockerManager.missingModes(
      for: blocklistTypeFilterListAggressive,
      version: newVersion
    )
    XCTAssertFalse(missingModesStandard2.isEmpty)

    // extra digit over `existingVersion`. This would fail regular string
    // comparison (`existing < newVersion`), but not if we use `.numeric` compare.
    let newVersionExtraDigit = "1.0.12345"
    XCTAssertNotEqual(existingVersion, newVersionExtraDigit)
    let missingModesStandard3 = await contentBlockerManager.missingModes(
      for: blocklistTypeFilterListAggressive,
      version: newVersionExtraDigit
    )
    XCTAssertFalse(missingModesStandard3.isEmpty)
  }

  @MainActor func testMissingModesGeneric() async throws {
    let blocklistTypeGenericTrackers: ContentBlockerManager.BlocklistType = .generic(.blockTrackers)
    let existingVersion = "2025-01-23T19:55:25Z"
    let existingVersions: [String: String] = [
      blocklistTypeGenericTrackers.makeIdentifier(for: .general): existingVersion
    ]
    let userDefaults = UserDefaults(suiteName: "tests")
    userDefaults?.set(existingVersions, forKey: "content-blocker.versions")
    let contentBlockerManager = makeManager(userDefaults: userDefaults)

    let ruleList = try await makeMockRuleList(id: #function)
    contentBlockerManager.setTestingCachedRuleLists([
      blocklistTypeGenericTrackers.makeIdentifier(for: .general): .success(ruleList)
    ])

    // Test no updates needed (no missing modes)
    let missingModes = await contentBlockerManager.missingModes(
      for: blocklistTypeGenericTrackers,
      version: existingVersion
    )
    XCTAssertTrue(missingModes.isEmpty)

    // Test update needed (>0 missing modes)
    let newVersionDate = "2025-02-23T19:55:25Z"
    XCTAssertNotEqual(existingVersion, newVersionDate)
    let missingModesStandard3 = await contentBlockerManager.missingModes(
      for: blocklistTypeGenericTrackers,
      version: newVersionDate
    )
    XCTAssertFalse(missingModesStandard3.isEmpty)
  }

  @MainActor private func makeMockRuleList(id: String) async throws -> WKContentRuleList {
    let testRuleListStore = try XCTUnwrap(
      WKContentRuleListStore(
        url: URL.temporaryDirectory.appending(path: id)
      )
    )
    let ruleList = try await testRuleListStore.compileContentRuleList(
      forIdentifier: id,
      encodedContentRuleList: #"[{"trigger":{"url-filter":".*"},"action":{"type":"block"}}]"#
    )
    return try XCTUnwrap(ruleList)
  }
}
