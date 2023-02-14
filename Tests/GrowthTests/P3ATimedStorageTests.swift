// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import Growth

class P3ATimedStorageTests: XCTestCase {
  
  /// ---------
  /// Test Cases from brave-core's similar `TimePeriodStorageTest` (`time_period_storage_unittest.cc`)
  /// ---------
  
  func testStartsZero() {
    var storage = P3ATimedStorage<Int>(name: "testStartsZero", lifetimeInDays: 7)
    storage.reset()
    
    XCTAssertEqual(storage.combinedValue, 0)
    XCTAssertTrue(storage.records.isEmpty)
  }
  
  func testAddSavings() {
    var storage = P3ATimedStorage<Int>(name: "testAddSavings", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    storage.append(value: savings)
    XCTAssertEqual(storage.combinedValue, savings)
    // Accumulate
    storage.append(value: savings)
    storage.append(value: savings)
    XCTAssertEqual(storage.combinedValue, savings * 3)
  }
  
  func testSubDelta() {
    var storage = P3ATimedStorage<Int>(name: "testSubDelta", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: 5000)
    storage.date = { Date().addingTimeInterval(1.days) }
    storage.append(value: 3000)
    storage.date = { Date().addingTimeInterval(2.days) }
    storage.append(value: 1000)
    storage.date = { Date().addingTimeInterval(3.days) }
    storage.append(value: -500)
    XCTAssertEqual(storage.combinedValue, 8500)
    storage.append(value: -4000)
    XCTAssertEqual(storage.combinedValue, 4500)
    storage.date = { Date().addingTimeInterval(7.days) }
    // First day value should expire, can't go below 0
    XCTAssertEqual(storage.combinedValue, 0)
    // If subtracting by an amount greater than the current sum,
    // the sum should not become negative or underflow.
    storage.append(value: 3000)
    storage.append(value: -5000)
    XCTAssertEqual(storage.combinedValue, 0)
    storage.append(value: -100000)
    XCTAssertEqual(storage.combinedValue, 0)
  }
  
  func testForgetsOldSavingsWeekly() {
    var storage = P3ATimedStorage<Int>(name: "testForgetsOldSavingsWeekly", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    storage.append(value: savings)
    XCTAssertEqual(storage.combinedValue, savings)
    
    storage.date = { Date().addingTimeInterval(8.days) }
    
    // More savings
    storage.append(value: savings)
    storage.append(value: savings)
    // Should have forgotten about older days
    XCTAssertEqual(storage.combinedValue, savings * 2)
  }
  
  func testForgetsOldSavingsMonthly() {
    var storage = P3ATimedStorage<Int>(name: "testForgetsOldSavingsMonthly", lifetimeInDays: 30)
    storage.reset()
    
    let savings = 10000
    storage.append(value: savings)
    XCTAssertEqual(storage.combinedValue, savings)
    
    storage.date = { Date().addingTimeInterval(31.days) }
    
    // More savings
    storage.append(value: savings)
    storage.append(value: savings)
    // Should have forgotten about older days
    XCTAssertEqual(storage.combinedValue, savings * 2)
  }
  
  func testRetrievesDailySavings() {
    var storage = P3ATimedStorage<Int>(name: "testRetrievesDailySavings", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    for day in 0...7 {
      storage.date = { Date().addingTimeInterval(day.days) }
      storage.append(value: savings)
    }
    
    XCTAssertEqual(storage.combinedValue, 7 * savings)
  }

  func testHandlesSkippedDay() {
    var storage = P3ATimedStorage<Int>(name: "testHandlesSkippedDay", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    for day in 0..<7 {
      storage.date = { Date().addingTimeInterval(day.days) }
      if day == 3 { continue }
      storage.append(value: savings)
    }
    XCTAssertEqual(storage.combinedValue, 6 * savings)
  }
  
  func testIntermittentUsageWeekly() {
    var storage = P3ATimedStorage<Int>(name: "testIntermittentUsageWeekly", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    for day in stride(from: 0, to: 10, by: 2) {
      storage.date = { Date().addingTimeInterval(day.days) }
      storage.append(value: savings)
    }
    XCTAssertEqual(storage.combinedValue, 4 * savings)
  }
  
  func testIntermittentUsageMonthly() {
    var storage = P3ATimedStorage<Int>(name: "testIntermittentUsageMonthly", lifetimeInDays: 30)
    storage.reset()
    
    let savings = 10000
    for day in stride(from: 0, to: 40, by: 10) {
      storage.date = { Date().addingTimeInterval(day.days) }
      storage.append(value: savings)
    }
    XCTAssertEqual(storage.combinedValue, 3 * savings)
  }
  
  func testInfrequentUsageWeekly() {
    var storage = P3ATimedStorage<Int>(name: "testInfrequentUsageWeekly", lifetimeInDays: 7)
    storage.reset()
    
    let savings = 10000
    storage.append(value: savings)
    storage.date = { Date().addingTimeInterval(6.days) }
    storage.append(value: savings)
    
    XCTAssertEqual(storage.combinedValue, 2 * savings)
  }
  
  func testInfrequentUsageMonthly() {
    var storage = P3ATimedStorage<Int>(name: "testInfrequentUsageWeekly", lifetimeInDays: 30)
    storage.reset()
    
    let savings = 10000
    storage.append(value: savings)
    storage.date = { Date().addingTimeInterval(29.days) }
    storage.append(value: savings)
    
    XCTAssertEqual(storage.combinedValue, 2 * savings)
  }
  
  func testGetHighestValueInPeriod() throws {
    var storage = P3ATimedStorage<Int>(name: "testGetHighestValueInPeriod", lifetimeInDays: 7)
    storage.reset()
    
    let lowestValue = 20
    let lowValue = 50
    let highValue = 75
    
    storage.append(value: lowValue)
    storage.date = { Date().addingTimeInterval(1.days) }
    storage.append(value: highValue)
    storage.date = { Date().addingTimeInterval(2.days) }
    storage.append(value: lowestValue)
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), highValue)
    storage.date = { Date().addingTimeInterval(3.days) }
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), highValue)
  }
  
  func testRecordsHigherValueForToday() {
    var storage = P3ATimedStorage<Int>(name: "testRecordsHigherValueForToday", lifetimeInDays: 30)
    storage.reset()
    
    let lowValue = 50
    let highValue = 75
    
    storage.replaceTodaysRecordsIfLargest(value: lowValue)
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), lowValue)
    
    storage.replaceTodaysRecordsIfLargest(value: highValue)
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), highValue)
    XCTAssertEqual(storage.combinedValue, highValue)
    
    storage.replaceTodaysRecordsIfLargest(value: lowValue)
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), highValue)
    XCTAssertEqual(storage.combinedValue, highValue)
  }
  
  /*
  TEST_F(TimePeriodStorageTest, GetsHighestValueInWeekFromReplacement) {
    InitStorage(30);
    // Add a low value a couple days after a high value,
    // should return highest day value.
    uint64_t low_value = 50;
    uint64_t high_value = 75;
    state_->ReplaceTodaysValueIfGreater(high_value);
    clock_->Advance(base::Days(2));
    state_->ReplaceTodaysValueIfGreater(low_value);
    EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
    // Sanity check disparate days were not replaced
    EXPECT_EQ(state_->GetPeriodSum(), high_value + low_value);
  }
  
  TEST_F(TimePeriodStorageTest, ReplaceIfGreaterForDate) {
    InitStorage(30);
    
    state_->AddDelta(4);
    clock_->Advance(base::Days(1));
    state_->AddDelta(2);
    clock_->Advance(base::Days(1));
    state_->AddDelta(1);
    clock_->Advance(base::Days(1));
    
    // should replace
    state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(2), 3);
    // should not replace
    state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(3), 3);
    
    EXPECT_EQ(state_->GetPeriodSum(), 8U);
    
    // should insert new daily value
    state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(4), 3);
    EXPECT_EQ(state_->GetPeriodSum(), 11U);
    
    // should store, but should not be in sum because it's too old
    state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(31), 10);
    EXPECT_EQ(state_->GetPeriodSum(), 11U);
  }
  */
  
  /// ---------
  /// Test Cases from brave-core's similar `WeeklyEventStorageTest` (`weekly_event_storage_unittest.cc`)
  /// ---------
  
  enum TestEvents: Equatable, Codable {
    case null
    case foo
    case bar
    case brave
  }
  
  func testStartsEmpty() {
    var storage = P3ATimedStorage<TestEvents>(name: "testStartsEmpty", lifetimeInDays: 7)
    storage.reset()
    
    XCTAssertTrue(storage.records.isEmpty)
  }
  
  func testAddEvents() {
    var storage = P3ATimedStorage<TestEvents>(name: "testAddEvents", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: .null)
    XCTAssertFalse(storage.records.isEmpty)
    
    storage.append(value: .brave)
    XCTAssertEqual(storage.records.last?.value, .brave)
  }
  
  func testForgetsOldEvents() {
    var storage = P3ATimedStorage<TestEvents>(name: "testForgetsOldEvents", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: .foo)
    XCTAssertEqual(storage.records.last?.value, .foo)
    
    storage.date = { Date().addingTimeInterval(8.days) }
    XCTAssertTrue(storage.records.isEmpty)
    
    storage.append(value: .null)
    storage.append(value: .bar)
    XCTAssertEqual(storage.records.count, 2)
    XCTAssertEqual(storage.records.last?.value, .bar)
  }
  
  func testIntermittentEventUsage() {
    var storage = P3ATimedStorage<TestEvents>(name: "testIntermittentEventUsage", lifetimeInDays: 7)
    storage.reset()
    
    let value: TestEvents = .foo
    for day in 0..<10 {
      storage.date = { Date().addingTimeInterval((day % 3).days) }
      storage.append(value: value)
    }
    XCTAssertEqual(storage.records.last?.value, value)
  }
  
  func testInfrequentEventUsage() {
    var storage = P3ATimedStorage<TestEvents>(name: "testInfrequentEventUsage", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: .foo)
    storage.date = { Date().addingTimeInterval(6.days) }
    storage.append(value: .bar)
    XCTAssertEqual(storage.records.last?.value, .bar)
    storage.date = { Date().addingTimeInterval(16.days) }
    XCTAssertTrue(storage.records.isEmpty)
  }
  
  func testSerializationOrder() {
    let prefName = "testSerializationOrder"
    var storage = P3ATimedStorage<TestEvents>(name: prefName, lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: .foo)
    storage.append(value: .bar)
    storage.date = { Date().addingTimeInterval(1.days) }
    storage.append(value: .foo)
    storage.append(value: .brave)
    storage.date = { Date().addingTimeInterval(2.days) }
    
    XCTAssertEqual(storage.records.last?.value, .brave)
    
    let newStorage = P3ATimedStorage<TestEvents>(name: prefName, lifetimeInDays: 7)
    XCTAssertEqual(newStorage.records.last?.value, .brave)
  }
  
  /// ------
  /// Original Test Cases
  /// ------
  
  func testMinimumValue() {
    var storage = P3ATimedStorage<Int>(name: "testMinimumValue", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: 1)
    storage.append(value: 2)
    storage.append(value: 3)
    
    XCTAssertEqual(try XCTUnwrap(storage.minimumValue), 1)
  }
  
  func testMaximumValue() {
    var storage = P3ATimedStorage<Int>(name: "testMaximumValue", lifetimeInDays: 7)
    storage.reset()
    
    storage.append(value: 1)
    storage.append(value: 2)
    storage.append(value: 3)
    
    XCTAssertEqual(try XCTUnwrap(storage.maximumValue), 3)
  }
  
  func testAddingToDate() {
    var storage = P3ATimedStorage<Int>(name: "testAddingToDate", lifetimeInDays: 7)
    storage.reset()
    
    let date = Date()
    storage.date = { date }
    storage.append(value: 1)
    storage.date = { Date().addingTimeInterval(1.days )}
    storage.append(value: 1)
    storage.add(value: 1, to: date)
    
    XCTAssertEqual(storage.records.count, 2)
    XCTAssertEqual(storage.combinedValue, 3)
    
    // Test adding a date that doesn't exist adds a new record
    storage.add(value: 1, to: Date().addingTimeInterval(2.days))
    
    XCTAssertEqual(storage.records.count, 3)
    XCTAssertEqual(storage.combinedValue, 4)
    
    // Test that adding a date that isn't in the lifetime does nothing
    storage.add(value: 1, to: Date().addingTimeInterval(-8.days))
    
    XCTAssertEqual(storage.records.count, 3)
    XCTAssertEqual(storage.combinedValue, 4)
    
    // Test that add also persists properly
    let newStorage = P3ATimedStorage<Int>(name: "testAddingToDate", lifetimeInDays: 7)
    
    XCTAssertEqual(newStorage.records.count, 3)
    XCTAssertEqual(newStorage.combinedValue, 4)
  }
  
  func testMaximumDaysCombinedValue() {
    var storage = P3ATimedStorage<Int>(name: "testMaximumDaysCombinedValue", lifetimeInDays: 7)
    storage.reset()
    
    let lowestValue = 20
    let lowValue = 50
    let highValue = 75
    
    storage.append(value: lowestValue)
    storage.date = { Date().addingTimeInterval(1.days) }
    storage.append(value: lowValue)
    storage.append(value: lowValue)
    storage.date = { Date().addingTimeInterval(2.days) }
    storage.append(value: highValue)
    
    // 2 low values added in one day are higher than the high value
    XCTAssertEqual(storage.maximumDaysCombinedValue, lowValue * 2)
  }
}
