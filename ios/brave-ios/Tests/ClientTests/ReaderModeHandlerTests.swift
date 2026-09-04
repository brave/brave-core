// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Brave

final class ReaderModeHandlerTests: XCTestCase {

  private func directives(
    of csp: String
  ) -> [(String, String)] {
    csp.components(separatedBy: ";")
      .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
      .filter { !$0.isEmpty }
      .map { directive in
        let components = directive.components(separatedBy: " ").filter { !$0.isEmpty }
        return (components[0], components.dropFirst().joined(separator: " "))
      }
  }

  private func makeCSP(originalCSP: String? = nil, nonce: String = "test-nonce") -> String {
    ReaderModeHandler.contentSecurityPolicy(originalCSP: originalCSP, scriptNonce: nonce)
  }

  func testBasePolicyWithoutOriginalCSP() {
    let directives = directives(of: makeCSP())
    XCTAssertTrue(directives.contains(where: { $0 == ("script-src", "'nonce-test-nonce'") }))
    XCTAssertTrue(directives.contains(where: { $0 == ("default-src", "'none'") }))
    XCTAssertTrue(directives.contains(where: { $0 == ("frame-ancestors", "'none'") }))
    XCTAssertTrue(directives.contains(where: { $0 == ("img-src", "*") }))
  }

  /// The original page's CSP must not be able to weaken the reader mode policy. Directives such
  /// as `script-src-attr` take precedence over the nonce-based `script-src` for inline event
  /// handlers, and `frame-src` would permit framing privileged internal pages.
  func testOriginalCSPDirectivesOtherThanImgSrcAreDropped() {
    let csp = makeCSP(
      originalCSP:
        "script-src-attr 'unsafe-inline'; frame-src internal: *; script-src 'unsafe-inline'; style-src *"
    )
    let directives = directives(of: csp)
    XCTAssertFalse(directives.contains(where: { $0.0 == "script-src-attr" }))
    XCTAssertFalse(directives.contains(where: { $0.0 == "script-src-elem" }))
    XCTAssertFalse(directives.contains(where: { $0.0 == "frame-src" }))
    XCTAssertFalse(directives.contains(where: { $0.0 == "style-src" && $0.1 == "*" }))
    // Our own directives must remain intact
    XCTAssertTrue(directives.contains(where: { $0 == ("script-src", "'nonce-test-nonce'") }))
    XCTAssertTrue(directives.contains(where: { $0 == ("default-src", "'none'") }))
  }

  /// The `img-src` directive is the only one adopted from the original page's CSP, and it
  /// replaces our own default `img-src *`
  func testOriginalCSPImgSrcIsAdopted() {
    let directives = directives(
      of: makeCSP(originalCSP: "img-src https: data:; default-src 'self'")
    )
    let imgSrc = directives.filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "https: data:")
    XCTAssertFalse(directives.contains(where: { $0.0 == "default-src" && $0.1 == "'self'" }))
  }

  /// Directive names are matched case-insensitively
  func testOriginalCSPImgSrcCaseInsensitive() {
    let directives = directives(of: makeCSP(originalCSP: "IMG-SRC https:"))
    let imgSrc = directives.filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "https:")
  }

  /// A malformed directive must not be able to smuggle in additional directives or remove our
  /// default `img-src`
  func testOriginalCSPMalformedImgSrcIsIgnored() {
    let directives = directives(of: makeCSP(originalCSP: "img-src"))
    XCTAssertTrue(directives.contains(where: { $0 == ("img-src", "*") }))
  }
}
