// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import XCTest

class URLExtensionTests: XCTestCase {
  func testOrigin() {
    let urls = [
      ("https://www.example.com/index.html", "https://www.example.com"),
      ("https://user:pass@m.foo.com/bar/baz?noo=abc#123", "https://m.foo.com"),
      ("http://blabla.com/test@test", "http://blabla.com"),
      ("http://test.com:8080", "http://test.com:8080"),
      ("http://test:test@[2606:4700:20::681a:a72]:8080/", "http://[2606:4700:20::681a:a72]:8080"),
    ]

    let badURLs = [
      "data://google.com",
      "url:http://blabla.com/test:test",
      "http://test:t/est",
    ]

    urls.forEach { XCTAssertEqual(URL(string: $0.0)?.origin.serialized, $0.1) }
    badURLs.forEach { XCTAssertTrue(URL(string: $0)?.origin.isOpaque ?? true) }
  }

  func testStrippedInternalURL() {
    let urls = [
      (
        "internal://local/web3/ddns?service_id=ethereum&url=http%3A%2F%2Fvitalik%2Eeth%2F",
        URL(string: "http://vitalik.eth/")
      ),
      (
        "internal://local/sessionrestore?url=https://en.m.wikipedia.org/wiki/Main_Page",
        URL(string: "https://en.m.wikipedia.org/wiki/Main_Page")
      ),
      (
        "internal://local/reader-mode?url=https://en.m.wikipedia.org/wiki/Main_Page",
        URL(string: "https://en.m.wikipedia.org/wiki/Main_Page")
      ),
    ]

    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.strippedInternalURL, $0.1) }

    let embeddedURL = URL(string: "https://en.m.wikipedia.org/wiki/Main_Page?somequery=abc-123")!

    XCTAssertEqual(
      embeddedURL.encodeEmbeddedInternalURL(for: .readermode)?.strippedInternalURL,
      embeddedURL
    )
    XCTAssertEqual(
      embeddedURL.encodeEmbeddedInternalURL(for: .blocked)?.strippedInternalURL,
      embeddedURL
    )
    XCTAssertNil(embeddedURL.strippedInternalURL)
  }

  func testIsInternalURL() {
    let goodURLs = [
      "\(InternalURL.baseUrl)/\(InternalURL.Path.readermode.rawValue)?url=https%3A%2F%2Fbrave%2Ecom"
    ]
    let badURLs = [
      "http://google.com",
      "http://localhost:6571/sessionrestore.html",
      "http://localhost:1234/about/home/#panel=0",
      [
        "http://localhost:6571/reader-mode/page",
        "https%3A%2F%2Fen%2Em%2Ewikipedia%2Eorg%2Fwiki%2FMain%5FPage",
      ].joined(separator: "?url="),
    ]

    goodURLs.forEach { goodURL in
      XCTAssertTrue(URL(string: goodURL)!.isInternalURL(for: .readermode))
    }
    badURLs.forEach { badURL in
      XCTAssertFalse(URL(string: badURL)!.isInternalURL(for: .readermode))
    }
  }

  func testEmbeddedInternalURL() {
    let embeddedURL = URL(string: "https://en.m.wikipedia.org/wiki/Main_Page?somequery=abc-123")!
    let goodURLs = [
      URL(
        string: [
          "\(InternalURL.baseUrl)/\(InternalURL.Path.readermode.rawValue)",
          "https%3A%2F%2Fen%2Em%2Ewikipedia%2Eorg%2Fwiki%2FMain%5FPage%3Fsomequery%3Dabc%2D123",
        ].joined(separator: "?url=")
      )!
    ]
    let badURLs = [
      "http://google.com",
      "http://localhost:6571/sessionrestore.html",
      "http://localhost:1234/about/home/#panel=0",
      "http://localhost:6571/reader-mode/page",
    ]

    // Test encoding
    goodURLs.forEach { goodURL in
      XCTAssertEqual(embeddedURL.encodeEmbeddedInternalURL(for: .readermode), goodURL)
    }
    // Test decoding
    goodURLs.forEach { goodURL in
      XCTAssertEqual(goodURL.decodeEmbeddedInternalURL(for: .readermode), embeddedURL)
    }
    // Test urls that don't have embedded content
    badURLs.forEach {
      XCTAssertNil(URL(string: $0)!.decodeEmbeddedInternalURL(for: .readermode), $0)
    }
  }

  func testDisplayURL() {
    let embeddedURL = URL(string: "https://en.m.wikipedia.org/wiki/Main_Page?somequery=abc-123")!

    XCTAssertEqual(
      embeddedURL.encodeEmbeddedInternalURL(for: .readermode)?.displayURL,
      embeddedURL
    )
    XCTAssertEqual(
      embeddedURL.encodeEmbeddedInternalURL(for: .blocked)?.displayURL,
      embeddedURL
    )
    XCTAssertEqual(
      embeddedURL.displayURL,
      embeddedURL
    )
  }
}
