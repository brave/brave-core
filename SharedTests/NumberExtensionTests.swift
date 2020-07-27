// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Shared

class NumberExtensionTests: XCTestCase {

    func testKFormattedNumber() {
        XCTAssertEqual(0.kFormattedNumber, "0")
        XCTAssertEqual(100.kFormattedNumber, "100")
        XCTAssertEqual(999.kFormattedNumber, "999")
        XCTAssertEqual(1000.kFormattedNumber, "1K")
        XCTAssertEqual(1599.kFormattedNumber, "1.5K")
        XCTAssertEqual(123_456.kFormattedNumber, "123K")
        XCTAssertEqual(1_000_000.kFormattedNumber, "1M")
        XCTAssertEqual(123_456_7.kFormattedNumber, "1.2M")
        XCTAssertEqual(999_999.kFormattedNumber, "999K")
    }
    
    func testFrontSymbolCurrencyFormatted() {
        let number = NSDecimalNumber(floatLiteral: 12.34)
        let usFormatted = number.frontSymbolCurrencyFormatted(with: Locale(identifier: "en_US"))
        let plFormatted = number.frontSymbolCurrencyFormatted(with: Locale(identifier: "pl_PL"))
        
        XCTAssertEqual(usFormatted, "$12.34")
        XCTAssertNotEqual(usFormatted, "12.34$")
        
        XCTAssertEqual(plFormatted, "zł12,34")
    }
}
