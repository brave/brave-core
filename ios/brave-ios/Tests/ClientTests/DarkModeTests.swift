// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import TestHelpers
import UIKit
import XCTest

@testable import Brave

class DarkModeTests: XCTestCase {

  func testNightModeBlockedURL() {
    let blockList = [
      "twitter.com",
      "m.twitter.com",
      "youtube.com",
      "m.youtube.com",
      "www.youtube.com",
      "amazon.co.uk",
      "amazon.com",
      "m.amazon.com",
      "m.amazon.co.uk",
      "x.com",
      "m.x.com",
      "search.brave.com",
      "m.search.brave.com",
      "www.search.brave.com",
    ]

    let allowList = [
      "twitter.brave.com",
      "m.twitter.brave.com",
      "m.search.amazon.com",
      "m.search.amazon.co.uk",
      "search.amazon.co.uk",
      "search.amazon.brave.com",
      "m.brave.com",
      "brave.com",
      "talk.brave.com",
      "bbc.co.uk",
      "news.bbc.co.uk",
      "bbc.example.co.uk",
      "example.bbc.foo.co.uk",
      "example.bbc.foo.com",
    ]

    for urlString in blockList {
      guard let url = URL(string: "https://\(urlString)") else {
        XCTFail("Invalid URL: \(urlString)")
        continue
      }

      XCTAssertTrue(DarkReaderScriptHandler.isNightModeBlockedURL(url))
    }

    for urlString in allowList {
      guard let url = URL(string: "https://\(urlString)") else {
        XCTFail("Invalid URL: \(urlString)")
        continue
      }

      XCTAssertFalse(DarkReaderScriptHandler.isNightModeBlockedURL(url))
    }
  }
}
