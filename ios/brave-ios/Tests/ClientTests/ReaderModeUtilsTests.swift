// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Brave

final class ReaderModeUtilsTests: XCTestCase {

  // MARK: - %READER-DIRECTION%

  /// The direction is substituted into the `<body dir="...">` attribute, so an attacker controlled
  /// value must be HTML entity encoded to avoid breaking out of the attribute
  func testDirectionAttributeInjectionIsEntityEncoded() async throws {
    let result = try makeReadabilityResult(direction: "\" onfocus=\"alert(1)")
    let html = try await generateReaderContent(result)
    XCTAssertTrue(html.contains("<body dir=\"&quot; onfocus=&quot;alert(1)\">"))
  }

  func testDirectionMarkupInjectionIsEntityEncoded() async throws {
    let result = try makeReadabilityResult(direction: "\"><script>alert(1)</script>")
    let html = try await generateReaderContent(result)
    XCTAssertTrue(
      html.contains("<body dir=\"&quot;&gt;&lt;script&gt;alert(1)&lt;/script&gt;\">")
    )
    // Only the legitimate nonce'd script element from the template should be present
    XCTAssertEqual(countOccurrences(of: "</script>", in: html), 1)
  }

  func testValidDirection() async throws {
    let result = try makeReadabilityResult(direction: "rtl")
    let html = try await generateReaderContent(result)
    XCTAssertTrue(html.contains("<body dir=\"rtl\">"))
  }

  // MARK: - %READER-ORIGINAL-PAGE-META-TAGS%

  /// Each CSP meta tag value is substituted into a `content="..."` attribute, so an attacker
  /// controlled value must be HTML entity encoded to avoid breaking out into the <head>
  func testCSPMetaTagInjectionIsEntityEncoded() async throws {
    let result = try makeReadabilityResult(
      cspMetaTags: ["default-src 'self'\"><script>alert(1)</script>"]
    )
    let html = try await generateReaderContent(result)
    XCTAssertTrue(
      html.contains(
        "<meta http-equiv=\"Content-Security-Policy\" "
          + "content=\"default-src &#39;self&#39;&quot;&gt;&lt;script&gt;alert(1)&lt;/script&gt;\">"
      )
    )
    XCTAssertEqual(countOccurrences(of: "</script>", in: html), 1)
  }

  func testValidCSPMetaTags() async throws {
    let result = try makeReadabilityResult(
      cspMetaTags: ["default-src 'none'; script-src 'none'"]
    )
    let html = try await generateReaderContent(result)
    XCTAssertTrue(
      html.contains(
        "<meta http-equiv=\"Content-Security-Policy\" "
          + "content=\"default-src &#39;none&#39;; script-src &#39;none&#39;\">"
      )
    )
  }

  // MARK: - %READER-PAGE-LANGUAGE%

  func testValidPageLanguage() async throws {
    let result = try makeReadabilityResult(documentLanguage: "en")
    let html = try await generateReaderContent(result)
    XCTAssertTrue(html.contains("<html lang=\"en\">"))
  }

  /// Non-ISO language codes are dropped entirely rather than escaped
  func testInvalidPageLanguageIsIgnored() async throws {
    let result = try makeReadabilityResult(
      documentLanguage: "en\"><script>alert(1)</script>"
    )
    let html = try await generateReaderContent(result)
    XCTAssertTrue(html.contains("<html lang=\"\">"))
    XCTAssertEqual(countOccurrences(of: "</script>", in: html), 1)
  }

  // MARK: - %READER-TITLE% / %READER-CREDITS%

  /// Titles and credits are substituted into JavaScript string literals inside the nonce'd script
  /// element, so they must remain JavaScript escaped and must not terminate the script element
  func testTitleAndCreditsCannotEscapeScriptElement() async throws {
    let payload = "</script><script>alert(1)</script>"
    let result = try makeReadabilityResult(title: payload, credits: payload)
    let html = try await generateReaderContent(result)
    XCTAssertFalse(html.contains(payload))
    XCTAssertTrue(html.contains("<\\/script><script>alert(1)<\\/script>"))
    XCTAssertEqual(countOccurrences(of: "</script>", in: html), 1)
  }

  func testTitleQuotesAreJavaScriptEscaped() async throws {
    let result = try makeReadabilityResult(title: "He said \"hi\"")
    let html = try await generateReaderContent(result)
    XCTAssertTrue(html.contains("He said \\\"hi\\\""))
  }

  // MARK: - Helpers

  private func makeReadabilityResult(
    title: String = "Example Article",
    credits: String = "John Doe",
    direction: String = "ltr",
    documentLanguage: String = "en",
    cspMetaTags: [String] = [],
    content: String = "<p>Hello World</p>"
  ) throws -> ReadabilityResult {
    let object: NSDictionary = [
      "uri": ["spec": "https://example.com/article", "host": "example.com"],
      "title": title,
      "byline": credits,
      "dir": direction,
      "documentLanguage": documentLanguage,
      "cspMetaTags": cspMetaTags,
      "content": content,
    ]
    return try XCTUnwrap(ReadabilityResult(object: object))
  }

  private func generateReaderContent(
    _ result: ReadabilityResult,
    nonce: String = "test-nonce"
  ) async throws -> String {
    let content = await ReaderModeUtils.generateReaderContent(
      result,
      initialStyle: defaultReaderModeStyle,
      titleNonce: nonce
    )
    return try XCTUnwrap(content)
  }

  private func countOccurrences(of substring: String, in string: String) -> Int {
    string.components(separatedBy: substring).count - 1
  }
}
