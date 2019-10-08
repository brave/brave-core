/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import XCTest
@testable import BraveRewardsUI

class BATValueTests: XCTestCase {
  
  func testStringInitializer() {
    XCTAssertEqual(BATValue("0")?.displayString, "0.0")
    XCTAssertEqual(BATValue("1.0")?.displayString, "1.0")
    XCTAssertEqual(BATValue("20.99")?.displayString, "21.0")
    XCTAssertEqual(BATValue("-10.0")?.displayString, "-10.0")
    XCTAssertEqual(BATValue("-10")?.displayString, "-10.0")
    XCTAssertEqual(BATValue("-200")?.displayString, "-200.0")
    
    // Not numbers
    XCTAssertNil(BATValue("wrong_format"))
    XCTAssertNil(BATValue(""))
  }
  
  func testProbiInitializer() {
    let wrongFormat = "sdfs"
    XCTAssertNil(BATValue(probi: wrongFormat)?.doubleValue)
    
    let tooShort = "9"
    XCTAssertEqual(BATValue(probi: tooShort)?.doubleValue, 0.0)
    
    let roundDown = "0999999999999999999"
    XCTAssertEqual(BATValue(probi: roundDown)?.doubleValue, 0.9)
    
    let regularConvert = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: regularConvert)?.doubleValue, 1.5)
    
    let doublePrecision = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: doublePrecision, precision: 2)?.doubleValue, 1.55)

    let big = "150000000000000000000000000"
    XCTAssertEqual(BATValue(probi: big)?.doubleValue, 150000000.0)
  }
  
  func testProbiValue() {
    let tooShort = "9"
    XCTAssertEqual(BATValue(probi: tooShort)?.displayString, "0.0")
    
    let roundDown = "0999999999999999999"
    XCTAssertEqual(BATValue(probi: roundDown)?.displayString, "0.9")
    
    let regularConvert = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: regularConvert)?.displayString, "1.5")
    
    let doublePrecision = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: doublePrecision, precision: 2)?.displayString, "1.6")
    
    let big = "150000000000000000000000000"
    XCTAssertEqual(BATValue(probi: big)?.displayString, "150000000.0")
  }
  
  func testDisplayString() {
    
    XCTAssertEqual(BATValue(10.1).displayString, "10.1")
    XCTAssertEqual(BATValue(1).displayString, "1.0")
    XCTAssertEqual(BATValue(22).displayString, "22.0")
    
    // Rounding
    XCTAssertEqual(BATValue(10.1123123).displayString, "10.1")
    XCTAssertEqual(BATValue(10.142).displayString, "10.1")
    
    XCTAssertEqual(BATValue(10.192).displayString, "10.2")
    XCTAssertEqual(BATValue(10.152).displayString, "10.2")
    XCTAssertEqual(BATValue(10.991).displayString, "11.0")
    
    // Zero
    XCTAssertEqual(BATValue(0.0).displayString, "0.0")
    XCTAssertEqual(BATValue(0).displayString, "0.0")
    XCTAssertEqual(BATValue(0.1).displayString, "0.1")
    XCTAssertEqual(BATValue(0.97).displayString, "1.0")
    
    XCTAssertEqual(BATValue(1.99999999999999999).displayString, "2.0")
    XCTAssertEqual(BATValue(0.00000000000000001).displayString, "0.0")
    
    // Negative values
    XCTAssertEqual(BATValue(-10.1).displayString, "-10.1")
    XCTAssertEqual(BATValue(-1).displayString, "-1.0")
    XCTAssertEqual(BATValue(-22).displayString, "-22.0")
    
    // Rounding
    XCTAssertEqual(BATValue(-10.1123123).displayString, "-10.1")
    XCTAssertEqual(BATValue(-10.142).displayString, "-10.1")
    
    XCTAssertEqual(BATValue(-10.192).displayString, "-10.2")
    XCTAssertEqual(BATValue(-10.152).displayString, "-10.2")
    XCTAssertEqual(BATValue(-10.991).displayString, "-11.0")
  }
  
}
