// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Brave

class DefaultBrowserHelperTests: XCTestCase {

  override func setUp() async throws {
    // Reset all relevant preferences
    Preferences.General.lastHTTPURLOpenedDate.reset()
    Preferences.General.isDefaultAPILastResult.reset()
    Preferences.General.isDefaultAPILastCheckDate.reset()
    Preferences.General.isDefaultAPILastResultDate.reset()
    Preferences.DAU.appRetentionLaunchDate.reset()
    Preferences.DAU.installationDate.reset()
  }

  // MARK: - API Availability Tests

  func testAPIUnavailable_FallsBackToHeuristic() {
    let date = Date()
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-5)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .unavailable
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .likely)
    XCTAssertFalse(helper.isAccurateDefaultCheckAvailable)
  }

  func testAPIUnavailable_FallsBackToHeuristic_NoLinksOpened() {
    let date = Date()

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .unavailable
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .unknown)
    XCTAssertFalse(helper.isAccurateDefaultCheckAvailable)
  }

  func testAPIUnavailable_FallsBackToHeuristic_Past14Days() {
    let date = Date()
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-30)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .unavailable
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .unknown)
    XCTAssertFalse(helper.isAccurateDefaultCheckAvailable)
  }

  func testAPIAvailable_PerformAccurateCheck() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date

    let helper = DefaultBrowserHelper(
      // API would return false, but cached is true
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }
    helper.performAccurateDefaultCheckIfNeeded()

    XCTAssertEqual(helper.status, .defaulted)
    XCTAssertTrue(helper.isAccurateDefaultCheckAvailable)
  }

  func testAPIAvailable_UsesCachedResult() {
    let date = Date()
    // Set up cached API result from 3 days ago
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-3)

    let helper = DefaultBrowserHelper(
      // API would return false, but cached is true
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .defaulted)  // Uses cached result
    XCTAssertTrue(helper.isAccurateDefaultCheckAvailable)
  }

  // MARK: - Caching Logic Tests

  func testCachedResult_ValidWithin14Days() {
    let date = Date()
    // Cache result from 10 days ago
    Preferences.General.isDefaultAPILastResult.value = false
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-10)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .notDefaulted)  // Uses cached result
  }

  func testCachedResult_ExpiredAfter14Days() {
    let date = Date()
    // Cache result from 15 days ago (expired)
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-15)
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-5)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .likely)  // Falls back to heuristic
  }

  // MARK: - HTTP URL Opened Tests

  func testHTTPURLOpened_RecentLinkOpening() {
    let date = Date()
    // API cached result from 5 days ago: not default
    Preferences.General.isDefaultAPILastResult.value = false
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-5)

    // User opened link 2 days ago (more recent than API check)
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-2)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .likely)  // HTTP override
  }

  func testHTTPURLOpened_OldLinkOpening() {
    let date = Date()
    // API cached result from 2 days ago: not default
    Preferences.General.isDefaultAPILastResult.value = false
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-2)

    // User opened link 5 days ago (older than API check)
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-5)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .notDefaulted)  // Uses API result
  }

  // MARK: - Check Scheduling Tests

  func testCheckScheduling_Day0_FirstOpen() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date  // Today is day 0

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertTrue(helper.shouldPerformAccurateDefaultCheck)
  }

  func testCheckScheduling_Day2_WithinWindow() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-2)  // Day 2

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertTrue(helper.shouldPerformAccurateDefaultCheck)
  }

  func testCheckScheduling_Day11_AfterWindow() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-11)  // Day 11

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertFalse(helper.shouldPerformAccurateDefaultCheck)  // No checks after day 10
  }

  func testCheckScheduling_AlreadyShownInWindow() {
    let date = Date()
    // Day 4 (days 3-6 window)
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-4)
    // Shown yesterday (also day 3)
    Preferences.General.isDefaultAPILastCheckDate.value = date.addingDays(-1)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    XCTAssertFalse(helper.shouldPerformAccurateDefaultCheck)  // Already shown in this window
  }

  func testCheckScheduling_AlreadyDefault() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-2)  // Day 2
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-1)
    Preferences.General.isDefaultAPILastCheckDate.value = date.addingDays(-1)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }

    XCTAssertFalse(helper.shouldPerformAccurateDefaultCheck)  // Already default
  }

  // MARK: - Record Check Tests

  func testRecordCheckShown_FirstTime() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    helper.performAccurateDefaultCheckIfNeeded()

    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)
  }

  func testRecordCheckShown_SubsequentTimes() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date
    Preferences.General.isDefaultAPILastCheckDate.value = date.addingDays(-1)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    helper.performAccurateDefaultCheckIfNeeded()

    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)  // Updated
  }

  // MARK: - Rate Limited API Tests

  func testRateLimited_UsesLastCachedResult() {
    let date = Date()
    // Set up cached result from 3 days ago
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-3)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .rateLimited(retryDate: date.addingDays(7))
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .defaulted)  // Uses cached result despite rate limiting
    XCTAssertTrue(helper.isAccurateDefaultCheckAvailable)  // API is available, just rate limited
  }

  func testRateLimited_NoCachedResult_FallsBackToHeuristic() {
    let date = Date()
    // Recent HTTP activity
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-5)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .rateLimited(retryDate: date.addingDays(7))
    )
    helper.now = { date }

    XCTAssertEqual(helper.status, .likely)  // Falls back to heuristic
    XCTAssertTrue(helper.isAccurateDefaultCheckAvailable)  // API is available, just rate limited
  }

  func testRateLimited_PerformAccurateCheck_DoesNotCallAPI() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-2)  // Day 2

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .rateLimited(retryDate: date.addingDays(7))
    )
    helper.now = { date }

    // Perform accurate check - should handle rate limiting gracefully
    helper.performAccurateDefaultCheckIfNeeded()

    // Check date should be set, but result should not be updated
    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)
    XCTAssertNil(Preferences.General.isDefaultAPILastResult.value)
    XCTAssertNil(Preferences.General.isDefaultAPILastResultDate.value)
  }

  // MARK: - Accurate Check Tests

  func testPerformAccurateCheck_APIReturnsTrue_UpdatesStatusAndCache() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-2)  // Day 2

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }

    // Initial status should be unknown
    XCTAssertEqual(helper.status, .unknown)

    // Perform accurate check
    helper.performAccurateDefaultCheckIfNeeded()

    // Verify status updated to defaulted
    XCTAssertEqual(helper.status, .defaulted)

    // Verify cache was updated
    XCTAssertEqual(Preferences.General.isDefaultAPILastResult.value, true)
    XCTAssertEqual(Preferences.General.isDefaultAPILastResultDate.value, date)
    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)
  }

  func testPerformAccurateCheck_APIReturnsFalse_UpdatesStatusAndCache() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-5)  // Day 5

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    // Perform accurate check
    helper.performAccurateDefaultCheckIfNeeded()

    // Verify status updated to not defaulted
    XCTAssertEqual(helper.status, .notDefaulted)

    // Verify cache was updated
    XCTAssertEqual(Preferences.General.isDefaultAPILastResult.value, false)
    XCTAssertEqual(Preferences.General.isDefaultAPILastResultDate.value, date)
    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)
  }

  func testPerformAccurateCheck_APIRateLimited_PreservesCacheAndStatus() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-7)  // Day 7

    // Set up existing cache
    let existingCacheDate = date.addingDays(-2)
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = existingCacheDate
    Preferences.General.isDefaultAPILastCheckDate.value = existingCacheDate

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .rateLimited(retryDate: date.addingDays(3))
    )
    helper.now = { date }

    // Initial status should use cached result
    XCTAssertEqual(helper.status, .defaulted)

    // Perform accurate check (should not throw, should handle gracefully)
    helper.performAccurateDefaultCheckIfNeeded()

    // Verify result cache was NOT changed (preserved existing values)
    XCTAssertEqual(Preferences.General.isDefaultAPILastResult.value, true)
    XCTAssertEqual(Preferences.General.isDefaultAPILastResultDate.value, existingCacheDate)
    // Check date should be updated
    XCTAssertEqual(Preferences.General.isDefaultAPILastCheckDate.value, date)

    // Status should remain the same
    XCTAssertEqual(helper.status, .defaulted)
  }

  func testPerformAccurateCheck_APIUnavailable_DoesNotUpdateCache() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-1)  // Day 1
    // Recent activity
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingHours(-12)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .unavailable
    )
    helper.now = { date }

    // Initial status should use heuristic
    XCTAssertEqual(helper.status, .likely)

    // Perform accurate check
    helper.performAccurateDefaultCheckIfNeeded()

    // Verify no cache was set (API unavailable)
    XCTAssertNil(Preferences.General.isDefaultAPILastResult.value)
    XCTAssertNil(Preferences.General.isDefaultAPILastResultDate.value)
    XCTAssertNil(Preferences.General.isDefaultAPILastCheckDate.value)

    // Status should remain heuristic-based
    XCTAssertEqual(helper.status, .likely)
  }

  func testPerformAccurateCheck_NoCheckNeeded_SkipsCheck() {
    let date = Date()
    // Day 15 (outside check window)
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-15)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }

    // Perform accurate check - should skip because no check needed
    helper.performAccurateDefaultCheckIfNeeded()

    // Verify no cache was set (check was skipped)
    XCTAssertNil(Preferences.General.isDefaultAPILastResult.value)
    XCTAssertNil(Preferences.General.isDefaultAPILastResultDate.value)
    XCTAssertNil(Preferences.General.isDefaultAPILastCheckDate.value)
  }

  func testPerformAccurateCheck_WithHTTPURLOpened_StatusReflectsOverride() {
    let date = Date()
    Preferences.DAU.appRetentionLaunchDate.value = date.addingDays(-8)  // Day 8

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date.addingHours(-2) }

    // Perform accurate check first
    helper.performAccurateDefaultCheckIfNeeded()

    helper.now = { date }

    // Verify initial cache and status
    XCTAssertEqual(Preferences.General.isDefaultAPILastResult.value, false)
    XCTAssertEqual(helper.status, .notDefaulted)

    // Now set HTTP activity more recent than the API check
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingHours(-1)

    // Update status to reflect HTTP override
    helper.updateStatus()

    // Status should now be likely due to HTTP override
    XCTAssertEqual(helper.status, .likely)

    // Cache should still have original API result
    XCTAssertEqual(Preferences.General.isDefaultAPILastResult.value, false)
  }

  func testHTTPURLOpened_WithTrueCachedResult_ReturnsDefaulted() {
    let date = Date()
    // API cached result from 5 days ago: IS default (true)
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-5)

    // User opened link 2 days ago (more recent than API check)
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-2)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    // HTTP override with TRUE cached result should return .defaulted
    XCTAssertEqual(helper.status, .defaulted)
  }

  func testHTTPURLOpened_WithFalseCachedResult_ReturnsLikely() {
    let date = Date()
    // API cached result from 5 days ago: NOT default (false)
    Preferences.General.isDefaultAPILastResult.value = false
    Preferences.General.isDefaultAPILastResultDate.value = date.addingDays(-5)

    // User opened link 2 days ago (more recent than API check)
    Preferences.General.lastHTTPURLOpenedDate.value = date.addingDays(-2)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: true)
    )
    helper.now = { date }

    // HTTP override with FALSE cached result should return .likely
    XCTAssertEqual(helper.status, .likely)
  }

  // MARK: - Record App Launched With Web URL Tests

  func testRecordAppLaunchedWithWebURL_SetsCurrentDate() {
    let date = Date()
    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    // Ensure no previous HTTP date is set
    XCTAssertNil(Preferences.General.lastHTTPURLOpenedDate.value)

    helper.recordAppLaunchedWithWebURL()

    XCTAssertEqual(Preferences.General.lastHTTPURLOpenedDate.value, date)
  }

  func testRecordAppLaunchedWithWebURL_UpdatesExistingDate() {
    let oldDate = Date().addingDays(-5)
    let newDate = Date()

    Preferences.General.lastHTTPURLOpenedDate.value = oldDate

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { newDate }

    helper.recordAppLaunchedWithWebURL()

    XCTAssertEqual(Preferences.General.lastHTTPURLOpenedDate.value, newDate)
    XCTAssertNotEqual(Preferences.General.lastHTTPURLOpenedDate.value, oldDate)
  }

  func testRecordAppLaunchedWithWebURL_TriggersStatusUpdate() {
    let date = Date()
    // Set up cached API result that indicates not default
    Preferences.General.isDefaultAPILastResult.value = false
    Preferences.General.isDefaultAPILastResultDate.value =
      date.addingDays(-3)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    // Initial status should be notDefaulted based on cached API result
    XCTAssertEqual(helper.status, .notDefaulted)

    // Record app launched with web URL
    helper.recordAppLaunchedWithWebURL()

    // Status should now be likely due to recent HTTP activity overriding
    // cached API result
    XCTAssertEqual(helper.status, .likely)
  }

  func testRecordAppLaunchedWithWebURL_WithAPIUnavailable_UpdatesStatus() {
    let date = Date()
    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .unavailable
    )
    helper.now = { date }

    // Initial status should be unknown (no HTTP activity)
    XCTAssertEqual(helper.status, .unknown)

    helper.recordAppLaunchedWithWebURL()

    // Status should now be likely due to recent HTTP activity
    XCTAssertEqual(helper.status, .likely)
  }

  func testRecordAppLaunchedWithWebURL_WithCachedTrue_StatusBecomesDefaulted() {
    let date = Date()
    // Set up cached API result that indicates IS default
    Preferences.General.isDefaultAPILastResult.value = true
    Preferences.General.isDefaultAPILastResultDate.value =
      date.addingDays(-5)

    let helper = DefaultBrowserHelper(
      isDefaultAppChecker: .available(isDefault: false)
    )
    helper.now = { date }

    // Initial status should be defaulted based on cached API result
    XCTAssertEqual(helper.status, .defaulted)

    // Record app launched with web URL (more recent than cache)
    helper.recordAppLaunchedWithWebURL()

    // Status should remain defaulted (HTTP override with true cache = defaulted)
    XCTAssertEqual(helper.status, .defaulted)
  }
}

// MARK: - Test Helpers

extension Date {
  fileprivate func addingDays(_ days: Int) -> Date {
    let calendar = Calendar(identifier: .gregorian)
    return calendar.date(byAdding: .day, value: days, to: self)!
  }
  fileprivate func addingHours(_ hours: Int) -> Date {
    let calendar = Calendar(identifier: .gregorian)
    return calendar.date(byAdding: .hour, value: hours, to: self)!
  }
}

extension IsDefaultAppChecker {
  static var unavailable: Self {
    .init(
      isAvailable: { false },
      isDefaultWebBrowser: { throw CheckError.unavailable }
    )
  }

  static func available(isDefault: Bool) -> Self {
    .init(
      isAvailable: { true },
      isDefaultWebBrowser: { isDefault }
    )
  }

  static func rateLimited(retryDate: Date? = nil, lastStatusDate: Date? = nil) -> Self {
    .init(
      isAvailable: { true },
      isDefaultWebBrowser: {
        throw CheckError.rateLimited(
          retryAvailableDate: retryDate,
          statusLastProvidedDate: lastStatusDate
        )
      }
    )
  }
}
