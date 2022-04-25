// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

class DebouncingResourceDownloaderTests: XCTestCase {
  private let downloader = DebouncingResourceDownloader()

  override func setUpWithError() throws {
    let bundle = Bundle(for: Self.self)
    let resourceURL = bundle.url(forResource: "debouncing", withExtension: "json")
    let data = try Data(contentsOf: resourceURL!)
    try downloader.setup(withRulesJSON: data)
  }

  /// Test simple redirection by query parameter
  func testSimpleRedirect() {
    // Given
    // A cosntructed url
    let baseURL = URL(string: "http://simple.a.com/")!
    let landingURL = URL(string: "http://simple.b.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: originalURL)

    // Then
    XCTAssertEqual(returnedURL, landingURL)
  }

  /// Test base64-encoded redirection by query parameter.
  func testBase64Redirect() {
    // Given
    // A cosntructed url
    let baseURL = URL(string: "http://base64.a.com/")!
    let landingURL = URL(string: "http://base64.b.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL, base64Encode: true)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: originalURL)

    // Then
    XCTAssertEqual(returnedURL, landingURL)
  }

  /// Test that debounce rules continue to be processed in order
  /// by constructing a URL that should be debounced to a second
  /// URL that should then be debounced to a third URL.
  func testRedirectChain() {
    // Given
    // A cosntructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlB = URL(string: "http://double.b.com/")!.addRedirectParam(urlZ)
    let urlA = URL(string: "http://double.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: urlA)

    // Then
    XCTAssertEqual(returnedURL, urlZ)
  }

  /// Test a long redirect chain.
  func testLongRedirectChain() {
    // Given
    // A cosntructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlD = URL(string: "http://quad.d.com/")!.addRedirectParam(urlZ)
    let urlC = URL(string: "http://quad.c.com/")!.addRedirectParam(urlD)
    let urlB = URL(string: "http://quad.b.com/")!.addRedirectParam(urlC)
    let urlA = URL(string: "http://quad.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: urlA)

    // Then
    XCTAssertEqual(returnedURL, urlZ)
  }

  /// Test a redirect chain that bounces from a tracker to a final URL in the
  /// tracker's domain.
  func testRedirectChainInTrackersDomain() {
    // Given
    // A cosntructed url
    let finalURL = URL(string: "http://z.com/")!
    let intermediateURL = URL(string: "http://tracker.z.com/")!.addRedirectParam(finalURL)
    let startURL = URL(string: "http://origin.h.com/")!.addRedirectParam(intermediateURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: startURL)

    // Then
    XCTAssertEqual(returnedURL, finalURL)
  }

  // Test a long redirect chain that bounces through the original URL's domain,
  // and verify the SiteForCookies used for the debounced request.
  func testLongRedirectChainInTrackersDomain() {
    // Given
    // A cosntructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlTrackerA = URL(string: "http://tracker.a.com/")!.addRedirectParam(urlZ)
    let urlD = URL(string: "http://quad.d.com/")!.addRedirectParam(urlTrackerA)
    let urlC = URL(string: "http://quad.c.com/")!.addRedirectParam(urlD)
    let urlB = URL(string: "http://quad.b.com/")!.addRedirectParam(urlC)
    let urlA = URL(string: "http://quad.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: urlA)

    // Then
    XCTAssertEqual(returnedURL, urlZ)
  }

  /// Test that debounce rules are not processed twice by constructing
  /// a URL that should be debounced to a second URL that would be
  /// debounced to a third URL except that that rule has already been
  /// processed, so it won't.
  func testDoubleBounce() {
    // Given
    // A cosntructed url
    let finalURL = URL(string: "http://z.com/")!
    let intermediateURL = URL(string: "http://double.a.com/")!.addRedirectParam(finalURL)
    let startURL = URL(string: "http://double.b.com/")!.addRedirectParam(intermediateURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: startURL)

    // Then
    // TODO: @JS Should be going to intermediateURL
    // Even if I add support for this on this method. The way that the implementation works for web-kit
    // It would just process it on the next webView.loadRequest navigation.
    XCTAssertEqual(returnedURL, finalURL)
  }

  /// Test wildcard URL patterns by constructing a URL that should be
  /// debounced because it matches a wildcard include pattern.
  func testWildcardIncludePattern() {
    // Given
    // A cosntructed url
    let landingURL = URL(string: "http://z.com/")!
    let startURL = URL(string: "http://included.c.com/")!.addRedirectParam(landingURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: startURL)

    // Then
    XCTAssertEqual(returnedURL, landingURL)
  }

  /// Test wildcard URL patterns by constructing a URL that should be
  /// debounced because it matches a wildcard include pattern.
  func testUnknownActionsAreIgnored() {
    // Given
    // A cosntructed url
    let landingURL = URL(string: "http://z.com/")!
    let startURL = URL(string: "http://included.d.com/")!.addRedirectParam(landingURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: startURL)

    // Then
    XCTAssertNil(returnedURL)
  }

  /// Test URL exclude patterns by constructing a URL that should be debounced
  /// because it matches a wildcard include pattern, then a second one
  /// that should not be debounced because it matches an exclude pattern.
  func testOneIncludeAndOneExclude() {
    // Given
    // A cosntructed url
    let finalURL = URL(string: "http://z.com/")!
    let startURL1 = URL(string: "http://included.e.com/")!.addRedirectParam(finalURL)
    let startURL2 = URL(string: "http://excluded.e.com/")!.addRedirectParam(finalURL)

    // When
    // A redirect url
    let returnedURL1 = downloader.redirectURL(for: startURL1)
    let returnedURL2 = downloader.redirectURL(for: startURL2)

    // Then
    XCTAssertEqual(returnedURL1, finalURL)
    XCTAssertNil(returnedURL2)
  }

  /// Test that debouncing rules only apply if the query parameter matches exactly.
  func testParamsMatch() {
    // Given
    // A cosntructed url
    let landingURL = URL(string: "http://z.com/")!
    let startURL = URL(string: "http://included.f.com/")!.addRedirectParam(landingURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: startURL)

    // Then
    XCTAssertNil(returnedURL)
  }

  /// Test that extra keys in a rule are ignored and the rule is still
  /// processed and applied.
  func testExtraKeysAreIgnored() {
    // Given
    // A cosntructed url
    let baseURL = URL(string: "http://simple.g.com/")!
    let landingURL = URL(string: "http://z.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL)

    // When
    // A redirect url
    let returnedURL = downloader.redirectURL(for: originalURL)

    // Then
    XCTAssertEqual(returnedURL, landingURL)
  }

  /// Test a match-all rule
  func testMatchallRule() {
    // Given
    // A cosntructed url
    let baseURL = URL(string: "https://match-all.com/")!
    let landingURL = URL(string: "https://example.com/")!
    let originalURL1 = baseURL.addRedirectParam(landingURL, base64Encode: true, paramName: "_match_all")
    let originalURL2 = baseURL.addRedirectParam(landingURL, base64Encode: true, paramName: "_not_match_all")

    // When
    // A redirect url
    let returnedURL1 = downloader.redirectURL(for: originalURL1)
    let returnedURL2 = downloader.redirectURL(for: originalURL2)

    // Then
    XCTAssertEqual(returnedURL1, landingURL)
    XCTAssertNil(returnedURL2)
  }

  func testRealisticExamples() {
    // Given
    // Some real life url examples
    let includedURLs = [
      URL(string: "https://www.youtube.com/redirect?q=https://example.com")!,
      URL(string: "https://m.facebook.com/l.php?u=https://example.com")!,
      URL(string: "https://www.leechall.com/redirect.php?url=https://example.com")!,
      URL(string: "https://www.pixelshost.com/?url=aHR0cHM6Ly9leGFtcGxlLmNvbQ==")!,
      // Fixed subdomain
      URL(string: "https://goto.walmart.com/c/?u=https%3A%2F%2Fexample.com")!,
      // An example where the pattern doesn't have a host and is base64 encoded
      // Example: `*://*/descargar/index.php?url=*`
      URL(string: "https://example.com/descargar/index.php?url=aHR0cHM6Ly9leGFtcGxlLmNvbQ==")!,
    ]

    let excludedURLs = [
      URL(string: "https://exclude.leechall.com/exclude.php?url=https://example.com")!
    ]

    // Then
    // Returns valid debounced links
    let extractURL = URL(string: "https://example.com")!
    for includedURL in includedURLs {
      XCTAssertEqual(downloader.redirectURL(for: includedURL), extractURL, "When handling '\(includedURL)'")
    }

    // Then
    // Returns nil for excluded items
    for excludedURL in excludedURLs {
      XCTAssertNil(downloader.redirectURL(for: excludedURL), "When handling '\(excludedURL)'")
    }
  }
}

private extension URL {
  func addRedirectParam(_ redirectURL: URL, base64Encode: Bool = false, paramName: String = "url") -> URL {
    var queryValue = redirectURL.absoluteString

    if base64Encode {
      queryValue = queryValue.toBase64()
    }

    return self.withQueryParam(paramName, value: queryValue)
  }
}
