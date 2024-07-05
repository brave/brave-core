// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Brave

@MainActor class StringExtensionsTests: XCTestCase {

  func testStringByTrimmingLeadingCharactersInSet() {
    XCTAssertEqual("foo   ", "   foo   ".stringByTrimmingLeadingCharactersInSet(.whitespaces))
    XCTAssertEqual("foo456", "123foo456".stringByTrimmingLeadingCharactersInSet(.decimalDigits))
    XCTAssertEqual("", "123456".stringByTrimmingLeadingCharactersInSet(.decimalDigits))
  }

  func testPreferredSearchSuggestionText() {
    XCTAssertEqual("brave", "   brave   ".preferredSearchSuggestionText)
    XCTAssertEqual("bravesearch123", "bravesearch123".preferredSearchSuggestionText)
    XCTAssertEqual("brave", "    brave".preferredSearchSuggestionText)
    XCTAssertEqual(
      "brave search talk- engine",
      "brave search talk- engine ".preferredSearchSuggestionText
    )
  }

  func testPercentEscaping() {
    func roundtripTest(
      _ input: String,
      _ expected: String,
      file: StaticString = #file,
      line: UInt = #line
    ) {
      let observed = input.escape()!
      XCTAssertEqual(observed, expected, "input is \(input)", file: file, line: line)
      let roundtrip = observed.unescape()
      XCTAssertEqual(roundtrip, input, "encoded is \(observed)", file: file, line: line)
    }

    roundtripTest("https://mozilla.com", "https://mozilla.com")
    roundtripTest(
      "http://www.cnn.com/2017/09/25/politics/north-korea-fm-us-bombers/index.html",
      "http://www.cnn.com/2017/09/25/politics/north-korea-fm-us-bombers/index.html"
    )
    roundtripTest("http://mozilla.com/?a=foo&b=bar", "http://mozilla.com/%3Fa%3Dfoo%26b%3Dbar")
  }

  func testCapitalizeFirst() {
    XCTAssertEqual("test".capitalizeFirstLetter, "Test")
    XCTAssertEqual("TEST".capitalizeFirstLetter, "TEST")
    XCTAssertEqual("Test".capitalizeFirstLetter, "Test")
    XCTAssertEqual("tEST".capitalizeFirstLetter, "TEST")
    XCTAssertEqual("test test".capitalizeFirstLetter, "Test test")
  }

  func testWithSecureUrlScheme() {
    XCTAssertEqual("test".withSecureUrlScheme, "https://test")
    XCTAssertEqual("http://test".withSecureUrlScheme, "https://test")
    XCTAssertEqual("https://test".withSecureUrlScheme, "https://test")
    XCTAssertEqual("https://https://test".withSecureUrlScheme, "https://test")
    XCTAssertEqual("https://http://test".withSecureUrlScheme, "https://test")
  }

  func testUnquotedIfNecessary() {
    XCTAssertEqual("\"test\"".unquotedIfNecessary, "test")
    XCTAssertEqual("'test'".unquotedIfNecessary, "test")
    XCTAssertEqual("test".unquotedIfNecessary, "test")
  }
}
