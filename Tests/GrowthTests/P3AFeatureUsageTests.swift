// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
@testable import Growth

class P3AFeatureUsageTests: XCTestCase {
  
  func testEmptyStart() {
    let feature = P3AFeatureUsage(name: "testEmptyStart", histogram: "")
    feature.reset()
    XCTAssertNil(feature.firstUsageOption.value)
    XCTAssertNil(feature.lastUsageOption.value)
    XCTAssertFalse(feature.returnedDayAfterOption.value)
  }
  
  func testFirstUsageSet() {
    let feature = P3AFeatureUsage(name: "testFirstUsageSet", histogram: "")
    feature.reset()
    feature.recordUsage()
    XCTAssertEqual(feature.firstUsageOption.value, feature.lastUsageOption.value)
  }
  
  func testBasicUsage() {
    var feature = P3AFeatureUsage(name: "testBasicUsage", histogram: "")
    feature.reset()
    let firstUsageDate = Date()
    feature.date = { firstUsageDate }
    feature.recordUsage()
    let lastUsageDate = firstUsageDate.addingTimeInterval(1.days)
    feature.date = { lastUsageDate }
    feature.recordUsage()
    // Dates are set as start of day
    XCTAssertEqual(feature.firstUsageOption.value, Calendar(identifier: .gregorian).startOfDay(for: firstUsageDate))
    XCTAssertEqual(feature.lastUsageOption.value, Calendar(identifier: .gregorian).startOfDay(for: lastUsageDate))
  }
  
  func testReturningUserNeverUsed() {
    let feature = P3AFeatureUsage(name: "testReturningUserNeverUsed", histogram: "")
    feature.reset()
    XCTAssertEqual(feature.returningUserState, .neverUsed)
  }
  
  func testReturningUserUsedAfterFirstWeek() {
    var feature = P3AFeatureUsage(name: "testReturningUserUsedAfterFirstWeek", histogram: "")
    feature.reset()
    feature.recordUsage()
    feature.date = { Date().addingTimeInterval(8.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .usedButImNotAFirstTimeUserThisWeek)
  }
  
  func testReturningUserUsedDayAfter() {
    var feature = P3AFeatureUsage(name: "testReturningUserUsedDayAfter", histogram: "")
    feature.reset()
    feature.recordUsage()
    feature.date = { Date().addingTimeInterval(1.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .firstTimeUserThisWeekAndUsedItTheFollowingDay)
    XCTAssertTrue(feature.returnedDayAfterOption.value)
    
    // Test that it remains following day even as the week goes on
    feature.date = { Date().addingTimeInterval(2.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .firstTimeUserThisWeekAndUsedItTheFollowingDay)
    
    feature.date = { Date().addingTimeInterval(7.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .firstTimeUserThisWeekAndUsedItTheFollowingDay)
    
    // Then finally leaving first week
    feature.date = { Date().addingTimeInterval(8.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .usedButImNotAFirstTimeUserThisWeek)
  }
  
  func testReturningUserUsedDayDuringFirstWeek() {
    var feature = P3AFeatureUsage(name: "testReturningUserUsedDayDuringFirstWeek", histogram: "")
    feature.reset()
    feature.recordUsage()
    feature.date = { Date().addingTimeInterval(3.days) }
    feature.recordUsage()
    XCTAssertEqual(feature.returningUserState, .firstTimeUserThisWeekAndUsedItThisWeek)
    XCTAssertFalse(feature.returnedDayAfterOption.value)
  }
  
  func testReturningUserDidNotUseDuringWeek() {
    var feature = P3AFeatureUsage(name: "testReturningUserDidNotUseDuringWeek", histogram: "")
    feature.reset()
    feature.recordUsage()
    feature.date = { Date().addingTimeInterval(7.days) }
    XCTAssertEqual(feature.returningUserState, .firstTimeUserThisWeekButDidntReturnDuringWeek)
  }
}
