// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveShared

class TimeExtensionTests: XCTestCase {

    func testHoursMinutesDays() {
        XCTAssertEqual(1.minutes, 60)
        XCTAssertEqual(12.minutes, 720)
        
        XCTAssertEqual(1.hours, 3600)
        XCTAssertEqual(12.hours, 43200)
        
        XCTAssertEqual(1.days, 86400)
        XCTAssertEqual(12.days, 1036800)
    }
}
