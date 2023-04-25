// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BigNumber
import BraveCore
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
  
  func testDecimalToAmount() {
    let decimalInputString = "1"
    let solValueString = "1000000000"
    let spdValueString = "1000000"
    
    let solFormatted = WeiFormatter.decimalToAmount(decimalInputString, tokenDecimals: Int(BraveWallet.BlockchainToken.mockSolToken.decimals))
    
    XCTAssertNotNil(UInt64(solValueString))
    XCTAssertNotNil(solFormatted)
    XCTAssertEqual(UInt64(solValueString)!, solFormatted!)
    
    let spdformatted = WeiFormatter.decimalToAmount(decimalInputString, tokenDecimals: Int(BraveWallet.BlockchainToken.mockSpdToken.decimals))
    
    XCTAssertNotNil(UInt64(spdValueString))
    XCTAssertNotNil(spdformatted)
    XCTAssertEqual(UInt64(spdValueString)!, spdformatted!)
  }
  
  func testDoubleDecimalPlaces() {
    typealias ValueExpected = (value: Double, expected: Int)
    let testCases: [ValueExpected] = [
      ValueExpected(value: 1.2345, expected: 4),
      ValueExpected(value: 12.345, expected: 3),
      ValueExpected(value: 1.23450, expected: 4),
      ValueExpected(value: 1.234500, expected: 4),
      ValueExpected(value: 1.23456, expected: 5),
      ValueExpected(value: 1.23456789, expected: 8),
      ValueExpected(value: -1.2345, expected: 4),
      ValueExpected(value: -1.23456, expected: 5),
    ]
    testCases.forEach {
      XCTAssertEqual($0.value.decimalPlaces, $0.expected)
    }
  }
  
  func testCoinMarketPriceString() {
    let testFormatter: NumberFormatter = .usdCurrencyFormatter
    typealias ValueExpected = (value: Double, expected: String)
    let testCases: [ValueExpected] = [
      ValueExpected(value: 29399.123, expected: "$29,399.12"),
      ValueExpected(value: 1.001, expected: "$1.001"),
      ValueExpected(value: 0.999307, expected: "$0.999307"),
      ValueExpected(value: 0.091685, expected: "$0.091685"),
      ValueExpected(value: 0.00001113, expected: "$0.00001113"),
    ]
    testCases.forEach {
      XCTAssertEqual(testFormatter.coinMarketPriceString(from: $0.value), $0.expected)
    }
  }
}
