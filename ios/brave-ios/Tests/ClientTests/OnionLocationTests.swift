// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web
import XCTest

@MainActor
final class OnionLocationTests: XCTestCase {
  func testRegularAndPrivateTabsRecognizeBraveComWithoutSharingMetadata() throws {
    let regular = FakeTabState()
    let privateTab = FakeTabState()
    privateTab.profile = privateTab.profile.offTheRecordProfile
    XCTAssertFalse(regular.isPrivate)
    XCTAssertTrue(privateTab.isPrivate)
    let source = URL(string: "https://brave.com/")!
    let onion = URL(string:
      "https://brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion/index.html"
    )!
    let response = try XCTUnwrap(HTTPURLResponse(
      url: source, statusCode: 200, httpVersion: nil,
      headerFields: ["Onion-Location": onion.absoluteString]
    ))
    regular.receiveOnionLocationResponse(response, isMainFrame: true)
    XCTAssertNil(regular.onionLocation)
    regular.commitOnionLocation(for: source)
    XCTAssertEqual(regular.onionLocation, onion)
    XCTAssertNil(privateTab.onionLocation)
    privateTab.receiveOnionLocationResponse(response, isMainFrame: false)
    privateTab.commitOnionLocation(for: source)
    XCTAssertNil(privateTab.onionLocation)
    privateTab.receiveOnionLocationResponse(response, isMainFrame: true)
    privateTab.commitOnionLocation(for: source)
    XCTAssertEqual(privateTab.onionLocation, onion)
    regular.clearOnionLocation()
    XCTAssertNil(regular.onionLocation)
    XCTAssertEqual(privateTab.onionLocation, onion)
    privateTab.clearOnionLocation()
    privateTab.receiveOnionLocationResponse(response, isMainFrame: true)
    privateTab.commitOnionLocation(for: URL(string: "https://other.example/"))
    XCTAssertNil(privateTab.onionLocation)
  }

  func testInsecureResponsesCannotAdvertiseOnionLocationInRegularMode() throws {
    let tab = FakeTabState()
    let source = URL(string: "http://brave.com/")!
    let response = try XCTUnwrap(HTTPURLResponse(
      url: source, statusCode: 200, httpVersion: nil,
      headerFields: ["Onion-Location": "http://example.onion/"]
    ))
    tab.receiveOnionLocationResponse(response, isMainFrame: true)
    tab.commitOnionLocation(for: source)
    XCTAssertNil(tab.onionLocation)
  }
}
