/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


@testable import Brave
import UIKit
import Shared

import XCTest

@MainActor class SearchTests: XCTestCase {
  
  override func setUp() {
    super.setUp()
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
