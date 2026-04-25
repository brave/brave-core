// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Onboarding

final class AddToDockEligibilityTests: XCTestCase {

  func testEligibleEnglishRegions() {
    for identifier in ["en_US", "en_CA", "en_AU"] {
      XCTAssertTrue(
        AddToDockEligibility.isEligible(for: Locale(identifier: identifier)),
        "expected eligible for \(identifier)"
      )
    }
  }

  func testIneligibleNonEnglish() {
    XCTAssertFalse(AddToDockEligibility.isEligible(for: Locale(identifier: "fr_CA")))
    XCTAssertFalse(AddToDockEligibility.isEligible(for: Locale(identifier: "de_DE")))
  }

  func testIneligibleEnglishOutsideAllowlist() {
    XCTAssertFalse(AddToDockEligibility.isEligible(for: Locale(identifier: "en_GB")))
    XCTAssertFalse(AddToDockEligibility.isEligible(for: Locale(identifier: "en_NZ")))
  }
}
