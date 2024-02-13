// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared
import Preferences
import BraveVPN
@testable import Growth

class AppReviewManagerTests: XCTestCase {
  
  private enum MainCriteriaType {
    case launchCount
    case daysInUse
    case sessionCrash
    case daysInBetweenReview
  }
  
  private enum SubCriteriaType {
    case numberOfBookmarks
    case paidVPNSubscription
    case walletConnectedDapp
    case numberOfPlaylistItems
    case syncEnabledWithTabSync
  }
  
  override func setUp() {
    super.setUp()
    
    resetAppReviewConstants()
  }
  
  override func tearDown() {
    super.tearDown()
   
    resetAppReviewConstants()
  }

  func testRevisedReviewPassingMainCriteriasPassingSubCriteria() {
    // Paid VPN Subcription
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: false, passingCriteria: .paidVPNSubscription)
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))

    
    // Number Of Bookmarks
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: false, passingCriteria: .numberOfBookmarks)
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))


    
    // Sync Tab Sync Enabled
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: false, passingCriteria: .syncEnabledWithTabSync)
    
    // Number Of Playlist Items
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))

    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))

    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: false, passingCriteria: .numberOfPlaylistItems)
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))
  }
  
  func testRevisedReviewFailigMainCriteriaPassingVPNSubCriteria() {
    resetAppReviewConstants()
    
    generateMainCriterias(passing: false, failingCriteria: nil)
    generateSubCriterias(failing: false, passingCriteria: nil)
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))
  }
  
  func testRevisedReviewPassingMainCriteriaFailingVPNSubCriteria() {
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: true, passingCriteria: nil)
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))

    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))
  }
  
  func testRevisedReviewFailigMainCriteriaFailingVPNSubCriteria() {
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)
    generateSubCriterias(failing: true, passingCriteria: nil)
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssert(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))
  }
  
  func testRevisedReviewFailingMainCriteriaDaysInBetween() {
    resetAppReviewConstants()
    
    generateMainCriterias(passing: false, failingCriteria: .daysInBetweenReview)
    generateSubCriterias(failing: false, passingCriteria: .numberOfBookmarks)
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revised))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .revisedCrossPlatform))
    
    XCTAssertFalse(AppReviewManager.shared.checkLogicCriteriaSatisfied(for: .newsRatingCard))
  }
  
  func testShouldShowNewsRatingCard() {
    resetAppReviewConstants()
    
    generateMainCriterias(passing: true, failingCriteria: nil)

    // Fresh Criteria car never shown
    XCTAssertTrue(AppReviewManager.shared.shouldShowNewsRatingCard())
    
    Preferences.Review.newsCardShownDate.value = Date() + 5.days
    
    // Less than week passed for next card
    XCTAssertFalse(AppReviewManager.shared.shouldShowNewsRatingCard())

    Preferences.Review.newsCardShownDate.value = Date() + 10.days

    // More than a week passed since previous card reveal
    XCTAssertTrue(AppReviewManager.shared.shouldShowNewsRatingCard())
  }
  
  private func resetAppReviewConstants() {
    AppConstants.buildChannel = .debug
    Preferences.Review.launchCount.reset()
    Preferences.Review.daysInUse.reset()
    Preferences.AppState.backgroundedCleanly.reset()
    Preferences.VPN.expirationDate.value = nil
    Preferences.Review.numberBookmarksAdded.reset()
    Preferences.Review.dateWalletConnectedToDapp.value = nil
    Preferences.Review.numberPlaylistItemsAdded.reset()
    Preferences.Chromium.syncEnabled.reset()
    Preferences.Chromium.syncOpenTabsEnabled.reset()
    Preferences.Review.lastReviewDate.value = nil
    Preferences.Review.newsCardShownDate.value = nil
  }
  
  private func generateDaysOfUse(active: Bool) {
    if active {
      var activeUserDates:[Date] = []
      var numberOfActiveDaysInWeek = 5
      
      while numberOfActiveDaysInWeek > 0 {
        activeUserDates.append(Date().addingTimeInterval(-numberOfActiveDaysInWeek.days))
        numberOfActiveDaysInWeek -= 1
      }
      
      Preferences.Review.daysInUse.value = activeUserDates
    } else {
      Preferences.Review.daysInUse.value = [Date()]
    }
  }
  
  private func generateMainCriterias(passing: Bool, failingCriteria: MainCriteriaType?) {
    if passing {
      Preferences.Review.launchCount.value = 5
      Preferences.AppState.backgroundedCleanly.value = true
      generateDaysOfUse(active: true)
      Preferences.Review.lastReviewDate.value = nil
      return
    }
    
    if let failingCriteria = failingCriteria {
      switch failingCriteria {
      case .launchCount:
        Preferences.Review.launchCount.value = 3
        Preferences.AppState.backgroundedCleanly.value = true
        generateDaysOfUse(active: true)
        Preferences.Review.lastReviewDate.value = nil
      case .sessionCrash:
        Preferences.Review.launchCount.value = 5
        Preferences.AppState.backgroundedCleanly.value = false
        generateDaysOfUse(active: true)
        Preferences.Review.lastReviewDate.value = nil
      case .daysInUse:
        Preferences.Review.launchCount.value = 5
        Preferences.AppState.backgroundedCleanly.value = true
        generateDaysOfUse(active: false)
        Preferences.Review.lastReviewDate.value = nil
      case .daysInBetweenReview:
        Preferences.Review.launchCount.value = 5
        Preferences.AppState.backgroundedCleanly.value = true
        generateDaysOfUse(active: false)
        Preferences.Review.lastReviewDate.value = Date()
      }
    }
  }

  private func generateSubCriterias(failing: Bool, passingCriteria: SubCriteriaType?) {
    if failing {
      Preferences.Review.numberBookmarksAdded.value = 1
      Preferences.Review.dateWalletConnectedToDapp.value = nil
      Preferences.Review.numberPlaylistItemsAdded.value = 3
      Preferences.VPN.expirationDate.value = Date().addingTimeInterval(-5.days)
      Preferences.Chromium.syncOpenTabsEnabled.value = false
      Preferences.Chromium.syncEnabled.value = false
      return
    }
    
    if let passingCriteria = passingCriteria {
      switch passingCriteria {
      case .numberOfBookmarks:
        Preferences.Review.numberBookmarksAdded.value = 5
      case .numberOfPlaylistItems:
        Preferences.Review.numberPlaylistItemsAdded.value = 5
      case .walletConnectedDapp:
        Preferences.Review.dateWalletConnectedToDapp.value = Date().addingTimeInterval(-5.days)
      case.syncEnabledWithTabSync:
        Preferences.Chromium.syncOpenTabsEnabled.value = true
        Preferences.Chromium.syncEnabled.value = true
      case .paidVPNSubscription:
        Preferences.VPN.expirationDate.value = Date().addingTimeInterval(5.days)
      }
    }
  }
}
