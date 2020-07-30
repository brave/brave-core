/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
@testable import Client
import UIKit

import XCTest

class DefaultSearchPrefsTests: XCTestCase {

    func testParsing() {
        // setup the list json
        let searchPrefs = DefaultSearchPrefs(with: Bundle.main.resourceURL!.appendingPathComponent("SearchPlugins").appendingPathComponent("list.json"))!

        // setup the most popular locales
        let allEngines = ["yahoo", "bing", "duckduckgo", "qwant", "startpage", "google"]
        let us = (lang: ["en-US", "en"], region: "US", resultList: allEngines, resultDefault: "Google")
        let england = (lang: ["en-GB"], region: "GB", resultList: allEngines, resultDefault: "Google")
        let france = (lang: ["fr-FR", "fr"], region: "FR", resultList: allEngines, resultDefault: "Qwant")
        let japan = (lang: ["ja-JP", "ja"], region: "JP", resultList: ["yahoo-jp", "bing", "duckduckgo", "qwant", "startpage", "google"], resultDefault: "Google")
        let canada = (lang: ["en-CA", "en"], region: "CA", resultList: allEngines, resultDefault: "Google")
        let russia = (lang: ["ru-RU", "ru"], region: "RU", resultList: ["yandex-ru", "duckduckgo", "qwant", "startpage", "google"], resultDefault: "Яндекс")
        let taiwan = (lang: ["zh-TW", "zh"], region: "TW", resultList: allEngines, resultDefault: "Google")
        let china = (lang: ["zh-hans-CN", "zh-CN", "zh"], region: "CN", resultList: allEngines, resultDefault: "Google")
        let germany = (lang: ["de-DE", "de"], region: "DE", resultList: allEngines, resultDefault: "DuckDuckGo")
        let southAfrica = (lang: ["en-SA", "en"], region: "SA", resultList: allEngines, resultDefault: "Google")
        let testLocales = [us, england, france, japan, canada, russia, taiwan, china, germany, southAfrica]

        // run tests
        testLocales.forEach { locale in
            XCTAssertEqual(searchPrefs.searchDefault(for: locale.lang, and: locale.region), locale.resultDefault)
            XCTAssertEqual(searchPrefs.visibleDefaultEngines(locales: locale.lang, region: locale.region), locale.resultList)
        }
    }
}
