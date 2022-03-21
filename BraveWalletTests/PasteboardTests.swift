// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

class PasteboardTests: XCTestCase {
  func testExpiringSecureString() {
    let expectation = expectation(description: "pasteboard-expiration")
    let pasteboard = UIPasteboard.withUniqueName()
    let data = "test"
    pasteboard.setSecureString(data, expirationDate: Date().addingTimeInterval(0.5))
    XCTAssertEqual(pasteboard.string, data)
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      defer { expectation.fulfill() }
      // After expiration the string should be nil
      XCTAssertNil(pasteboard.string)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
}
