// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveNews

class OPMLParsingTests: XCTestCase {

  func loadTestData(named testFileName: String) -> Data {
    try! Data(contentsOf: URL(fileURLWithPath: Bundle.module.path(forResource: testFileName, ofType: "opml")!))
  }

  func testBasicParsing() throws {
    let data = loadTestData(named: "subscriptionList")
    let opml = try XCTUnwrap(OPMLParser.parse(data: data))
    XCTAssertEqual(opml.title, "mySubscriptions.opml")
    // Test parse all outlines
    XCTAssertEqual(opml.outlines.count, 13)
    // Test basic parse
    XCTAssert(opml.outlines.contains(.init(text: "CNET News.com", xmlUrl: "http://news.com.com/2547-1_3-0-5.xml")))
    // Test parse where title text contained HTML entity ("NYT &gt; Business")
    XCTAssert(opml.outlines.contains(.init(text: "NYT > Business", xmlUrl: "http://www.nytimes.com/services/xml/rss/nyt/Business.xml")))
  }

  func testNoFeedsFound() throws {
    let data = loadTestData(named: "states")
    let opml = try XCTUnwrap(OPMLParser.parse(data: data))
    XCTAssertEqual(opml.outlines.count, 0)
  }

  func testParseInvalidData() throws {
    let json = try XCTUnwrap(#"{"data": "This isn't XML or OPML"}"#.data(using: .utf8))
    XCTAssertNil(OPMLParser.parse(data: json))
    let html = try XCTUnwrap(#"<html><head><title>This isn't OPML</title></head><body></body></html>"#.data(using: .utf8))
    XCTAssertNil(OPMLParser.parse(data: html))
  }
}
