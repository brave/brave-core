// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Growth

class UptimeMonitorTests: XCTestCase {

  // How much a single second is actually accounted for during the test
  static let testSecond: TimeInterval = 0.01

  override func setUp() {
    super.setUp()

    Preferences.UptimeMonitor.startTime.reset()
    Preferences.UptimeMonitor.uptimeSum.reset()

    var testCalendar = Calendar(identifier: .gregorian)
    testCalendar.timeZone = .init(abbreviation: "GMT")!
    testCalendar.locale = .init(identifier: "en_US_POSIX")
    UptimeMonitor.setCalendarForTesting(testCalendar)
    UptimeMonitor.setNowForTesting({ .now })
    UptimeMonitor.setUsageIntervalForTesting(Self.testSecond)
  }

  func testNoStartTimeInit() {
    XCTAssertNil(Preferences.UptimeMonitor.startTime.value)
    let um = UptimeMonitor()
    XCTAssertNotNil(Preferences.UptimeMonitor.startTime.value)
    XCTAssertFalse(um.isMonitoring)
  }

  func testRecordAfterDay() {
    let um = UptimeMonitor()
    let e = expectation(description: "recorded")
    let now = Date()
    UptimeMonitor.setNowForTesting({ now })
    Preferences.UptimeMonitor.startTime.value = .now.addingTimeInterval(-60 * 60 * 24)
    Preferences.UptimeMonitor.uptimeSum.value = 60
    um.didRecordP3A = { minutes in
      XCTAssertEqual(minutes, 1)
      e.fulfill()
    }
    um.beginMonitoring()
    wait(for: [e], timeout: Self.testSecond * 2)
    um.pauseMonitoring()

    // Ensure prefs are reset
    XCTAssertEqual(Preferences.UptimeMonitor.startTime.value, now)
    XCTAssertEqual(Preferences.UptimeMonitor.uptimeSum.value, 0)
  }

  func testNoRecordBeforeOneDay() {
    let um = UptimeMonitor()
    let now = Date()
    UptimeMonitor.setNowForTesting({ now })
    let e = expectation(description: "not-recorded")
    e.isInverted = true
    Preferences.UptimeMonitor.startTime.value = now
    Preferences.UptimeMonitor.uptimeSum.value = 60
    um.didRecordP3A = { _ in
      XCTFail("Should not record any data before a day has passed")
    }
    um.beginMonitoring()
    wait(for: [e], timeout: Self.testSecond * 2)
    um.pauseMonitoring()

    // Ensure prefs are not reset
    XCTAssertEqual(Preferences.UptimeMonitor.startTime.value, now)
    XCTAssertNotEqual(Preferences.UptimeMonitor.uptimeSum.value, 0)
  }
}
