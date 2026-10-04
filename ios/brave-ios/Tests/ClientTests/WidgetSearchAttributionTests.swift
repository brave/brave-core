// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

final class WidgetSearchAttributionTests: XCTestCase {

  func testApplyWidgetSearchAttribution_replacesIosSourceOnBraveSearch() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=ios")!
    XCTAssertEqual(
      OpenSearchEngine.applyWidgetSearchAttribution(to: url)?.absoluteString,
      "https://search.brave.com/search?q=test&source=ios-widget"
    )
  }

  func testApplyWidgetSearchAttribution_isCaseInsensitiveOnHost() {
    let url = URL(string: "https://Search.Brave.com/search?q=test&source=ios")!
    let result = OpenSearchEngine.applyWidgetSearchAttribution(to: url)!
    let components = URLComponents(url: result, resolvingAgainstBaseURL: false)!
    XCTAssertTrue(
      components.queryItems!.contains(URLQueryItem(name: "source", value: "ios-widget"))
    )
  }

  func testApplyWidgetSearchAttribution_preservesQueryOrder() {
    let url = URL(string: "https://search.brave.com/search?source=ios&q=test")!
    XCTAssertEqual(
      OpenSearchEngine.applyWidgetSearchAttribution(to: url)?.absoluteString,
      "https://search.brave.com/search?source=ios-widget&q=test"
    )
  }

  func testApplyWidgetSearchAttribution_leavesAlreadyWidgetSourceUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=ios-widget")!
    XCTAssertEqual(OpenSearchEngine.applyWidgetSearchAttribution(to: url), url)
  }

  func testApplyWidgetSearchAttribution_leavesUrlWithoutSourceParamUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test")!
    XCTAssertEqual(OpenSearchEngine.applyWidgetSearchAttribution(to: url), url)
  }

  func testApplyWidgetSearchAttribution_leavesOtherSourceValuesUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=spotlight")!
    XCTAssertEqual(OpenSearchEngine.applyWidgetSearchAttribution(to: url), url)
  }

  func testApplyWidgetSearchAttribution_leavesNonBraveHostsUnchanged() {
    let url = URL(string: "https://www.google.com/search?q=test&source=ios")!
    XCTAssertEqual(OpenSearchEngine.applyWidgetSearchAttribution(to: url), url)
  }

  func testApplyWidgetSearchAttribution_doesNotMatchSpoofedHosts() {
    let url = URL(string: "https://search.brave.com.evil.com/search?q=test&source=ios")!
    XCTAssertEqual(OpenSearchEngine.applyWidgetSearchAttribution(to: url), url)
  }
}
