// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveShared

class DateTimeExtensionTests: XCTestCase {

  func testHoursMinutesDays() {
    XCTAssertEqual(1.minutes, 60)
    XCTAssertEqual(12.minutes, 720)

    XCTAssertEqual(1.hours, 3600)
    XCTAssertEqual(12.hours, 43200)

    XCTAssertEqual(1.days, 86400)
    XCTAssertEqual(12.days, 1036800)
  }

  func testTimeStampToDate() {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss Z"
    dateFormatter.locale = Locale(identifier: "en_US_POSIX")

    let date1 = dateFormatter.date(from: "2022-03-03 20:47:07 +0000")!
    let date2 = dateFormatter.date(from: "2009-06-30 08:28:47 +0000")!

    let timeStamp1: UInt64 = 1646340427908992
    let timeStamp2: UInt64 = 1246350527908882

    XCTAssertTrue(timeStamp1.toDate() == date1)
    XCTAssertTrue(timeStamp2.toDate() == date2)
  }
}
