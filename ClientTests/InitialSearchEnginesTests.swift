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
            XCTAssertEqual(localeSE.priorityEngine, .yahoo)
        }
    }
    
    func testYandexLocales() throws {
        
        for language in ["ru", "kk", "tr"] {
            for region in InitialSearchEngines.yandexDefaultRegions {
                let localeSE = SE(locale: Locale(identifier: "\(language)_\(region)"))
                let engines = localeSE.engines.map { $0.id }
                
                XCTAssertEqual(engines, [.yandex,
                                         .google,
                                         .bing,
                                         .duckduckgo,
                                         .qwant,
                                         .startpage])
                
                XCTAssertEqual(localeSE.defaultSearchEngine, .yandex)
                
                // Check for language specific search engines
                if language == "ru" {
                    XCTAssertNotNil(localeSE.engines.first(where: { $0.customId == "yandex-ru" }))
                }
                
                if language == "tr" {
                    XCTAssertNotNil(localeSE.engines.first(where: { $0.customId == "yandex-tr" }))
                }
                
                if language == "kk" {
                    XCTAssertNil(localeSE.engines.first(where: { $0.id == .yandex })?.customId)
                }
            }
            
            // non matching region
            for region in ["XX", "PL"] {
                let localeSE = SE(locale: Locale(identifier: "\(language)_\(region)"))
                let engines = localeSE.engines.map { $0.id }
                
                XCTAssertEqual(engines, [.google,
                                         .bing,
                                         .duckduckgo,
                                         .qwant,
                                         .startpage,
                                         .yandex])
                
                XCTAssertEqual(localeSE.defaultSearchEngine, .google)
            }
        }
    }
    
    // MARK: - Country specific tests
    
    func testEnUS() throws {
        let localeSE = SE(locale: Locale(identifier: "en_US"))
        let engines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.yahoo,
                                 .google,
                                 .bing,
                                 .duckduckgo,
                                 .qwant,
                                 .startpage])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertEqual(localeSE.priorityEngine, .yahoo)
    }
    
    func testJaJP() throws {
        let localeSE = SE(locale: Locale(identifier: "ja_JP"))
        let engines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.google,
                                 .bing,
                                 .duckduckgo,
                                 .qwant,
                                 .startpage,
                                 .yahoo])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertNil(localeSE.priorityEngine)
        XCTAssertNotNil(localeSE.engines.first(where: { $0.customId == "yahoo-jp" }))
    }
    
    func testEnGB() throws {
        let localeSE = SE(locale: Locale(identifier: "en_GB"))
        let engines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.yahoo,
                                 .google,
                                 .bing,
                                 .duckduckgo,
                                 .qwant,
                                 .startpage])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .google)
        XCTAssertEqual(localeSE.priorityEngine, .yahoo)
    }
    
    func testDeDE() throws {
        let localeSE = SE(locale: Locale(identifier: "de_DE"))
        let engines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.yahoo,
                                 .duckduckgo,
                                 .google,
                                 .bing,
                                 .qwant,
                                 .startpage])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .duckduckgo)
        XCTAssertEqual(localeSE.priorityEngine, .yahoo)
    }
    
    func testFrFR() throws {
        let localeSE = SE(locale: Locale(identifier: "fr_FR"))
        let engines = localeSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.yahoo,
                                 .qwant,
                                 .google,
                                 .bing,
                                 .duckduckgo,
                                 .startpage])
        
        XCTAssertEqual(localeSE.defaultSearchEngine, .qwant)
        XCTAssertEqual(localeSE.priorityEngine, .yahoo)
    }
    
    func testPlPL() throws {
        let unknownLocaleSE = SE(locale: Locale(identifier: "pl_PL"))
        let engines = unknownLocaleSE.engines.map { $0.id }
        
        XCTAssertEqual(engines, [.google,
                                 .bing,
                                 .duckduckgo,
                                 .qwant,
                                 .startpage])
        
        XCTAssertEqual(unknownLocaleSE.defaultSearchEngine, .google)
        XCTAssertNil(unknownLocaleSE.priorityEngine)
    }
}
