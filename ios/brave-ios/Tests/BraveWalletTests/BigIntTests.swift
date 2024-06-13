// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import XCTest

@testable import BraveWallet

class BigIntTests: XCTestCase {
  /// Test fix for BDouble initialization from Double in scientific notation.
  /// Fixed in v2.2.1+ https://github.com/mkrd/Swift-BigInt/pull/66
  func testBDoubleInitFromScientificNotation() {
    let doubleValue: Double = 5.779e-05  // 0.00005779
    let bdoubleValue = BDouble(doubleValue)
    XCTAssertEqual(bdoubleValue.decimalExpansion(precisionAfterDecimalPoint: 10), "0.0000577900")
  }
}
