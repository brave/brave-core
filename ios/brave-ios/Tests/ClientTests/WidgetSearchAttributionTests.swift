// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

final class WidgetSearchAttributionTests: XCTestCase {

  func testApplyWidgetAttribution_replacesIosSourceOnBraveSearch() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=ios")!
    XCTAssertEqual(
      WidgetSearchTabHelper.applyWidgetAttribution(url).absoluteString,
      "https://search.brave.com/search?q=test&source=ios-widget"
    )
  }

  func testApplyWidgetAttribution_isCaseInsensitiveOnHost() {
    let url = URL(string: "https://Search.Brave.com/search?q=test&source=ios")!
    let result = WidgetSearchTabHelper.applyWidgetAttribution(url)
    let components = URLComponents(url: result, resolvingAgainstBaseURL: false)!
    XCTAssertTrue(
      components.queryItems!.contains(URLQueryItem(name: "source", value: "ios-widget"))
    )
  }

  func testApplyWidgetAttribution_preservesQueryOrder() {
    let url = URL(string: "https://search.brave.com/search?source=ios&q=test")!
    XCTAssertEqual(
      WidgetSearchTabHelper.applyWidgetAttribution(url).absoluteString,
      "https://search.brave.com/search?source=ios-widget&q=test"
    )
  }

  func testApplyWidgetAttribution_leavesAlreadyWidgetSourceUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=ios-widget")!
    XCTAssertEqual(WidgetSearchTabHelper.applyWidgetAttribution(url), url)
  }

  func testApplyWidgetAttribution_leavesUrlWithoutSourceParamUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test")!
    XCTAssertEqual(WidgetSearchTabHelper.applyWidgetAttribution(url), url)
  }

  func testApplyWidgetAttribution_leavesOtherSourceValuesUnchanged() {
    let url = URL(string: "https://search.brave.com/search?q=test&source=spotlight")!
    XCTAssertEqual(WidgetSearchTabHelper.applyWidgetAttribution(url), url)
  }

  func testApplyWidgetAttribution_leavesNonBraveHostsUnchanged() {
    let url = URL(string: "https://www.google.com/search?q=test&source=ios")!
    XCTAssertEqual(WidgetSearchTabHelper.applyWidgetAttribution(url), url)
  }

  func testApplyWidgetAttribution_doesNotMatchSpoofedHosts() {
    let url = URL(string: "https://search.brave.com.evil.com/search?q=test&source=ios")!
    XCTAssertEqual(WidgetSearchTabHelper.applyWidgetAttribution(url), url)
  }
}
