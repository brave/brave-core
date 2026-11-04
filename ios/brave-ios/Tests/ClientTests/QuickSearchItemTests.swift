// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import Strings
import SwiftUI
import UIKit
import XCTest

@testable import Brave

class QuickSearchItemTests: XCTestCase {

  override func setUp() {
    super.setUp()

    Preferences.Search.defaultEngineName.reset()
    Preferences.Search.defaultPrivateEngineName.reset()
    Preferences.Search.disabledEngines.reset()
    Preferences.Search.orderedEngines.reset()
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

  private func makeDataSource(
    engines: [OpenSearchEngine],
    defaultEngineName: String? = nil,
    disabledEngineNames: [String] = [],
    isPrivate: Bool = false
  ) -> SearchSuggestionDataSource {
    Preferences.Search.disabledEngines.value = disabledEngineNames
    let searchEngines = SearchEngines()
    searchEngines.orderedEngines = engines
    if let defaultEngineName {
      let type: DefaultEngineType = isPrivate ? .privateMode : .standard
      type.option.value = defaultEngineName
    }
    return SearchSuggestionDataSource(
      isPrivate: isPrivate,
      isAIChatAvailable: false,
      isPlaylistAvailable: false,
      searchEngines: searchEngines
    )
  }

  // MARK: - QuickSearchItem

  func testItemIDMatchesSource() {
    let engine = makeEngine(engineID: "google.com", shortName: "Google")
    XCTAssertEqual(QuickSearchItem(source: .engine(engine)).id, .engine("google.com"))

    let custom = makeEngine(
      engineID: nil,
      shortName: "Custom",
      searchTemplate: "https://a.example/?q={searchTerms}",
      isCustomEngine: true
    )
    XCTAssertEqual(
      QuickSearchItem(source: .engine(custom)).id,
      .customEngine("https://a.example/?q={searchTerms}")
    )

    XCTAssertEqual(QuickSearchItem(source: .aiChat).id, .aiChat)

    let vertical = BraveSearchVertical(
      id: "images",
      accessibilityTitle: "Images",
      icon: Image(systemName: "photo")
    )
    XCTAssertEqual(QuickSearchItem(source: .braveVertical(vertical)).id, .braveVertical("images"))
  }

  func testEngineAccessibilityTitleUsesDisplayNameNotShortName() {
    let brave = makeEngine(
      engineID: "bravesearch",
      shortName: OpenSearchEngine.EngineNames.brave
    )
    XCTAssertEqual(
      QuickSearchItem(source: .engine(brave)).accessibilityTitle,
      String(format: Strings.searchEngineFormatText, "Brave Search")
    )
  }

  // MARK: - quickSearchItems

  func testQuickSearchItemsIsEmptyWithoutSearchEngines() {
    let dataSource = SearchSuggestionDataSource(
      isPrivate: false,
      isAIChatAvailable: true,
      isPlaylistAvailable: false,
      searchEngines: nil
    )
    XCTAssertTrue(dataSource.quickSearchItems.isEmpty)
    XCTAssertFalse(dataSource.hasQuickSearchEngines)
  }

  func testQuickSearchItemsPreservesEngineOrderAndContainsNoAiChat() {
    let dataSource = makeDataSource(
      engines: [
        makeEngine(engineID: "google.com", shortName: "Google"),
        makeEngine(engineID: "bing.com", shortName: "Bing"),
      ],
      defaultEngineName: "google.com"
    )

    XCTAssertEqual(
      dataSource.quickSearchItems.map(\.id),
      [.engine("google.com"), .engine("bing.com")]
    )
  }

  func testQuickSearchItemsExcludesDisabledEngines() {
    let dataSource = makeDataSource(
      engines: [
        makeEngine(engineID: "google.com", shortName: "Google"),
        makeEngine(engineID: "bing.com", shortName: "Bing"),
        makeEngine(engineID: "ddg", shortName: "DuckDuckGo"),
      ],
      defaultEngineName: "google.com",
      disabledEngineNames: ["Bing"]
    )

    XCTAssertEqual(
      dataSource.quickSearchItems.map(\.id),
      [.engine("google.com"), .engine("ddg")]
    )
  }

  func testQuickSearchItemsPrependsDefaultEngineWhenDisabled() {
    let dataSource = makeDataSource(
      engines: [
        makeEngine(engineID: "google.com", shortName: "Google"),
        makeEngine(engineID: "bing.com", shortName: "Bing"),
      ],
      defaultEngineName: "google.com",
      disabledEngineNames: ["Google"]
    )

    XCTAssertEqual(
      dataSource.quickSearchItems.map(\.id),
      [.engine("google.com"), .engine("bing.com")]
    )
  }

  // MARK: - hasQuickSearchEngines

  func testHasQuickSearchEnginesIsFalseForDefaultEngineOnly() {
    let dataSource = makeDataSource(
      engines: [makeEngine(engineID: "google.com", shortName: "Google")],
      defaultEngineName: "google.com"
    )

    XCTAssertEqual(dataSource.quickSearchItems.map(\.id), [.engine("google.com")])
    XCTAssertFalse(dataSource.hasQuickSearchEngines)
  }

  func testHasQuickSearchEnginesIsTrueForMultipleEngines() {
    let dataSource = makeDataSource(
      engines: [
        makeEngine(engineID: "google.com", shortName: "Google"),
        makeEngine(engineID: "bing.com", shortName: "Bing"),
      ],
      defaultEngineName: "google.com"
    )

    XCTAssertTrue(dataSource.hasQuickSearchEngines)
  }
}
