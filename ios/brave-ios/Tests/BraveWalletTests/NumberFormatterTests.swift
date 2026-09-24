// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import BraveWallet

class NumberFormatterTests: XCTestCase {

  func testFormatAsFiat() {
    typealias ValueExpected = (value: Double, expected: String)
    let currencyFormatter = NumberFormatter.usdCurrencyFormatter
    let testCases: [ValueExpected] = [
      // minimum 2 digits after decimal
      ValueExpected(value: 0, expected: "$0.00"),
      ValueExpected(value: 1, expected: "$1.00"),
      ValueExpected(value: 100, expected: "$100.00"),
      ValueExpected(value: 100001.1, expected: "$100,001.10"),
      ValueExpected(value: 10001.1, expected: "$10,001.10"),
      // significant digits for values < 1
      ValueExpected(value: 0.0000000000001, expected: "$0.0000000000001"),
      ValueExpected(value: 0.0000123454321, expected: "$0.0000123"),
      // trailing zeros trimmed
      ValueExpected(value: 0.0010000, expected: "$0.001"),
      ValueExpected(value: 0.001230000, expected: "$0.00123"),
      ValueExpected(value: 0.0000000000001000, expected: "$0.0000000000001"),
      // Comma separated
      ValueExpected(value: 123456789.0, expected: "$123,456,789.00"),
      ValueExpected(value: 12345678.90, expected: "$12,345,678.90"),
      ValueExpected(value: 1234567.890, expected: "$1,234,567.89"),
      ValueExpected(value: 123456.7890, expected: "$123,456.79"),
      ValueExpected(value: 12345.67890, expected: "$12,345.68"),
    ]
    testCases.forEach {
      XCTAssertEqual(currencyFormatter.formatAsFiat($0.value), $0.expected)
    }
  }
}
