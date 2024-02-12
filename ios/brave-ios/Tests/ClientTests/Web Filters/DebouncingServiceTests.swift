// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

class DebouncingServiceTests: XCTestCase {
  private let service = DebouncingService()

  override func setUpWithError() throws {
    let bundle = Bundle.module
    let resourceURL = bundle.url(forResource: "debouncing", withExtension: "json")
    let data = try Data(contentsOf: resourceURL!)
    try service.setup(withRulesJSON: data)
  }

  /// Test simple redirection by query parameter
  func testSimpleRedirect() {
    // Given
    // A constructed url
    let baseURL = URL(string: "http://simple.a.com/")!
    let landingURL = URL(string: "http://simple.b.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL)
    let originalURL2 = baseURL.addRedirectParam(baseURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: originalURL)
    let redirectChain2 = service.redirectChain(for: originalURL2)

    // Then
    XCTAssertEqual(redirectChain.last?.url, landingURL)
    XCTAssertNil(
      redirectChain2.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
  }

  /// Test base64-encoded redirection by query parameter.
  func testBase64Redirect() {
    // Given
    // A constructed url
    let baseURL = URL(string: "http://base64.a.com/")!
    let landingURL = URL(string: "http://base64.b.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL, base64Encode: true)
    let originalURL2 = baseURL.addRedirectParam(baseURL, base64Encode: true)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: originalURL)
    let redirectChain2 = service.redirectChain(for: originalURL2)

    // Then
    XCTAssertEqual(redirectChain.last?.url, landingURL)
    XCTAssertNil(
      redirectChain2.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
  }

  /// Test that debounce rules continue to be processed in order
  /// by constructing a URL that should be debounced to a second
  /// URL that should then be debounced to a third URL.
  func testRedirectChain() {
    // Given
    // A constructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlB = URL(string: "http://double.b.com/")!.addRedirectParam(urlZ)
    let urlA = URL(string: "http://double.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: urlA)

    // Then
    XCTAssertEqual(redirectChain.last?.url, urlZ)
  }

  /// Test a long redirect chain.
  func testLongRedirectChain() {
    // Given
    // A constructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlD = URL(string: "http://quad.d.com/")!.addRedirectParam(urlZ)
    let urlC = URL(string: "http://quad.c.com/")!.addRedirectParam(urlD)
    let urlB = URL(string: "http://quad.b.com/")!.addRedirectParam(urlC)
    let urlA = URL(string: "http://quad.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: urlA)

    // Then
    XCTAssertEqual(redirectChain.last?.url, urlZ)
  }

  /// Test a redirect chain that bounces from a tracker to a final URL in the
  /// tracker's domain.
  func testRedirectChainInTrackersDomain() {
    // Given
    // A constructed url
    let finalURL = URL(string: "http://z.com/")!
    let intermediateURL = URL(string: "http://tracker.z.com/")!.addRedirectParam(finalURL)
    let startURL = URL(string: "http://origin.h.com/")!.addRedirectParam(intermediateURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: startURL)

    // Then
    // While we used to go to `finalURL`, we now stop at `intermediateURL`
    // This changed with the introduction of ticket #6146
    XCTAssertEqual(redirectChain.last?.url, intermediateURL)
  }

  // Test a long redirect chain that bounces through the original URL's domain,
  // and verify the SiteForCookies used for the debounced request.
  func testLongRedirectChainInTrackersDomain() {
    // Given
    // A constructed url
    let urlZ = URL(string: "http://z.com/")!
    let urlTrackerA = URL(string: "http://tracker.a.com/")!.addRedirectParam(urlZ)
    let urlD = URL(string: "http://quad.d.com/")!.addRedirectParam(urlTrackerA)
    let urlC = URL(string: "http://quad.c.com/")!.addRedirectParam(urlD)
    let urlB = URL(string: "http://quad.b.com/")!.addRedirectParam(urlC)
    let urlA = URL(string: "http://quad.a.com/")!.addRedirectParam(urlB)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: urlA)

    // Then
    XCTAssertEqual(redirectChain.last?.url, urlZ)
  }

  /// Test that debounce rules are not processed twice by constructing
  /// a URL that should be debounced to a second URL that would be
  /// debounced to a third URL except that that rule has already been
  /// processed, so it won't.
  func testDoubleBounce() {
    // Given
    // A constructed url
    let finalURL = URL(string: "http://z.com/")!
    let intermediateURL = URL(string: "http://double.a.com/")!.addRedirectParam(finalURL)
    let startURL = URL(string: "http://double.b.com/")!.addRedirectParam(intermediateURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: startURL)

    // Then
    // TODO: @JS Should be going to intermediateURL
    // Even if I add support for this on this method. The way that the implementation works for web-kit
    // It would just process it on the next webView.loadRequest navigation.
    XCTAssertEqual(redirectChain.last?.url, finalURL)
  }

  /// Test wildcard URL patterns by constructing a URL that should be
  /// debounced because it matches a wildcard include pattern.
  func testWildcardIncludePattern() {
    // Given
    // A constructed url
    let baseURL = URL(string: "http://included.c.com/")!
    let landingURL = URL(string: "http://z.com/")!
    let startURL = baseURL.addRedirectParam(landingURL)
    let startURL2 = baseURL.addRedirectParam(baseURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: startURL)
    let redirectChain2 = service.redirectChain(for: startURL2)

    // Then
    XCTAssertEqual(redirectChain.last?.url, landingURL)
    XCTAssertNil(
      redirectChain2.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
  }

  /// Test wildcard URL patterns by constructing a URL that should be
  /// debounced because it matches a wildcard include pattern.
  func testUnknownActionsAreIgnored() {
    // Given
    // A constructed url
    let landingURL = URL(string: "http://z.com/")!
    let startURL = URL(string: "http://included.d.com/")!.addRedirectParam(landingURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: startURL)

    // Then
    XCTAssertNil(redirectChain.last)
  }

  /// Test URL exclude patterns by constructing a URL that should be debounced
  /// because it matches a wildcard include pattern, then a second one
  /// that should not be debounced because it matches an exclude pattern.
  func testOneIncludeAndOneExclude() {
    // Given
    // A constructed url
    let finalURL = URL(string: "http://z.com/")!
    let startURL1 = URL(string: "http://included.e.com/")!.addRedirectParam(finalURL)
    let startURL2 = URL(string: "http://excluded.e.com/")!.addRedirectParam(finalURL)

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)
    let redirectChain2 = service.redirectChain(for: startURL2)

    // Then
    XCTAssertEqual(redirectChain1.last?.url, finalURL)
    XCTAssertNil(redirectChain2.last)
  }

  /// Test that debouncing rules only apply if the query parameter matches exactly.
  func testParamsMatch() {
    // Given
    // A constructed url
    let landingURL = URL(string: "http://z.com/")!
    let startURL = URL(string: "http://included.f.com/")!.addRedirectParam(landingURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: startURL)

    // Then
    XCTAssertNil(redirectChain.last)
  }

  /// Test that extra keys in a rule are ignored and the rule is still
  /// processed and applied.
  func testExtraKeysAreIgnored() {
    // Given
    // A constructed url
    let baseURL = URL(string: "http://simple.g.com/")!
    let landingURL = URL(string: "http://z.com/")!
    let originalURL = baseURL.addRedirectParam(landingURL)

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: originalURL)

    // Then
    XCTAssertEqual(redirectChain.last?.url, landingURL)
  }

  /// Test a match-all rule
  func testMatchallRule() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://match-all.com/")!
    let landingURL1 = URL(string: "https://example.com/")!
    let originalURL1 = baseURL.addRedirectParam(landingURL1, base64Encode: true, paramName: "_match_all")
    let originalURL2 = baseURL.addRedirectParam(landingURL1, base64Encode: true, paramName: "_not_match_all")

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: originalURL1)
    let redirectChain2 = service.redirectChain(for: originalURL2)

    // Then
    XCTAssertEqual(redirectChain1.last?.url, landingURL1)
    XCTAssertNil(redirectChain2.last)
  }
  
  /// Test regex rule where `prepend_scheme` is specified
  func testRegexRuleWithPrependScheme() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave-scheme.example")!
    let startURL1 = baseURL.appendingPathComponent("braveattentiontoken.example")
    let startURL2 = baseURL.appendingPathComponent("braveattentiontoken.example/")
    let startURL3 = baseURL.appendingPathComponent("://braveattentiontoken.example")
    let startURL4 = baseURL.appendingPathComponent("https://braveattentiontoken.example")
    let startURL5 = baseURL.appendingPathComponent("https://braveattentiontoken.example/")
    let startURL6 = baseURL.appendingPathComponent("https://brave-scheme.example/")
    let landingURL1 = URL(string: "https://braveattentiontoken.example")!
    let landingURL2 = URL(string: "https://braveattentiontoken.example/")!

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)
    let redirectChain2 = service.redirectChain(for: startURL2)
    let redirectChain3 = service.redirectChain(for: startURL3)
    let redirectChain4 = service.redirectChain(for: startURL4)
    let redirectChain5 = service.redirectChain(for: startURL5)
    let redirectChain6 = service.redirectChain(for: startURL6)

    // Then
    XCTAssertEqual(redirectChain1.last?.url, landingURL1)
    XCTAssertEqual(redirectChain2.last?.url, landingURL2)
    
    XCTAssertNil(
      redirectChain3.last?.url,
      "This should be nil because scheme is not there but we don't want to support ://some-url types of schemeless urls"
    )
    XCTAssertNil(
      redirectChain4.last,
      "This should be nil because a scheme is already on the raw value and the rule specifies a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain5.last,
      "This should be nil because a scheme is already on the raw value and the rule specifies a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain6.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified
  func testRegexRuleWithNoPrependScheme() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave.com")!
    let startURL1 = baseURL.appendingPathComponent("https://braveattentiontoken.example")
    let startURL2 = baseURL.appendingPathComponent("https://braveattentiontoken.example".addingPercentEncoding(withAllowedCharacters: .URLAllowed)!)
    let startURL3 = baseURL.appendingPathComponent("http://braveattentiontoken.example/")
    let startURL4 = baseURL.appendingPathComponent("http://93.184.216.34")
    let startURL5 = baseURL.appendingPathComponent("ftp://braveattentiontoken.example")
    let startURL6 = baseURL.appendingPathComponent("braveattentiontoken.example")
    let startURL7 = baseURL.appendingPathComponent("braveattentiontoken.example/")
    let startURL8 = baseURL.appendingPathComponent("://braveattentiontoken.example")
    let startURL9 = baseURL.appendingPathComponent(baseURL.absoluteString)
    let startURL10 = baseURL.appendingPathComponent("http://example")
    let landingURL1And2 = URL(string: "https://braveattentiontoken.example")
    let landingURL3 = URL(string: "http://braveattentiontoken.example/")
    let landingURL4 = URL(string: "http://93.184.216.34")

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)
    let redirectChain2 = service.redirectChain(for: startURL2)
    let redirectChain3 = service.redirectChain(for: startURL3)
    let redirectChain4 = service.redirectChain(for: startURL4)
    let redirectChain5 = service.redirectChain(for: startURL5)
    let redirectChain6 = service.redirectChain(for: startURL6)
    let redirectChain7 = service.redirectChain(for: startURL7)
    let redirectChain8 = service.redirectChain(for: startURL8)
    let redirectChain9 = service.redirectChain(for: startURL9)
    let redirectChain10 = service.redirectChain(for: startURL10)

    // Then
    XCTAssertEqual(redirectChain1.last?.url, landingURL1And2)
    XCTAssertEqual(redirectChain2.last?.url, landingURL1And2)
    XCTAssertEqual(redirectChain3.last?.url, landingURL3)
    XCTAssertEqual(redirectChain4.last?.url, landingURL4)
    
    XCTAssertNil(
      redirectChain5.last,
      "This should be nil because the scheme is not `http` or `https`"
    )
    XCTAssertNil(
      redirectChain6.last,
      "This should be nil because a scheme is missing and the rule doesn't specify a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain7.last,
      "This should be nil because a scheme is missing and the rule doesn't specify a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain8.last,
      "This should be nil because a scheme is missing and the rule doesn't specify a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain9.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
    XCTAssertNil(
      redirectChain10.last,
      "This should return nil because it is not a valid eTLD+1"
    )
  }
  
  /// Test several restrictions to debouncing
  func testDebounceRestrictions() {
    // Given
    // A constructed url
    let baseURL = URL(string: "http://a.click.example/bounce")!
    let startURL1 = baseURL.addRedirectParam(URL(string: "http://a.click.example/dest")!)
    let startURL2 = baseURL.addRedirectParam(URL(string: "http://b.click.example/dest")!)
    let startURL3 = baseURL.addRedirectParam(URL(string: "ftp://example.com/dest")!)

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)
    let redirectChain2 = service.redirectChain(for: startURL2)
    let redirectChain3 = service.redirectChain(for: startURL3)

    // Then
    XCTAssertNil(
      redirectChain1.last,
      "This should be nil because origin matches"
    )
    XCTAssertNil(
      redirectChain2.last,
      "This should be nil because eTLD+1 matches"
    )
    XCTAssertNil(
      redirectChain3.last,
      "This should be nil because the scheme is not http or https"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified and `base64` action is specified
  func testRegexRuleWithBase64DecodedURL() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave-base64.example/")!
    let startURL1 = baseURL.appendingPathComponent("https://braveattentiontoken.example".toBase64())
    let startURL2 = baseURL.appendingPathComponent("http://xyz".toBase64())
    let startURL3 = baseURL.appendingPathComponent("braveattentiontoken.example".toBase64())
    let startURL4 = baseURL.appendingPathComponent("https://brave-base64.example".toBase64())
    let startURL5 = baseURL.appendingPathComponent("https://braveattentiontoken.example")
    let landingURL1 = URL(string: "https://braveattentiontoken.example")!

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)
    let redirectChain2 = service.redirectChain(for: startURL2)
    let redirectChain3 = service.redirectChain(for: startURL3)
    let redirectChain4 = service.redirectChain(for: startURL4)
    let redirectChain5 = service.redirectChain(for: startURL5)

    // Then
    XCTAssertEqual(redirectChain1.last?.url, landingURL1)
    XCTAssertNil(
      redirectChain2.last?.url,
      "This should be nil because the host does not have 2 components separated by a ."
    )
    XCTAssertNil(
      redirectChain3.last,
      "This should be nil because a scheme is missing and the rule doesn't specify a `prepend_scheme`"
    )
    XCTAssertNil(
      redirectChain4.last,
      "This should return nil because the host of the source and destination urls is the same"
    )
    XCTAssertNil(
      redirectChain5.last,
      "This should return nil because the destination is not base64 encoded"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified and `base64` action is specified
  func testInvalidRegex() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave-invalid-regex.example/")!
    let startURL1 = baseURL.appendingPathComponent("https://braveattentiontoken.example")

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)

    // Then
    XCTAssertNil(
      redirectChain1.last?.url,
      "This should be nil because the regex rule is invalid"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified and `base64` action is specified
  func testRegexWithMultipleMatches() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave-multiple-matchers.example/")!
    let startURL1 = baseURL.appendingPathComponent("https://braveattentiontoken.example")

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)

    // Then
    XCTAssertNil(
      redirectChain1.last?.url,
      "This should be nil because the regex rule has multiple matches"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified and `base64` action is specified
  func testLongRegex() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave-long-regex.example/")!
    let startURL1 = baseURL.appendingPathComponent("https://lorem.example")

    // When
    // A redirect is returned
    let redirectChain1 = service.redirectChain(for: startURL1)

    // Then
    XCTAssertNil(
      redirectChain1.last?.url,
      "This should be nil because the regex rule has multiple matches"
    )
  }
  
  /// Test regex rule where `prepend_scheme` is not specified and `base64` action is specified
  func testRegexRuleWithInvalidURL() {
    // Given
    // A constructed url
    let baseURL = URL(string: "https://brave.com/xyz")!

    // When
    // A redirect is returned
    let redirectChain = service.redirectChain(for: baseURL)

    // Then
    XCTAssertNil(redirectChain.last)
  }

  /// Test a few realistic examples
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
      URL(string: "https://example2.com/descargar/index.php?url=aHR0cHM6Ly9leGFtcGxlLmNvbQ==")!,
    ]

    let excludedURLs = [
      URL(string: "https://exclude.leechall.com/exclude.php?url=https://example.com")!
    ]

    // Then
    // Returns valid debounced links
    let extractURL = URL(string: "https://example.com")!
    for includedURL in includedURLs {
      XCTAssertEqual(service.redirectChain(for: includedURL).last?.url, extractURL, "When handling '\(includedURL)'")
    }

    // Then
    // Returns nil for excluded items
    for excludedURL in excludedURLs {
      XCTAssertNil(service.redirectChain(for: excludedURL).last?.url, "When handling '\(excludedURL)'")
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
  
  var base64Encoded: String {
    self.absoluteString.toBase64()
  }
}
