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
        let us = (lang: ["en-US", "en"], region: "US", resultList: ["amazondotcom", "twitter", "wikipedia", "yahoo", "bing", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let england = (lang: ["en-GB"], region: "GB", resultList: ["amazon-en-GB", "bing", "google", "twitter", "wikipedia", "yahoo-en-GB", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let france = (lang: ["fr-FR", "fr"], region: "FR", resultList: ["bing", "google", "twitter", "wikipedia-fr", "yahoo-france", "eBay-france", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let japan = (lang: ["ja-JP", "ja"], region: "JP", resultList: ["amazon-jp", "bing", "google", "twitter-ja", "wikipedia-ja", "yahoo-jp", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let canada = (lang: ["en-CA", "en"], region: "CA", resultList: ["amazondotcom", "twitter", "wikipedia", "yahoo", "bing", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let russia = (lang: ["ru-RU", "ru"], region: "RU", resultList: ["twitter", "wikipedia-ru", "yandex-ru", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Яндекс")
        let taiwan = (lang: ["zh-TW", "zh"], region: "TW", resultList: ["bing", "wikipedia-zh-TW", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let china = (lang: ["zh-hans-CN", "zh-CN", "zh"], region: "CN", resultList: ["baidu", "bing", "taobao", "wikipedia-zh-CN", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "百度")
        let germany = (lang: ["de-DE", "de"], region: "DE", resultList: ["amazon-de", "bing", "google", "twitter", "wikipedia-de", "yahoo-de", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let southAfrica = (lang: ["en-SA", "en"], region: "SA", resultList: ["amazondotcom", "twitter", "wikipedia", "yahoo", "bing", "google", "duckduckgo", "github", "infogalactic", "qwant", "semanticscholar", "stackoverflow", "startpage", "wolframalpha", "youtube"], resultDefault: "Google")
        let testLocales = [us, england, france, japan, canada, russia, taiwan, china, germany, southAfrica]

        // run tests
        testLocales.forEach { locale in
            XCTAssertEqual(searchPrefs.searchDefault(for: locale.lang, and: locale.region), locale.resultDefault)
            XCTAssertEqual(searchPrefs.visibleDefaultEngines(for: locale.lang, and: locale.region), locale.resultList)
        }
    }
}
