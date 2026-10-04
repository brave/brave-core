// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import PlaylistUI

final class TimestampFormatStyleTests: XCTestCase {
  func testFormatDurationUnderOneHour() {
    XCTAssertEqual(Duration.seconds(500).formatted(.timestamp), "8:20")
    XCTAssertEqual(Duration.seconds(500.95).formatted(.timestamp), "8:20")
    XCTAssertEqual(Duration.seconds(500.25).formatted(.timestamp), "8:20")
    XCTAssertEqual(Duration.seconds(-500).formatted(.timestamp), "-8:20")
    XCTAssertEqual(Duration.seconds(3599).formatted(.timestamp), "59:59")
    XCTAssertEqual(Duration.seconds(3599.99).formatted(.timestamp), "59:59")
  }

  func testFormatDurationExactlyOneHour() {
    XCTAssertEqual(Duration.seconds(3600).formatted(.timestamp), "1:00:00")
    XCTAssertEqual(Duration.seconds(3600.95).formatted(.timestamp), "1:00:00")
    XCTAssertEqual(Duration.seconds(3600.25).formatted(.timestamp), "1:00:00")
    XCTAssertEqual(Duration.seconds(-3600).formatted(.timestamp), "-1:00:00")
  }

  func testFormatDurationOverOneHour() {
    XCTAssertEqual(Duration.seconds(7200).formatted(.timestamp), "2:00:00")
    XCTAssertEqual(Duration.seconds(7200.95).formatted(.timestamp), "2:00:00")
    XCTAssertEqual(Duration.seconds(7200.25).formatted(.timestamp), "2:00:00")
    XCTAssertEqual(Duration.seconds(-7200).formatted(.timestamp), "-2:00:00")
  }
}
