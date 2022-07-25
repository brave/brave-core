// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BigNumber
@testable import BraveWallet

class WeiFormatterTests: XCTestCase {
  func testWeiToDecimal() throws {
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 5))
    // Above 0
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "31050382080585020020", decimals: 18)), "31.05038")
    // Below 0
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "50382080585020020", decimals: 18)), "0.05038")
    // Different decimals
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "50382080585020020", decimals: 10)), "5038208.05850")
    // Hex
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "16345785d8a0000", radix: .hex, decimals: 18)), "0.10000")
  }
  func testWeiToBalance() throws {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    // Round up
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "31050382080585020020", decimals: 18)), "31.0504")
    // Round down
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "31050332080585020020", decimals: 18)), "31.0503")
  }
  func testWeiToGasFee() throws {
    let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: "100"))
    XCTAssertEqual(try XCTUnwrap(formatter.decimalString(for: "31050382080585020020", decimals: 18)), "3105.038208")
  }
  func testInvalidString() {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    XCTAssertNil(formatter.decimalString(for: "0x429d069189e0000", radix: .decimal, decimals: 18))
    XCTAssertNil(formatter.decimalString(for: "", decimals: 18))
    XCTAssertNil(formatter.decimalString(for: "hello, world", radix: .hex, decimals: 18))
  }
  func testDecimalToWei() throws {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    XCTAssertEqual(try XCTUnwrap(formatter.weiString(from: "1", radix: .decimal, decimals: 18)), "1000000000000000000")
    XCTAssertEqual(try XCTUnwrap(formatter.weiString(from: 1.0, radix: .decimal, decimals: 18)), "1000000000000000000")
    // Maximum number of digits
    XCTAssertEqual(try XCTUnwrap(formatter.weiString(from: "0.000000000000000001", radix: .decimal, decimals: 18)), "1")
    // Invalid number of digits
    XCTAssertNil(formatter.weiString(from: "0.000000000000000001", radix: .decimal, decimals: 10))
    // Hex
    XCTAssertEqual(try XCTUnwrap(formatter.weiString(from: 0.1, radix: .hex, decimals: 18)), "16345785d8a0000")
  }
  
  func testTrimmingTrailingZeros() {
    typealias ValueExpected = (value: String, expected: String)
    let testCases: [ValueExpected] = [
      ValueExpected(value: "0", expected: "0"),
      ValueExpected(value: "100", expected: "100"),
      ValueExpected(value: "100001.1", expected: "100001.1"),
      ValueExpected(value: "10001.1", expected: "10001.1"),
      ValueExpected(value: "0.0000000000001", expected: "0.0000000000001"),
      ValueExpected(value: "123456789.0", expected: "123456789"),
      ValueExpected(value: "12345678.90", expected: "12345678.9"),
      ValueExpected(value: "1234567.890", expected: "1234567.89"),
      ValueExpected(value: "123456.7890", expected: "123456.789"),
      ValueExpected(value: "12345.67890", expected: "12345.6789"),
      ValueExpected(value: "0.000000000000000000", expected: "0"),
      ValueExpected(value: "1.000000000000000000", expected: "1"),
      ValueExpected(value: "0.112233440000000000", expected: "0.11223344"),
      ValueExpected(value: "1122.33440000000000000", expected: "1122.3344"),
      ValueExpected(value: "12345.112233440000000000", expected: "12345.11223344"),
      ValueExpected(value: "12345.6789", expected: "12345.6789")
    ]
    testCases.forEach {
      XCTAssertEqual($0.value.trimmingTrailingZeros, $0.expected)
    }
  }
  
  func testDecimalToLamports() {
    let decimalInputString = "1"
    let lamportsValueString = "1000000000"
    let lamports = WeiFormatter.decimalToLamports(decimalInputString)
    
    XCTAssertNotNil(UInt64(lamportsValueString))
    XCTAssertNotNil(lamports)
    XCTAssertEqual(UInt64(lamportsValueString)!, lamports!)
  }
}
