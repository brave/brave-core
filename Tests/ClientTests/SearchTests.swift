/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


@testable import Brave
import UIKit
import Shared
import BraveCore

import XCTest

class SearchTests: XCTestCase {
  
  override func setUp() {
    super.setUp()
    
    assert(BraveCoreMain.initializeICUForTesting(), "ICU should load for test")
  }
  
  func testParsing() {
    let parser = OpenSearchParser(pluginMode: true)
    let file = Bundle.module.path(forResource: "google-search-plugin", ofType: "xml")
    let engine: OpenSearchEngine! = parser.parse(file!, engineID: "google", referenceURL: "google.com")
    XCTAssertEqual(engine.shortName, "Google")

    // Test regular search queries.
    XCTAssertEqual(engine.searchURLForQuery("foobar")!.absoluteString, "https://www.google.com/search?q=foobar&ie=utf-8&oe=utf-8")

    // Test search suggestion queries.
    XCTAssertEqual(engine.suggestURLForQuery("foobar")!.absoluteString, "https://www.google.com/complete/search?client=firefox&q=foobar")
  }

  func testURIFixup() {
    // Check valid URLs. We can load these after some fixup.
    checkValidURL("about:", afterFixup: "about:")
    checkValidURL("about:config", afterFixup: "about:config")
    checkValidURL("about: config", afterFixup: "about:%20config")
    
    checkValidURL("file:///f/o/o", afterFixup: "file:///f/o/o")
    checkValidURL("ftp://ftp.mozilla.org", afterFixup: "ftp://ftp.mozilla.org/")
    checkValidURL("foo.bar", afterFixup: "http://foo.bar/")
    checkValidURL(" foo.bar ", afterFixup: "http://foo.bar/")
    
    checkValidURL("[::1]:80", afterFixup: "http://[::1]/")
    checkValidURL("[2a04:4e42:400::288]", afterFixup: "http://[2a04:4e42:400::288]/")
    checkValidURL("[2a04:4e42:600::288]:80", afterFixup: "http://[2a04:4e42:600::288]/")
    checkValidURL("[2605:2700:0:3::4713:93e3]:443", afterFixup: "http://[2605:2700:0:3::4713:93e3]:443/")
    checkValidURL("[::192.9.5.5]", afterFixup: "http://[::c009:505]/")
    checkValidURL("[::192.9.5.5]:80", afterFixup: "http://[::c009:505]/")
    checkValidURL("[::192.9.5.5]/png", afterFixup: "http://[::c009:505]/png")
    checkValidURL("[::192.9.5.5]:80/png", afterFixup: "http://[::c009:505]/png")
    checkValidURL("192.168.2.1", afterFixup: "http://192.168.2.1/")
    
    checkValidURL("http://www.brave.com", afterFixup: "http://www.brave.com/")
    checkValidURL("https://www.wikipedia.org", afterFixup: "https://www.wikipedia.org/")
    checkValidURL("brave.io", afterFixup: "http://brave.io/")
    checkValidURL("brave.new.world", afterFixup: "http://brave.new.world/")
    checkValidURL("brave.new.world.test", afterFixup: "http://brave.new.world.test/")
    checkValidURL("brave.new.world.test.io", afterFixup: "http://brave.new.world.test.io/")
    checkValidURL("brave.new.world.test.whatever.io", afterFixup: "http://brave.new.world.test.whatever.io/")
    
    checkValidURL("http://2130706433:8000/", afterFixup: "http://127.0.0.1:8000/")
    checkValidURL("http://127.0.0.1:8080", afterFixup: "http://127.0.0.1:8080/")
    checkValidURL("http://127.0.1", afterFixup: "http://127.0.0.1/")
    checkValidURL("http://127.1", afterFixup: "http://127.0.0.1/")
    checkValidURL("http://127.1:8000", afterFixup: "http://127.0.0.1:8000/")
    checkValidURL("http://1.1:80", afterFixup: "http://1.0.0.1/")
    checkValidURL("http://1.1:80", afterFixup: "http://1.0.0.1/")
    checkValidURL("http://1.1:80", afterFixup: "http://1.0.0.1/")
  
    // Check invalid URLs. These are passed along to the default search engine.
    checkInvalidURL("user@domain.com")
    checkInvalidURL("user@domain.com/whatever")
    checkInvalidURL("user@domain.com?something=whatever")
  
    checkInvalidURL("foobar")
    checkInvalidURL("foo bar")
    checkInvalidURL("brave. org")
    checkInvalidURL("123")
    checkInvalidURL("a/b")
    checkInvalidURL("创业咖啡")
    checkInvalidURL("创业咖啡 中国")
    checkInvalidURL("创业咖啡. 中国")
    checkInvalidURL("data:text/html;base64,SGVsbG8gV29ybGQhCg==")
    checkInvalidURL("data://https://www.example.com,fake example.com")
    checkInvalidURL("1.2.3")
    checkInvalidURL("1.1")
    checkInvalidURL("127.1")
    checkInvalidURL("127.1.1")

    // Check invalid quoted URLs, emails, and quoted domains.
    // These are passed along to the default search engine.
    checkInvalidURL(#""123"#)
    checkInvalidURL(#""123""#)
    checkInvalidURL(#""ftp.brave.com"#)
    checkInvalidURL(#""ftp.brave.com""#)
    checkInvalidURL(#"https://"ftp.brave.com""#)
    checkInvalidURL(#"https://"ftp.brave.com"#)
    checkInvalidURL("foo@brave.com")
    checkInvalidURL("\"foo@brave.com")
    checkInvalidURL("\"foo@brave.com\"")
    checkInvalidURL(#""创业咖啡.中国"#)
    checkInvalidURL(#""创业咖啡.中国""#)
    checkInvalidURL("foo:5000")
    checkInvalidURL("http://::192.9.5.5")
    checkInvalidURL("http://::192.9.5.5:8080")
  }
  
  func testURIFixupPunyCode() {
    checkValidURL("https://日本語.jp", afterFixup: "https://xn--wgv71a119e.jp/")
    checkValidURL("http://anlaşırız.net", afterFixup: "http://xn--anlarz-s9ab52b.net/")
    checkValidURL("http://ö.de", afterFixup: "http://xn--nda.de/")
    checkValidURL("https://дом.рф", afterFixup: "https://xn--d1aqf.xn--p1ai/")
    checkValidURL("https://дoм.рф", afterFixup: "https://xn--o-gtbz.xn--p1ai/")
    checkValidURL("https://www.Ǎ-.com", afterFixup: "https://www.xn----dta.com/")
    checkValidURL("http://www.googlé.com", afterFixup: "http://www.xn--googl-fsa.com/")
    checkValidURL("http://asĸ.com", afterFixup: "http://xn--as-3pa.com/")
    checkValidURL("http://dießner.de", afterFixup: "http://diessner.de/")
    checkValidURL("http://16კ.com", afterFixup: "http://xn--16-1ik.com/")
    checkValidURL("http://www.pаypal.com", afterFixup: "http://www.xn--pypal-4ve.com/")
    checkValidURL("http://аpple.com", afterFixup: "http://xn--pple-43d.com/")
    checkValidURL("http://ebаy.com/", afterFixup: "http://xn--eby-7cd.com/")
    checkValidURL("https://biṇaṇce.com", afterFixup: "https://xn--biace-4l1bb.com/")
    
    XCTAssertEqual(URLFormatter.formatURLOrigin(forSecurityDisplay: "https://biṇaṇce.com", schemeDisplay: .show), "https://xn--biace-4l1bb.com")
    XCTAssertEqual(URLFormatter.formatURLOrigin(forSecurityDisplay: "https://дом.рф", schemeDisplay: .show), "https://дом.рф")
    XCTAssertEqual(URLFormatter.formatURLOrigin(forSecurityDisplay: "https://дoм.рф", schemeDisplay: .show), "https://xn--o-gtbz.рф")
  }

  fileprivate func checkValidURL(_ beforeFixup: String, afterFixup: String) {
    XCTAssertEqual(URIFixup.getURL(beforeFixup)!.absoluteString, afterFixup)
  }

  fileprivate func checkInvalidURL(_ beforeFixup: String) {
    XCTAssertNil(URIFixup.getURL(beforeFixup))
  }

  func testExtractingOfSearchTermsFromURL() {
    let parser = OpenSearchParser(pluginMode: true)
    var file = Bundle.module.path(forResource: "google-search-plugin", ofType: "xml")
    let googleEngine: OpenSearchEngine! = parser.parse(file!, engineID: "google", referenceURL: "google.com")

    // create URL
    let searchTerm = "Foo Bar"
    let encodedSeachTerm = searchTerm.replacingOccurrences(of: " ", with: "+")
    let googleSearchURL = URL(string: "https://www.google.com/search?q=\(encodedSeachTerm)&ie=utf-8&oe=utf-8&gws_rd=cr&ei=I0UyVp_qK4HtUoytjagM")
    let duckDuckGoSearchURL = URL(string: "https://duckduckgo.com/?q=\(encodedSeachTerm)&ia=about")
    let invalidSearchURL = URL(string: "https://www.google.co.uk")

    // check it correctly matches google search term given google config
    XCTAssertEqual(searchTerm, googleEngine.queryForSearchURL(googleSearchURL))

    // check it doesn't match when the URL is not a search URL
    XCTAssertNil(googleEngine.queryForSearchURL(invalidSearchURL))

    // check that it matches given a different configuration
    file = Bundle.module.path(forResource: "duckduckgo-search-plugin", ofType: "xml")
    let duckDuckGoEngine: OpenSearchEngine! = parser.parse(file!, engineID: "duckduckgo", referenceURL: "duckduckgo.com/opensearch")
    XCTAssertEqual(searchTerm, duckDuckGoEngine.queryForSearchURL(duckDuckGoSearchURL))

    // check it doesn't match search URLs for different configurations
    XCTAssertNil(duckDuckGoEngine.queryForSearchURL(googleSearchURL))

    // check that if you pass in a nil URL that everything works
    XCTAssertNil(duckDuckGoEngine.queryForSearchURL(nil))
  }
}
