// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import Storage
import Onboarding

@testable import Brave

class BlockSummaryTests: XCTestCase {

  override func setUp() {
    // The test json is loaded from Client Tests Target
    subject = BlockingSummaryDataSource(with: Bundle.module.path(forResource: "blocking-summary-test", ofType: "json"))
  }

  override func tearDown() {
    subject = nil

    super.tearDown()
  }

  func testExistingTierList() {
    // Savings value rounded ceil

    XCTAssertTrue(
      fetchDomainSiteSavings(with: "http://dengekionline.com/") == "1.48",
      "Rounded savings Amount - 1.477627916")

    XCTAssertTrue(
      fetchDomainSiteSavings(with: "http://ascii.jp/") == "1.36",
      "Rounded savings Amount - 1.359224634")

    XCTAssertTrue(
      fetchDomainSiteSavings(with: "http://jisin.jp/") == "0.68",
      "Rounded savings Amount - 0.678194046")

    // Savings value rounded floor

    XCTAssertTrue(
      fetchDomainSiteSavings(with: "http://bunshun.jp/") == "1.13",
      "Rounded savings Amount - 1.13061841")

    XCTAssertFalse(
      fetchDomainSiteSavings(with: "http://jbpress.ismedia.jp/") == "0.66",
      "Rounded savings Amount - 1.343074322 - Should return 1.34")

    XCTAssertTrue(
      fetchDomainSiteSavings(with: "https://ananweb.jp/") == "0.85",
      "Rounded savings Amount - 0.8544185638")

    // Websites that doesnt exist in Json file

    XCTAssertNil(fetchDomainSiteSavings(with: "https://fugazzi.com/"))

    XCTAssertNil(fetchDomainSiteSavings(with: "https://abc.com/"))

    XCTAssertNil(fetchDomainSiteSavings(with: "https://test.com/"))

  }

  // Function utility that will fetch savings amount
  // It will return nil If website and savings amount for it doesnt exist in json file
  private func fetchDomainSiteSavings(with urlString: String) -> String? {
    if let url = URL(string: urlString),
      let savings = subject.fetchDomainFetchedSiteSavings(url) {
      return savings
    }

    return nil
  }

  private var subject: BlockingSummaryDataSource!
}
