// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

private typealias SE = InitialSearchEngines

class InitialSearchEnginesTests: XCTestCase {
    
    func testDefaultValues() throws {
        let unknownLocaleSE = SE(locale: Locale(identifier: "xx_XX"))
        let engines = unknownLocaleSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.google,
                                 .braveSearch,
                                 .bing,
                                 .duckduckgo,
                                 .qwant,
                                 .startpage])
        
        XCTAssertEqual(unknownLocaleSE.defaultSearchEngine, .google)
        XCTAssertNil(unknownLocaleSE.priorityEngine)
        
        unknownLocaleSE.engines.forEach {
            XCTAssertNil($0.customId)
        }
    }
    
    // MARK: - Locale overrides
    
    func testDDGRegions() {
        for region in InitialSearchEngines.ddgDefaultRegions {
            let localeSE = SE(locale: Locale(identifier: "en_\(region)"))
            XCTAssertEqual(localeSE.defaultSearchEngine, .duckduckgo)
        }
    }
    
    func testQwantRegions() {
        for region in InitialSearchEngines.qwantDefaultRegions {
            let localeSE = SE(locale: Locale(identifier: "en_\(region)"))
            XCTAssertEqual(localeSE.defaultSearchEngine, .qwant)
        }
    }
    
    func testYahooRegions() {
        for region in InitialSearchEngines.yahooEligibleRegions {
            let localeSE = SE(locale: Locale(identifier: "en_\(region)"))
            XCTAssertNil(localeSE.priorityEngine)
        }
    }
    
    func testYandexRegions() throws {
        
        for region in InitialSearchEngines.yandexDefaultRegions {
            let localeSE = SE(locale: Locale(identifier: "ru_\(region)"))
            XCTAssertEqual(localeSE.defaultSearchEngine, .yandex)
        }
    }
    
    // MARK: - Country specific tests
    
    func testEnUS() throws {
        let localeSE = SE(locale: Locale(identifier: "en_US"))
        
        let availableEngines = localeSE.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.google,
                                          .braveSearch,
                                          .bing,
                                          .duckduckgo,
                                          .qwant,
                                          .startpage,
                                          .ecosia,
                                          .yahoo])
        
        let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.google,
                                           .braveSearch,
                                           .bing,
                                           .duckduckgo,
                                           .qwant,
                                           .startpage,
                                           .ecosia])
        
        
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertNil(localeSE.priorityEngine)
    }
    
    func testJaJP() throws {
        let localeSE = SE(locale: Locale(identifier: "ja_JP"))
        let availableEngines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(availableEngines, [.google,
                                          .braveSearch,
                                          .bing,
                                          .duckduckgo,
                                          .qwant,
                                          .startpage,
                                          .yahoo])
        
        let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.google,
                                           .bing,
                                           .duckduckgo,
                                           .qwant,
                                           .startpage])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertNil(localeSE.priorityEngine)
        XCTAssertNotNil(localeSE.engines.first(where: { $0.customId == "yahoo-jp" }))
    }
    
    func testEnGB() throws {
        let localeSE = SE(locale: Locale(identifier: "en_GB"))
        let availableEngines = localeSE.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.google,
                                          .braveSearch,
                                          .bing,
                                          .duckduckgo,
                                          .qwant,
                                          .startpage,
                                          .ecosia,
                                          .yahoo])
        
        let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.google,
                                           .bing,
                                           .duckduckgo,
                                           .qwant,
                                           .startpage,
                                           .ecosia])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertNil(localeSE.priorityEngine)
    }
    
    func testDeDE() throws {
        let localeSE = SE(locale: Locale(identifier: "de_DE"))
        
        let availableEngines = localeSE.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.duckduckgo,
                                          .braveSearch,
                                          .google,
                                          .bing,
                                          .qwant,
                                          .startpage,
                                          .ecosia,
                                          .yahoo])
        
        let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.duckduckgo,
                                           .google,
                                           .bing,
                                           .qwant,
                                           .startpage,
                                           .ecosia])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .duckduckgo)
        XCTAssertNil(localeSE.priorityEngine)
    }
    
    func testFrFR() throws {
        let localeSE = SE(locale: Locale(identifier: "fr_FR"))
        
        let availableEngines = localeSE.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.qwant,
                                          .braveSearch,
                                          .google,
                                          .bing,
                                          .duckduckgo,
                                          .startpage,
                                          .ecosia,
                                          .yahoo])
        
        let onboardingEngines = localeSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.qwant,
                                           .google,
                                           .bing,
                                           .duckduckgo,
                                           .startpage,
                                           .ecosia])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .qwant)
        XCTAssertNil(localeSE.priorityEngine)
    }
    
    func testPlPL() throws {
        let unknownLocaleSE = SE(locale: Locale(identifier: "pl_PL"))
        
        let availableEngines = unknownLocaleSE.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.google,
                                          .braveSearch,
                                          .bing,
                                          .duckduckgo,
                                          .qwant,
                                          .startpage])
        
        let onboardingEngines = unknownLocaleSE.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.google,
                                           .bing,
                                           .duckduckgo,
                                           .qwant,
                                           .startpage])
        
        XCTAssertEqual(unknownLocaleSE.defaultSearchEngine, .google)
        XCTAssertNil(unknownLocaleSE.priorityEngine)
    }
    
    func testRuRu() throws {
        let russianLocale = SE(locale: Locale(identifier: "ru_RU"))
        
        let availableEngines = russianLocale.engines.map { $0.id }
        XCTAssertEqual(availableEngines, [.yandex,
                                          .braveSearch,
                                          .google,
                                          .bing,
                                          .duckduckgo,
                                          .qwant,
                                          .startpage])
        
        let onboardingEngines = russianLocale.onboardingEngines.map { $0.id }
        XCTAssertEqual(onboardingEngines, [.yandex,
                                           .google,
                                           .bing,
                                           .duckduckgo,
                                           .qwant,
                                           .startpage])
        
        XCTAssertEqual(russianLocale.defaultSearchEngine, .yandex)
        XCTAssertNil(russianLocale.priorityEngine)
    }
}
