// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Shared
import XCTest

@testable import Brave

class BraveSearchVerticalTests: XCTestCase {

  func testSubstitutesQueryIntoTemplate() {
    let vertical = BraveSearchVertical(kind: .images)

    XCTAssertEqual(
      vertical.searchURL(for: "puppies")?.absoluteString,
      "https://search.brave.com/images?q=puppies&source=ios"
    )
  }

  func testEscapesQuery() {
    let vertical = BraveSearchVertical(kind: .videos)

    XCTAssertEqual(
      vertical.searchURL(for: "cats & dogs")?.absoluteString,
      "https://search.brave.com/videos?q=cats%20%26%20dogs&source=ios"
    )
  }

  func testEveryKindBuildsAURL() {
    for kind in BraveSearchVertical.Kind.allCases {
      XCTAssertNotNil(
        BraveSearchVertical(kind: kind).searchURL(for: "puppies"),
        "\(kind) built no URL"
      )
    }
  }

  // MARK: - Widget search attribution

  /// Attribution is silent when it fails — `applyWidgetSearchAttribution` returns the URL
  /// untouched unless `source=ios` is already there — so every vertical is checked.
  func testWidgetAttributionRewritesSourceForEveryKind() {
    for kind in BraveSearchVertical.Kind.allCases {
      let url = BraveSearchVertical(kind: kind)
        .searchURL(for: "puppies", isWidgetSearchAttribution: true)

      XCTAssertEqual(
        url?.getQuery()["source"],
        "ios-widget",
        "\(kind) did not pick up widget attribution"
      )
    }
  }

  func testLeavesSourceAloneWithoutWidgetAttribution() {
    let url = BraveSearchVertical(kind: .images)
      .searchURL(for: "puppies", isWidgetSearchAttribution: false)

    XCTAssertEqual(url?.getQuery()["source"], "ios")
  }
}
