// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Brave

class SystemSearchURLMatcherTests: XCTestCase {

  private func assertMatch(
    _ urlString: String,
    engine: SystemSearchURLMatcher.Engine,
    query: String = "test",
    file: StaticString = #filePath,
    line: UInt = #line
  ) {
    guard let url = URL(string: urlString) else {
      XCTFail("Invalid URL: \(urlString)", file: file, line: line)
      return
    }
    XCTAssertEqual(
      SystemSearchURLMatcher.match(url),
      .init(engine: engine, query: query),
      urlString,
      file: file,
      line: line
    )
  }

  private func assertNoMatch(
    _ urlString: String,
    file: StaticString = #filePath,
    line: UInt = #line
  ) {
    guard let url = URL(string: urlString) else {
      XCTFail("Invalid URL: \(urlString)", file: file, line: line)
      return
    }
    XCTAssertNil(SystemSearchURLMatcher.match(url), urlString, file: file, line: line)
  }

  func testGoogle() {
    assertMatch(
      "https://www.google.com/search?q=test&ie=UTF-8&oe=UTF-8&hl=en-us&client=safari",
      engine: .google
    )
    assertMatch(
      "https://www.google.com/search?client=safari&rls=en&q=test&ie=UTF-8&oe=UTF-8",
      engine: .google
    )
    assertMatch(
      "https://www.google.cn/search?q=test&ie=UTF-8&oe=UTF-8&hl=zh-hans-cn&client=safari",
      engine: .google
    )
    assertMatch("https://www.google.ca/search?q=test&client=safari", engine: .google)
    assertMatch("https://www.google.co.uk/search?q=test&client=safari", engine: .google)
    assertMatch("https://www.google.com.au/search?q=test&client=safari", engine: .google)
    assertMatch("https://www.google.com/search?q=test&tbm=isch", engine: .google)
    assertMatch("https://www.google.com/search?q=test&udm=2", engine: .google)
  }

  func testYahoo() {
    assertMatch("https://search.yahoo.com/search?p=test&fr=iphone&.tsrc=apple", engine: .yahoo)
    assertMatch("https://search.yahoo.com/search?p=test&fr=ipad", engine: .yahoo)
    assertMatch("https://search.yahoo.com/search?ei=utf-8&fr=aaplw&p=test", engine: .yahoo)
    assertMatch(
      "https://uk.search.yahoo.com/search?p=test&fr=iphone&.tsrc=apple",
      engine: .yahoo
    )
  }

  func testYahooJapan() {
    assertMatch("https://search.yahoo.co.jp/search?p=test&ei=UTF-8&fr=applep1", engine: .yahooJP)
    assertMatch("https://search.yahoo.co.jp/search?p=test&ei=UTF-8&fr=applpd", engine: .yahooJP)
    assertMatch("https://search.yahoo.co.jp/search?p=test&ei=UTF-8&fr=appsfch2", engine: .yahooJP)
  }

  func testBing() {
    assertMatch("https://www.bing.com/search?q=test&form=APIPH1&PC=APPL", engine: .bing)
    assertMatch("https://www.bing.com/search?q=test&form=APIPA1&PC=APPD", engine: .bing)
    assertMatch("https://www.bing.com/search?q=test&form=APMCS1&PC=APMC", engine: .bing)
    assertMatch("https://www.bing.com/images/search?q=test", engine: .bing)
  }

  func testDuckDuckGo() {
    assertMatch("https://duckduckgo.com/?q=test&t=iphone", engine: .duckDuckGo)
    assertMatch("https://duckduckgo.com/?q=test&t=ipad", engine: .duckDuckGo)
    assertMatch("https://duckduckgo.com/?q=test&t=osx", engine: .duckDuckGo)
    assertMatch("https://duckduckgo.com?q=test&t=iphone", engine: .duckDuckGo)
  }

  func testEcosia() {
    assertMatch("https://www.ecosia.org/search?q=test&tts=st_asaf_iphone", engine: .ecosia)
    assertMatch("https://www.ecosia.org/search?q=test&tts=st_asaf_ipad", engine: .ecosia)
    assertMatch("https://www.ecosia.org/search?q=test&tts=st_asaf_macos", engine: .ecosia)
  }

  func testBaidu() {
    assertMatch("https://m.baidu.com/s?from=1000539d&word=test", engine: .baidu)
    assertMatch("https://m.baidu.com/s?from=1099b&word=test", engine: .baidu)
    assertMatch("https://www.baidu.com/s?ie=utf-8&wd=test&tn=84053098_4_dg", engine: .baidu)
    assertMatch("https://www.baidu.com/s?ie=utf-8&wd=test&tn=84053098_1_dg", engine: .baidu)
    assertMatch("https://www.baidu.com/s?wd=test&tn=84053098_dg&ie=utf-8", engine: .baidu)
  }

  func testSogou() {
    assertMatch("https://m.sogou.com/web/sl?keyword=test&v=5", engine: .sogou)
    assertMatch("https://www.sogou.com/web?query=test&ie=utf8", engine: .sogou)
  }

  func test360Search() {
    assertMatch("https://m.so.com/s?q=test&src=home&srcg=safariwap_1", engine: .so360)
    assertMatch("https://www.so.com/s?q=test&src=pclm&ls=safarimac", engine: .so360)
  }

  func testYandex() {
    assertMatch("https://yandex.ru/search/touch/?clid=1906591&text=test", engine: .yandex)
    assertMatch("https://yandex.ru/search/pad/?clid=1906723&text=test", engine: .yandex)
    assertMatch("https://yandex.ru/search?clid=1906725&text=test", engine: .yandex)
    assertMatch("https://yandex.com/search/touch/?clid=1906591&text=test", engine: .yandex)
    assertMatch("https://yandex.com.tr/search/touch/?clid=1906591&text=test", engine: .yandex)
    assertMatch("https://yandex.kz/search/touch/?clid=1906591&text=test", engine: .yandex)
    assertMatch("https://yandex.by/search/touch/?clid=1906591&text=test", engine: .yandex)
    assertMatch("https://yandex.ua/search/touch/?clid=1906591&text=test", engine: .yandex)
  }

  func testQueryDecoding() {
    assertMatch("https://www.google.com/search?q=foo+bar", engine: .google, query: "foo bar")
    assertMatch("https://www.google.com/search?q=foo%20bar", engine: .google, query: "foo bar")
    assertMatch("https://www.google.com/search?q=1%2B1", engine: .google, query: "1+1")
    assertMatch("https://www.google.com/search?q=a%26b", engine: .google, query: "a&b")
    assertMatch("https://m.baidu.com/s?word=%E6%B5%8B%E8%AF%95", engine: .baidu, query: "测试")
    assertMatch(
      "https://yandex.ru/search/touch/?text=%D1%82%D0%B5%D1%81%D1%82",
      engine: .yandex,
      query: "тест"
    )
  }

  func testNonSearchURLs() {
    assertNoMatch("https://www.google.com/")
    assertNoMatch("https://www.google.com/maps?q=test")
    assertNoMatch("https://www.google.com/url?q=https://brave.com")
    assertNoMatch("https://www.google.com/search?q=")
    assertNoMatch("https://www.google.com/search?q=%20")
    assertNoMatch("https://www.google.com/search?client=safari")
    assertNoMatch("https://www.google.com.evil.com/search?q=test")
    assertNoMatch("https://evilgoogle.com/search?q=test")
    assertNoMatch("https://mail.google.com/search?q=test")
    assertNoMatch("ftp://www.google.com/search?q=test")
    assertNoMatch("https://www.bing.com/maps?q=test")
    assertNoMatch("https://duckduckgo.com/about")
    assertNoMatch("https://search.yahoo.com/?p=test")
    assertNoMatch("https://www.baidu.com/s?q=test")
    assertNoMatch("https://search.brave.com/search?q=test")
    assertNoMatch("https://brave.com/search?q=test")
  }
}

@MainActor
class SystemSearchURLReplacementTests: XCTestCase {

  private let googleURL = URL(
    string: "https://www.google.com/search?q=foo+bar&ie=UTF-8&oe=UTF-8&hl=en-us&client=safari"
  )!
  private let baiduURL = URL(string: "https://m.baidu.com/s?from=1000539d&word=test")!

  override func setUp() {
    super.setUp()
    resetPreferences()
  }

  override func tearDown() {
    super.tearDown()
    resetPreferences()
  }

  private func resetPreferences() {
    Preferences.Search.defaultEngineName.reset()
    Preferences.Search.defaultPrivateEngineName.reset()
    Preferences.Search.disabledEngines.reset()
    Preferences.Search.orderedEngines.reset()
  }

  private func loadedSearchEngines() async -> SearchEngines {
    let engines = SearchEngines(locale: Locale(identifier: "en_US"))
    await engines.loadSearchEngines()
    return engines
  }

  private func engine(
    named name: String,
    in engines: SearchEngines
  ) throws -> OpenSearchEngine {
    try XCTUnwrap(engines.orderedEngines.first(where: { $0.shortName == name }))
  }

  func testReplacesWithDefaultEngine() async throws {
    let engines = await loadedSearchEngines()
    engines.updateDefaultEngine(OpenSearchEngine.EngineNames.brave, forType: .standard)

    let result = NavigationPath.replacingSystemSearchURL(
      googleURL,
      isPrivate: false,
      searchEngines: engines
    )
    XCTAssertEqual(result.host, "search.brave.com")
    XCTAssertEqual(
      URLComponents(url: result, resolvingAgainstBaseURL: false)?.queryItems?
        .first(where: { $0.name == "q" })?.value,
      "foo bar"
    )
  }

  func testLeavesURLWhenDefaultEngineMatches() async throws {
    let engines = await loadedSearchEngines()
    engines.updateDefaultEngine("Google", forType: .standard)

    let result = NavigationPath.replacingSystemSearchURL(
      googleURL,
      isPrivate: false,
      searchEngines: engines
    )
    XCTAssertEqual(result, googleURL)
  }

  func testUsesPrivateDefaultEngine() async throws {
    let engines = await loadedSearchEngines()
    engines.updateDefaultEngine("Google", forType: .standard)
    engines.updateDefaultEngine(OpenSearchEngine.EngineNames.duckDuckGo, forType: .privateMode)

    XCTAssertEqual(
      NavigationPath.replacingSystemSearchURL(googleURL, isPrivate: false, searchEngines: engines),
      googleURL
    )
    let privateResult = NavigationPath.replacingSystemSearchURL(
      googleURL,
      isPrivate: true,
      searchEngines: engines
    )
    XCTAssertEqual(privateResult.host, "duckduckgo.com")
  }

  func testReplacesEngineWithoutBundledEquivalent() async throws {
    let engines = await loadedSearchEngines()
    engines.updateDefaultEngine("Google", forType: .standard)

    let result = NavigationPath.replacingSystemSearchURL(
      baiduURL,
      isPrivate: false,
      searchEngines: engines
    )
    XCTAssertEqual(result.host, "www.google.com")
  }

  func testLeavesNonSearchURL() async throws {
    let engines = await loadedSearchEngines()
    engines.updateDefaultEngine(OpenSearchEngine.EngineNames.brave, forType: .standard)

    let url = URL(string: "https://www.google.com/maps?q=test")!
    XCTAssertEqual(
      NavigationPath.replacingSystemSearchURL(url, isPrivate: false, searchEngines: engines),
      url
    )
  }

  func testBundledEngineMatching() async throws {
    let engines = await loadedSearchEngines()
    XCTAssertTrue(
      try engine(named: "Google", in: engines).isSameEngine(as: .google, url: googleURL)
    )
    XCTAssertFalse(try engine(named: "Bing", in: engines).isSameEngine(as: .google, url: googleURL))
    XCTAssertFalse(
      try engine(named: OpenSearchEngine.EngineNames.brave, in: engines)
        .isSameEngine(as: .google, url: googleURL)
    )
  }

  func testCustomEngineMatching() {
    let customGoogle = OpenSearchEngine(
      shortName: "My Google",
      image: UIImage(),
      searchTemplate: "https://google.com/search?q={searchTerms}&foo=bar",
      isCustomEngine: true
    )
    XCTAssertTrue(customGoogle.isSameEngine(as: .google, url: googleURL))

    let customBaidu = OpenSearchEngine(
      shortName: "My Baidu",
      image: UIImage(),
      searchTemplate: "https://baidu.com/custom?query={searchTerms}",
      isCustomEngine: true
    )
    XCTAssertTrue(customBaidu.isSameEngine(as: .baidu, url: baiduURL))
    XCTAssertFalse(customBaidu.isSameEngine(as: .google, url: googleURL))

    let customOther = OpenSearchEngine(
      shortName: "Other",
      image: UIImage(),
      searchTemplate: "https://example.com/find?q={searchTerms}",
      isCustomEngine: true
    )
    XCTAssertFalse(customOther.isSameEngine(as: .google, url: googleURL))
  }
}
