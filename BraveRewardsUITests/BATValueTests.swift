/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import XCTest
@testable import BraveRewardsUI

class BATValueTests: XCTestCase {
  
  func testStringInitializer() {
    XCTAssertEqual(BATValue("0")?.displayString, "0.000")
    XCTAssertEqual(BATValue("1.0")?.displayString, "1.000")
    XCTAssertEqual(BATValue("20.9999")?.displayString, "21.000")
    XCTAssertEqual(BATValue("-10.0")?.displayString, "-10.000")
    XCTAssertEqual(BATValue("-10")?.displayString, "-10.000")
    XCTAssertEqual(BATValue("-200")?.displayString, "-200.000")
    
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
    XCTAssertEqual(BATValue(probi: roundDown)?.doubleValue, 0.999)
    
    let regularConvert = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: regularConvert)?.doubleValue, 1.559)
    
    let doublePrecision = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: doublePrecision, precision: 2)?.doubleValue, 1.55)

    let big = "150000000000000000000000000"
    XCTAssertEqual(BATValue(probi: big)?.doubleValue, 150000000.0)
  }
  
  func testProbiValue() {
    let tooShort = "9"
    XCTAssertEqual(BATValue(probi: tooShort)?.displayString, "0.000")
    
    let roundDown = "0999999999999999999"
    XCTAssertEqual(BATValue(probi: roundDown)?.displayString, "0.999")
    
    let regularConvert = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: regularConvert)?.displayString, "1.559")
    
    let doublePrecision = "1559999999999999990"
    XCTAssertEqual(BATValue(probi: doublePrecision, precision: 3)?.displayString, "1.559")
    
    let big = "150000000000000000000000000"
    XCTAssertEqual(BATValue(probi: big)?.displayString, "150000000.000")
  }
  
  func testDisplayString() {
    
    XCTAssertEqual(BATValue(10.1).displayString, "10.100")
    XCTAssertEqual(BATValue(1).displayString, "1.000")
    XCTAssertEqual(BATValue(22).displayString, "22.000")
    
    // Rounding
    XCTAssertEqual(BATValue(10.111123123).displayString, "10.111")
    XCTAssertEqual(BATValue(10.14234).displayString, "10.142")
    
    XCTAssertEqual(BATValue(10.1929).displayString, "10.193")
    XCTAssertEqual(BATValue(10.1529).displayString, "10.153")
    XCTAssertEqual(BATValue(10.9999).displayString, "11.000")
    
    // Zero
    XCTAssertEqual(BATValue(0.0).displayString, "0.000")
    XCTAssertEqual(BATValue(0).displayString, "0.000")
    XCTAssertEqual(BATValue(0.1).displayString, "0.100")
    XCTAssertEqual(BATValue(0.9999).displayString, "1.000")
    
    XCTAssertEqual(BATValue(1.99999999999999999).displayString, "2.000")
    XCTAssertEqual(BATValue(0.00000000000000001).displayString, "0.000")
    
    // Negative values
    XCTAssertEqual(BATValue(-10.1).displayString, "-10.100")
    XCTAssertEqual(BATValue(-1).displayString, "-1.000")
    XCTAssertEqual(BATValue(-22).displayString, "-22.000")
    
    // Rounding
    XCTAssertEqual(BATValue(-10.1123123).displayString, "-10.112")
    XCTAssertEqual(BATValue(-10.1423).displayString, "-10.142")
    
    XCTAssertEqual(BATValue(-10.1929).displayString, "-10.193")
    XCTAssertEqual(BATValue(-10.1529).displayString, "-10.153")
    XCTAssertEqual(BATValue(-10.99992).displayString, "-11.000")
  }
  
}
