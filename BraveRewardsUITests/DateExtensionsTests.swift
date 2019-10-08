/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import XCTest
@testable import BraveRewardsUI

class DateExtensionsTests: XCTestCase {

  func testCurrentMonth() {
    let february = dateFrom(string: "2018-02-22")
    // Default locale
    XCTAssertEqual(february.currentMonthName().lowercased(), "february")
    
    // Locale with declensions
    // where 'June 2019' and 'June 10 2019' words are slightly different in some languages.
    // See https://en.wiktionary.org/wiki/luty#Declension_2 for context.
    let polishLocale = Locale(identifier: "pl")
    XCTAssertEqual(february.currentMonthName(for: polishLocale).lowercased(), "luty")
    XCTAssertNotEqual(february.currentMonthName(for: polishLocale).lowercased(), "lutego")
    
  }
  
  func testCurrentYear() {
    let _2019 = dateFrom(string: "2019-01-20")
    XCTAssertEqual(_2019.currentYear, 2019)
    
    let _1984 = dateFrom(string: "1984-11-11")
    XCTAssertEqual(_1984.currentYear, 1984)
  }
  
  func testDateStringFromIntervalSince1970() {
    let timeInterval = UInt64(dateFrom(string: "2019-01-20").timeIntervalSince1970)
    XCTAssertEqual(Date.stringFrom(reconcileStamp: timeInterval), "1/20/19")
  }
  
  private func dateFrom(string: String) -> Date {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = "yyyy-MM-dd"
    
    return dateFormatter.date(from: string)!
  }
  
}
