// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import XCTest

class StringExtensionTests: XCTestCase {

  func testURLEncoding() {
    let urlString = "https://example.com/test%"
    let urlStringEncoded = "https://example.com/test%25"
    XCTAssertEqual(
      urlString.addingPercentEncoding(withAllowedCharacters: .urlAllowed),
      urlStringEncoded
    )
  }

  func testJavascriptEscapedString() {
    let originalString =
      "function(){window.google={kEI:\'UZe4YMHZE8HBkwWpkIPoBg\',kEXPI:\'31\',kBL:\'ckvK\'};google.sn=\'web\';google.kHL=\'en\';}\u{2028}"

    let expectedString =
      "\"function(){window.google={kEI:\'UZe4YMHZE8HBkwWpkIPoBg\',kEXPI:\'31\',kBL:\'ckvK\'};google.sn=\'web\';google.kHL=\'en\';}\\\\u2028\""

    XCTAssertEqual(originalString.javaScriptEscapedString, expectedString)
  }

  func testHTMLEntityEncodedString() {
    let testString1 = "Sauron & Saruman's Device"
    let testString2 = " 1 > 2 < 3"
    let testString3 = "Brave New World`"

    let expectedString1 = "Sauron &amp; Saruman&#39;s Device"
    let expectedString2 = " 1 &gt; 2 &lt; 3"
    let expectedString3 = "Brave New World&lsquo;"

    XCTAssertEqual(testString1.htmlEntityEncodedString, expectedString1)
    XCTAssertEqual(testString2.htmlEntityEncodedString, expectedString2)
    XCTAssertEqual(testString3.htmlEntityEncodedString, expectedString3)
  }

  func testTruncateMiddle() {
    var str =
      "long-extended-file-name-containing-many-letters-and-dashes-to-confuse-the-user-with-a-long-extended-file-name-containing-many-letters-and-dashes-to-confuse-the-user.pdf"
    var result = str.truncatingMiddle(maxLength: 32)
    XCTAssertEqual(result.count, 32)
    XCTAssertTrue(result.hasSuffix(".pdf"))
    XCTAssertTrue(result.hasPrefix("long"))

    str = "user.pdf"
    result = str.truncatingMiddle(maxLength: 32)
    XCTAssertEqual(result, "user.pdf")

    str = "a"
    result = str.truncatingMiddle(maxLength: 0)
    XCTAssertEqual(result, "a")

    str = "a"
    result = str.truncatingMiddle(maxLength: -1)
    XCTAssertEqual(result, "a")

    str = "a"
    result = str.truncatingMiddle(maxLength: 1)
    XCTAssertEqual(result, "a")

    str = "a"
    result = str.truncatingMiddle(maxLength: 2)
    XCTAssertEqual(result, "a")

    str = "a"
    result = str.truncatingMiddle(maxLength: 3)
    XCTAssertEqual(result, "a")

    str = "abc"
    result = str.truncatingMiddle(maxLength: 2)
    XCTAssertEqual(result, "a…c")

    str = "abcd"
    result = str.truncatingMiddle(maxLength: 2)
    XCTAssertEqual(result, "a…d")

    str = "abcde"
    result = str.truncatingMiddle(maxLength: 2)
    XCTAssertEqual(result, "a…e")

    str = "abcde"
    result = str.truncatingMiddle(maxLength: 3)
    XCTAssertEqual(result, "a…e")
  }
}
