// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import Storage

@testable import Client

class BenchmarkTierTests: XCTestCase {
    typealias Tier = BrowserViewController.BenchmarkTrackerCountTier

    override func setUp() {
        subject = BrowserViewController.BenchmarkTrackerCountTier.allCases
    }

    override func tearDown() {
        subject = nil

        super.tearDown()
    }

    func testExistingTierList() {
        testNextTier(numberOfTrackerAds: 10)
        testNextTier(numberOfTrackerAds: 1010)
        testNextTier(numberOfTrackerAds: 5010)
        testNextTier(numberOfTrackerAds: 10010)
        testNextTier(numberOfTrackerAds: 25010)
        testNextTier(numberOfTrackerAds: 75010)
        testNextTier(numberOfTrackerAds: 100005)
        testNextTier(numberOfTrackerAds: 250002)
        testNextTier(numberOfTrackerAds: 500002)
        testNextTier(numberOfTrackerAds: 1000001)
    }

    private func testNextTier(numberOfTrackerAds: Int) {
        let existingTierList = Tier.allCases.filter({ numberOfTrackerAds < $0.rawValue })
        let existingNextTier = existingTierList.first?.nextTier

        switch numberOfTrackerAds {
            case 0..<Tier.specialTier.rawValue:
                XCTAssertTrue(existingNextTier == .newbieExclusiveTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.specialTier.rawValue..<Tier.newbieExclusiveTier.rawValue:
                XCTAssertTrue(existingNextTier == .casualExclusiveTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.newbieExclusiveTier.rawValue..<Tier.casualExclusiveTier.rawValue:
                XCTAssertTrue(existingNextTier == .regularExclusiveTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.casualExclusiveTier.rawValue..<Tier.regularExclusiveTier.rawValue:
                XCTAssertTrue(existingNextTier == .expertExclusiveTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.regularExclusiveTier.rawValue..<Tier.expertExclusiveTier.rawValue:
                XCTAssertTrue(existingNextTier == .professionalTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.expertExclusiveTier.rawValue..<Tier.professionalTier.rawValue:
                XCTAssertTrue(existingNextTier == .primeTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.professionalTier.rawValue..<Tier.primeTier.rawValue:
                XCTAssertTrue(existingNextTier == .grandTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            case Tier.primeTier.rawValue..<Tier.grandTier.rawValue:
                XCTAssertTrue(existingNextTier == .legendaryTier,
                              "Next Tier for # of ads: \(numberOfTrackerAds)")

            default:
                XCTAssertNil(existingNextTier)
        }
    }

    private var subject: [Tier]!
}
