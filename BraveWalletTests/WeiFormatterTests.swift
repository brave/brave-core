// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
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
}
