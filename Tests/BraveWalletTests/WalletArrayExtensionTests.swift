// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

class WalletArrayExtensionTests: XCTestCase {
  func testOddLengthInput() {
    let inputHexData = "0xff573"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([15, 245, 115])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testEvenLengthInput() {
    let inputHexData = "0xff5733"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([255, 87, 51])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testNoHexPrefixInput() {
    let inputHexData = "ff5733"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([255, 87, 51])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testInvalidHexInput() {
    let inputHexData = "0x0csadgasg"
    let bytes = [NSNumber](hexString: inputHexData)
    XCTAssertNil(bytes)
  }

  func testEmptyHexInput() {
    let inputHexData = "0x"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init()
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }
}
