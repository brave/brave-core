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

  /// Splits a serialized CSP into its comma-separated policies, each parsed into directives
  private func policies(of csp: String) -> [[(String, String)]] {
    csp.components(separatedBy: ",").map { directives(of: $0) }
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
        "script-src-attr 'unsafe-inline'; script-src-elem *; frame-src internal: *; script-src 'unsafe-inline'; style-src *"
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

  /// A bare `img-src` is a valid directive with an empty source list which blocks all image
  /// loads. It must be preserved rather than falling back to the permissive `img-src *`
  func testOriginalCSPEmptyImgSrcIsAdopted() {
    let directives = directives(of: makeCSP(originalCSP: "img-src"))
    let imgSrc = directives.filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "")
  }

  /// Directive tokens may be separated by any whitespace, not just spaces
  func testOriginalCSPImgSrcTabSeparatedIsAdopted() {
    let directives = directives(of: makeCSP(originalCSP: "img-src\thttps:"))
    let imgSrc = directives.filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "https:")
  }

  /// CSP uses the first occurrence of a directive and ignores later duplicates, so a page
  /// cannot loosen its own restrictive `img-src` by appending a second one
  func testOriginalCSPDuplicateImgSrcUsesFirstOccurrence() {
    let directives = directives(
      of: makeCSP(originalCSP: "img-src 'none'; img-src *")
    )
    let imgSrc = directives.filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "'none'")
  }

  /// `allHeaderFields` coalesces repeated CSP headers into a comma-separated policy list, so an
  /// `img-src` appearing after the comma must still be adopted
  func testOriginalCSPImgSrcInSecondPolicyIsAdopted() {
    let policies = policies(
      of: makeCSP(originalCSP: "default-src 'none', img-src https:")
    )
    XCTAssertEqual(policies.count, 1)
    let imgSrc = policies[0].filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "https:")
  }

  /// Directives from a subsequent coalesced policy must not leak into the base policy
  func testOriginalCSPSecondPolicyDirectivesAreDropped() {
    let policies = policies(
      of: makeCSP(originalCSP: "img-src https:, default-src 'none'")
    )
    XCTAssertEqual(policies.count, 1)
    let imgSrc = policies[0].filter { $0.0 == "img-src" }
    XCTAssertEqual(imgSrc.count, 1)
    XCTAssertEqual(imgSrc.first?.1, "https:")
  }

  /// When multiple coalesced policies each specify `img-src`, the restrictions must intersect,
  /// so each additional `img-src` is emitted as its own policy
  func testOriginalCSPImgSrcInMultiplePoliciesIntersect() {
    let policies = policies(
      of: makeCSP(originalCSP: "img-src data:, img-src https:")
    )
    XCTAssertEqual(policies.count, 2)
    XCTAssertEqual(policies[0].filter { $0.0 == "img-src" }.first?.1, "data:")
    XCTAssertEqual(policies[1].count, 1)
    XCTAssertEqual(policies[1].first?.0, "img-src")
    XCTAssertEqual(policies[1].first?.1, "https:")
  }
}
