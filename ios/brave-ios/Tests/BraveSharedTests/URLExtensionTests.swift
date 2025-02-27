// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
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
    XCTAssertEqual(
      embeddedURL.encodeEmbeddedInternalURL(for: .httpBlocked)?.strippedInternalURL,
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
      embeddedURL.encodeEmbeddedInternalURL(for: .httpBlocked)?.displayURL,
      embeddedURL
    )
    XCTAssertEqual(
      embeddedURL.displayURL,
      embeddedURL
    )
  }

  // Test that `windowOriginURL` returns the same value as `window.origin`.
  @MainActor func testWindowOriginURL() async {
    let testURLs = [
      // multiple subdomains
      (URL(string: "https://one.two.three.example.com")!, "https://one.two.three.example.com"),
      // trailing slash
      (URL(string: "https://example.com/")!, "https://example.com"),
      // query
      (URL(string: "https://www.example.com/?v=1234567")!, "https://www.example.com"),
      // match
      (URL(string: "https://www.example.com")!, "https://www.example.com"),
      // punycode
      (URL(string: "http://Дом.ru/")!, "http://xn--d1aqf.ru"),
      // punycode
      (URL(string: "http://Дoм.ru/")!, "http://xn--o-gtbz.ru"),
    ]

    let webView = WKWebView()
    for (value, expected) in testURLs {
      do {
        let expectation = XCTestExpectation(description: "didFinish")
        let navigationDelegate = NavigationDelegate(didFinish: {
          expectation.fulfill()
        })
        webView.navigationDelegate = navigationDelegate
        webView.loadHTMLString("", baseURL: value)

        // await load of html
        await fulfillment(of: [expectation], timeout: 4)

        guard let result = try await webView.evaluateJavaScript("window.origin") as? String else {
          XCTFail("Expected a String result")
          return
        }
        XCTAssertEqual(result, expected)
        XCTAssertEqual(result, value.windowOriginURL.absoluteString)
      } catch {
        XCTFail("Expected a valid `window.origin`")
      }
    }
  }

  func testURLToShred() {
    let testURL = URL(string: "https://brave.com")!

    // Verify regular url
    XCTAssertEqual(testURL, testURL.urlToShred)
    XCTAssertTrue(testURL.isShredAvailable)

    // Verify original url returned for reader mode
    let readerModeTestURL = testURL.encodeEmbeddedInternalURL(for: .readermode, headers: [:])!
    XCTAssertNotEqual(readerModeTestURL, testURL)
    XCTAssertEqual(readerModeTestURL.urlToShred, testURL)
    XCTAssertTrue(readerModeTestURL.isShredAvailable)

    // Verify nil for other `InternalURL`s
    let errorPageTestURL = testURL.encodeEmbeddedInternalURL(for: .errorpage)!
    XCTAssertNil(errorPageTestURL.urlToShred)
    XCTAssertFalse(errorPageTestURL.isShredAvailable)

    let blockedTestURL = testURL.encodeEmbeddedInternalURL(for: .blocked)!
    XCTAssertNil(blockedTestURL.urlToShred)
    XCTAssertFalse(blockedTestURL.isShredAvailable)

    // Verify nil for new tab page
    let ntpTestURL = URL(string: "\(InternalURL.baseUrl)/about/home#panel=0")!
    XCTAssertNil(ntpTestURL.urlToShred)
    XCTAssertFalse(ntpTestURL.isShredAvailable)
  }

  func testBlobURLAuthStripping() {
    var url = URL(string: "blob:https://brave.github.io/1945c9fb-9834-479b-bb3a-e67e3a9408cf")!
    XCTAssertEqual(
      url.strippingBlobURLAuth.absoluteString,
      "blob:https://brave.github.io/1945c9fb-9834-479b-bb3a-e67e3a9408cf"
    )

    url = URL(
      string:
        "blob:https://some.malicious.domain.com@brave.github.io/1945c9fb-9834-479b-bb3a-e67e3a9408cf"
    )!
    XCTAssertEqual(
      url.strippingBlobURLAuth.absoluteString,
      "blob:https://brave.github.io/1945c9fb-9834-479b-bb3a-e67e3a9408cf"
    )

    url = URL(string: "blob:https://some.malicious.domain.com@brave.github.io/")!
    XCTAssertEqual(url.strippingBlobURLAuth.absoluteString, "blob:https://brave.github.io/")

    url = URL(string: "blob:https://some.malicious.domain.com@brave.github.io/?hello=world")!
    XCTAssertEqual(
      url.strippingBlobURLAuth.absoluteString,
      "blob:https://brave.github.io/?hello=world"
    )

    url = URL(string: "blob:https://some.malicious.domain.com@brave.github.io/helloworld")!
    XCTAssertEqual(
      url.strippingBlobURLAuth.absoluteString,
      "blob:https://brave.github.io/helloworld"
    )

    url = URL(string: "https://some.malicious.domain.com@brave.github.io/")!
    XCTAssertEqual(
      url.strippingBlobURLAuth.absoluteString,
      "https://some.malicious.domain.com@brave.github.io/"
    )

    url = URL(string: "https://brave.github.io/")!
    XCTAssertEqual(url.strippingBlobURLAuth.absoluteString, "https://brave.github.io/")
  }

  private func testURLComponentsEncoding() {
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "Дом.com"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "Дoм.com"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://Дом.com"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://Дoм.com"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://google.ca/"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://google.ca/search?q=hello"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://google.ca/search?q=hello%20world"))
    XCTAssertTrue(URL.isValidURLWithoutEncoding(text: "https://google.ca/", scheme: "https"))
    XCTAssertFalse(URL.isValidURLWithoutEncoding(text: "https://google.ca/", scheme: "blob"))
    XCTAssertFalse(URL.isValidURLWithoutEncoding(text: "https://google.ca/search?q=hello + world"))
    XCTAssertFalse(URL.isValidURLWithoutEncoding(text: "https://google.ca/search?q=hello world"))
    XCTAssertFalse(URL.isValidURLWithoutEncoding(text: "hello world"))
  }
}

private class NavigationDelegate: NSObject, WKNavigationDelegate {
  private var didFinish: () -> Void

  init(didFinish: @escaping () -> Void) {
    self.didFinish = didFinish
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    didFinish()
  }
}
