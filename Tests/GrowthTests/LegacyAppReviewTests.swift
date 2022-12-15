// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared
import BraveShared
@testable import Growth

class LegacyAppReviewTests: XCTestCase {
  
  private var lastReviewDate: Date? { return Preferences.LegacyReview.lastReviewDate.value }
  private var launchCount: Int { return Preferences.Review.launchCount.value }
  private var threshold: Int { return Preferences.LegacyReview.threshold.value }

  override func setUp() {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    super.setUp()

    Preferences.LegacyReview.lastReviewDate.reset()
    Preferences.Review.launchCount.reset()
    Preferences.LegacyReview.threshold.reset()

    // Each test should start with at least one simulated launch, to mimic behavior of regular app.
    // (incrementing launch count at app start)
    simulateLaunch()
  }

  func testFirstRun() {
    let date = Date()

    XCTAssertEqual(launchCount, 1, "First run should have launch count equal to 1")
    XCTAssertNil(lastReviewDate)
    XCTAssertEqual(threshold, AppReviewManager.shared.legacyFirstThreshold)

    XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview(date: date))

    XCTAssertEqual(launchCount, 1, "Launch count shouldn't change after review request.")
    XCTAssertNil(lastReviewDate)
    XCTAssertEqual(threshold, AppReviewManager.shared.legacyFirstThreshold)
  }

  func testThresholdsNoDateInterval() {

    // First threshold
    for i in 2...AppReviewManager.shared.legacyFirstThreshold {
      simulateLaunch()
      XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview())

      XCTAssertNil(lastReviewDate)
      XCTAssertEqual(threshold, AppReviewManager.shared.legacyFirstThreshold)
      XCTAssertEqual(launchCount, i)
    }

    let firstDate = dateFrom(string: "2019-01-01")

    simulateLaunch()
    // Next app launch triggers the review
    XCTAssert(AppReviewManager.shared.legacyShouldRequestReview(date: firstDate))

    XCTAssertEqual(lastReviewDate, firstDate)
    XCTAssertEqual(threshold, AppReviewManager.shared.legacySecondThreshold)

    // Second threshold
    for i in launchCount..<AppReviewManager.shared.legacySecondThreshold {
      simulateLaunch()
      XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview())
      XCTAssertEqual(lastReviewDate, firstDate)
      XCTAssertEqual(threshold, AppReviewManager.shared.legacySecondThreshold)
      XCTAssertEqual(launchCount, i + 1)
    }

    // Enough launches passed but not enough time between app reviews.
    let secondDate = dateFrom(string: "2019-01-14")
    simulateLaunch()
    XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview(date: secondDate))

    XCTAssertEqual(lastReviewDate, firstDate)
    XCTAssertEqual(threshold, AppReviewManager.shared.legacySecondThreshold)
  }

  func testThresholdsWithDateInterval() {
    let firstDate = dateFrom(string: "2019-01-01")

    Preferences.Review.launchCount.value = AppReviewManager.shared.legacyFirstThreshold + 3

    XCTAssert(AppReviewManager.shared.legacyShouldRequestReview(date: firstDate))

    // Second threshold, no date change
    Preferences.Review.launchCount.value = AppReviewManager.shared.legacySecondThreshold + 3
    XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview(date: firstDate))

    // Second threshold, date change
    XCTAssert(AppReviewManager.shared.legacyShouldRequestReview(date: dateFrom(string: "2019-04-01")))

    // Second threshold, not enough date interval
    XCTAssertFalse(AppReviewManager.shared.legacyShouldRequestReview(date: dateFrom(string: "2019-05-01")))

    // Third threshold, good launch count and date
    Preferences.Review.launchCount.value = AppReviewManager.shared.legacyLastThreshold + 3
    XCTAssert(AppReviewManager.shared.legacyShouldRequestReview(date: dateFrom(string: "2019-11-20")))

    // Date and launch count far away in the future
    Preferences.Review.launchCount.value = AppReviewManager.shared.legacyLastThreshold + 1000
    XCTAssert(AppReviewManager.shared.legacyShouldRequestReview(date: dateFrom(string: "2022-01-01")))

  }

  private func simulateLaunch() {
    Preferences.Review.launchCount.value += 1
  }

  private func dateFrom(string: String, format: String? = nil) -> Date {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = format ?? "yyyy-MM-dd"

    return dateFormatter.date(from: string)!
  }
}
