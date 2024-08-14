// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

private typealias SE = InitialSearchEngines

class InitialSearchEnginesTests: XCTestCase {

  func testDefaultValues() throws {
    let unknownLocaleSE = SE(locale: Locale(identifier: "xx_XX"))
    let engines = unknownLocaleSE.engines.map { $0.id }

    XCTAssertEqual(
      engines,
      [
        .google,
        .braveSearch,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    XCTAssertEqual(unknownLocaleSE.defaultSearchEngine, .google)
    XCTAssertNil(unknownLocaleSE.priorityEngine)

    unknownLocaleSE.engines.forEach {
      XCTAssertNil($0.customId)
    }
  }

  // MARK: - Locale overrides

  func testYandexRegions() throws {
    var localeSE = SE()

    for region in localeSE.yandexDefaultRegions {
      localeSE = SE(locale: Locale(identifier: "ru_\(region)"))
      XCTAssertEqual(localeSE.defaultSearchEngine, .yandex)
    }
  }

  func testBraveSearchDefaultRegion() throws {
    var localeSE = SE(locale: Locale(identifier: "en_US"))

    localeSE = SE(locale: Locale(identifier: "en_GB"))
    XCTAssertTrue(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "ja_JP"))
    XCTAssertFalse(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "de_DE"))
    XCTAssertTrue(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "fr_FR"))
    XCTAssertTrue(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "pl_PL"))
    XCTAssertFalse(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "ru_AZ"))
    XCTAssertFalse(localeSE.isBraveSearchDefaultRegion)

    localeSE = SE(locale: Locale(identifier: "xx_XX"))
    XCTAssertFalse(localeSE.isBraveSearchDefaultRegion)
  }

  // MARK: - Country specific tests

  func testEnUS() throws {
    let localeSE = SE(locale: Locale(identifier: "en_US"))

    let availableEngines = localeSE.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .braveSearch,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    XCTAssertEqual(localeSE.defaultSearchEngine, .braveSearch)
    XCTAssertNil(localeSE.priorityEngine)
  }

  func testJaJP() throws {
    let localeSE = SE(locale: Locale(identifier: "ja_JP"))
    let availableEngines = localeSE.engines.map { $0.id }

    XCTAssertEqual(
      availableEngines,
      [
        .google,
        .braveSearch,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    XCTAssertEqual(localeSE.defaultSearchEngine, .google)
    XCTAssertNil(localeSE.priorityEngine)
  }

  func testEnGB() throws {
    let localeSE = SE(locale: Locale(identifier: "en_GB"))
    let availableEngines = localeSE.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .braveSearch,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    XCTAssertEqual(localeSE.defaultSearchEngine, .braveSearch)
    XCTAssertNil(localeSE.priorityEngine)
  }

  func testDeDE() throws {
    let localeSE = SE(locale: Locale(identifier: "de_DE"))

    let availableEngines = localeSE.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .braveSearch,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    XCTAssertEqual(localeSE.defaultSearchEngine, .braveSearch)
    XCTAssertNil(localeSE.priorityEngine)
  }

  func testFrFR() throws {
    let localeSE = SE(locale: Locale(identifier: "fr_FR"))

    let availableEngines = localeSE.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .braveSearch,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
        .ecosia,
      ]
    )

    XCTAssertEqual(localeSE.defaultSearchEngine, .braveSearch)
    XCTAssertNil(localeSE.priorityEngine)
  }

  func testPlPL() throws {
    let unknownLocaleSE = SE(locale: Locale(identifier: "pl_PL"))

    let availableEngines = unknownLocaleSE.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .google,
        .braveSearch,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    let onboardingEngines = unknownLocaleSE.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    XCTAssertEqual(unknownLocaleSE.defaultSearchEngine, .google)
    XCTAssertNil(unknownLocaleSE.priorityEngine)
  }

  func testRuRu() throws {
    let russianLocale = SE(locale: Locale(identifier: "ru_RU"))

    let availableEngines = russianLocale.engines.map { $0.id }
    XCTAssertEqual(
      availableEngines,
      [
        .yandex,
        .braveSearch,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    let onboardingEngines = russianLocale.onboardingEngines.map { $0.id }
    XCTAssertEqual(
      onboardingEngines,
      [
        .yandex,
        .google,
        .bing,
        .duckduckgo,
        .qwant,
        .startpage,
      ]
    )

    XCTAssertEqual(russianLocale.defaultSearchEngine, .yandex)
    XCTAssertNil(russianLocale.priorityEngine)
  }
}
