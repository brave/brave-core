// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import UIKit
import XCTest

@testable import Brave

class QuickSearchItemIDTests: XCTestCase {

  override func setUp() {
    super.setUp()

    Preferences.Search.orderedEngines.reset()
    Preferences.Search.disabledEngines.reset()
    Preferences.Search.quickSearchOrder.reset()
    Preferences.Search.quickSearchDisabled.reset()
  }

  // MARK: - Helpers

  private func makeEngine(
    engineID: String?,
    shortName: String,
    searchTemplate: String = "https://example.com/?q={searchTerms}",
    isCustomEngine: Bool = false
  ) -> OpenSearchEngine {
    OpenSearchEngine(
      engineID: engineID,
      shortName: shortName,
      image: UIImage(),
      searchTemplate: searchTemplate,
      suggestTemplate: nil,
      isCustomEngine: isCustomEngine
    )
  }

  func testRawValueIsInjectiveAcrossCases() {
    let ids: [QuickSearchItemID] = [
      .engine("news"),
      .customEngine("news"),
      .aiChat,
      .braveVertical("news"),
    ]

    let rawValues = ids.map(\.rawValue)
    XCTAssertEqual(Set(rawValues).count, ids.count, "rawValue collision between cases")
  }

  func testRawValueIsNamespaced() {
    XCTAssertEqual(QuickSearchItemID.engine("bravesearch").rawValue, "engine:bravesearch")
    XCTAssertEqual(
      QuickSearchItemID.customEngine("https://a/?q=%s").rawValue,
      "custom:https://a/?q=%s"
    )
    XCTAssertEqual(QuickSearchItemID.aiChat.rawValue, "ai-chat")
    XCTAssertEqual(QuickSearchItemID.braveVertical("images").rawValue, "vertical:images")
  }

  func testBundledEngineUsesEngineID() {
    let id = QuickSearchItemID(
      engine: makeEngine(engineID: "bravesearch", shortName: "Brave Search beta")
    )
    XCTAssertEqual(id, .engine("bravesearch"))
  }

  func testCustomEngineUsesTemplateEvenWhenEngineIDIsPresent() {
    let template = "https://firefox.com/find?q={searchTerms}"
    let id = QuickSearchItemID(
      engine: makeEngine(
        engineID: "ATester",
        shortName: "ATester",
        searchTemplate: template,
        isCustomEngine: true
      )
    )
    XCTAssertEqual(id, .customEngine(template))
  }

  func testCustomEnginesWithSameNameGetDistinctIDs() {
    let first = QuickSearchItemID(
      engine: makeEngine(
        engineID: nil,
        shortName: "Search",
        searchTemplate: "https://a.example/?q={searchTerms}",
        isCustomEngine: true
      )
    )
    let second = QuickSearchItemID(
      engine: makeEngine(
        engineID: nil,
        shortName: "Search",
        searchTemplate: "https://b.example/?q={searchTerms}",
        isCustomEngine: true
      )
    )
    XCTAssertNotEqual(first, second)
  }

  func testMigrationMapsLegacyPrefsPreservingOrder() {
    let engines = [
      makeEngine(engineID: "google.com", shortName: "Google"),
      makeEngine(engineID: "bravesearch", shortName: "Brave Search beta"),
      makeEngine(engineID: "bing.com", shortName: "Bing"),
    ]
    Preferences.Search.orderedEngines.value = ["Bing", "Google", "Brave Search beta"]
    Preferences.Search.disabledEngines.value = ["Google"]

    SearchEngines().migrateQuickSearchIfNeeded(engines: engines)

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      ["engine:bing.com", "engine:google.com", "engine:bravesearch"]
    )
    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, ["engine:google.com"])
  }

  func testMigrationSeedsFromEngineOrderWhenLegacyOrderIsMissing() {
    let engines = [
      makeEngine(engineID: "google.com", shortName: "Google"),
      makeEngine(engineID: "bing.com", shortName: "Bing"),
    ]

    SearchEngines().migrateQuickSearchIfNeeded(engines: engines)

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      ["engine:google.com", "engine:bing.com"]
    )
    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, [])
  }

  func testMigrationIsIdempotent() {
    let engines = [makeEngine(engineID: "google.com", shortName: "Google")]
    let searchEngines = SearchEngines()

    searchEngines.migrateQuickSearchIfNeeded(engines: engines)
    let afterFirstRun = Preferences.Search.quickSearchOrder.value

    // A later reorder writes the new pref directly; re-running must not clobber it.
    Preferences.Search.orderedEngines.value = ["Google", "Bing"]
    searchEngines.migrateQuickSearchIfNeeded(engines: engines)

    XCTAssertEqual(Preferences.Search.quickSearchOrder.value, afterFirstRun)
  }

  func testMigrationIsFirstWinsOnDuplicateShortName() {
    let engines = [
      makeEngine(engineID: "google.com", shortName: "Search"),
      makeEngine(engineID: "bing.com", shortName: "Search"),
    ]
    Preferences.Search.orderedEngines.value = ["Search"]

    SearchEngines().migrateQuickSearchIfNeeded(engines: engines)

    XCTAssertEqual(Preferences.Search.quickSearchOrder.value, ["engine:google.com"])
  }
}
