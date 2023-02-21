// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import Shared
import BraveShared
import StoreKit
import UIKit
import BraveVPN

/// Singleton Manager handles App Review Criteria
public class AppReviewManager: ObservableObject {
  
  /// A main criteria that should be satisfied before checking sub-criteria
  public enum AppReviewMainCriteriaType: CaseIterable {
    case launchCount
    case daysInUse
    case sessionCrash
  }
  
  /// A sub-criteria that should be satisfied if all main criterias are valid
  public enum AppReviewSubCriteriaType: CaseIterable {
    case numberOfBookmarks
    case paidVPNSubscription
    case walletConnectedDapp
    case numberOfPlaylistItems
    case syncEnabledWithTabSync
  }
    
  @Published public var isReviewRequired = false
  
  private let launchCountLimit = 5
  private let bookmarksCountLimit = 5
  private let playlistCountLimit = 5
  private let dappConnectionPeriod = AppConstants.buildChannel.isPublic ? 7.days : 7.minutes
  private let daysInUseMaxPeriod = AppConstants.buildChannel.isPublic ? 7.days : 7.minutes
  private let daysInUseRequiredPeriod = 4
  
  // MARK: Legacy properties - Added for swithing back to legacy logic easily
  private let legacyAppReviewLogicEnabled = true
  let legacyFirstThreshold = 14
  let legacySecondThreshold = 41
  let legacyLastThreshold = 121
  private let legacyMinimumDaysBetweenReviewRequest = 60
  
  // MARK: Lifecycle
  
  public static var shared = AppReviewManager()
  
  // MARK: Legacy Review Methods
  
  public func legacyShouldRequestReview(date: Date = Date()) -> Bool {
     let launchCount = Preferences.Review.launchCount.value
     let threshold = Preferences.LegacyReview.threshold.value

     var daysSinceLastRequest = 0
     if let previousRequest = Preferences.LegacyReview.lastReviewDate.value {
       daysSinceLastRequest = Calendar.current.dateComponents([.day], from: previousRequest, to: date).day ?? 0
     } else {
       daysSinceLastRequest = legacyMinimumDaysBetweenReviewRequest
     }

     if launchCount <= threshold || daysSinceLastRequest < legacyMinimumDaysBetweenReviewRequest {
       return false
     }

     Preferences.LegacyReview.lastReviewDate.value = date

     switch threshold {
     case legacyFirstThreshold:
       Preferences.LegacyReview.threshold.value = legacySecondThreshold
     case legacySecondThreshold:
       Preferences.LegacyReview.threshold.value = legacyLastThreshold
     default:
       break
     }

     return legacyAppReviewLogicEnabled
   }
  
  // MARK: Review Handler Methods
  
  /// Method that handles If App Rating should be requested and request if condition is sucessful
  /// - Parameter currentScene: Current Scene where App Rating will be asked
  public func handleAppReview(for controller: UIViewController) {
    if shouldRequestReview() {
      if AppConstants.buildChannel.isPublic {
        // Request Review when the main-queue is free or on the next cycle.
        DispatchQueue.main.async {
          guard let windowScene = controller.currentScene else { return }
          SKStoreReviewController.requestReview(in: windowScene)
        }
      } else {
        let alert = UIAlertController(
          title: "Show App Rating",
          message: "Criteria is satified to Request Review",
          preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
        controller.present(alert, animated: true)
      }
    }
  }
  
  /// Method for handling changes to main criteria inside the various parts in application
  /// - Parameter mainCriteria: Type of the main Criteria
  public func processMainCriteria(for mainCriteria: AppReviewMainCriteriaType) {
    switch mainCriteria {
    case .daysInUse:
      var daysInUse = Preferences.Review.daysInUse.value
      
      daysInUse.append(Date())
      daysInUse = daysInUse.filter { $0 < Date().addingTimeInterval(daysInUseMaxPeriod) }
      
      Preferences.Review.daysInUse.value = daysInUse
    default:
      break
    }
  }
  
  /// Method for handling changes to sub criteria inside the various parts in application
  /// - Parameter subCriteria: Type of the sub Criteria
  public func processSubCriteria(for subCriteria: AppReviewSubCriteriaType) {
    switch subCriteria {
    case .walletConnectedDapp:
      // Saving when a user is connected its wallet to a Dapp
      Preferences.Review.dateWalletConnectedToDapp.value = Date()
    case .numberOfPlaylistItems:
      // Increase the number of playlist items added by the user
      Preferences.Review.numberPlaylistItemsAdded.value += 1
    case .numberOfBookmarks:
      // Increase the number of bookmarks added by the user
      Preferences.Review.numberBookmarksAdded.value += 1
    default:
      break
    }
  }
  
  /// Method checking If all main criterias are handled including at least one additional sub-criteria
  /// - Returns: Boolean value showing If App RAting should be requested
  func shouldRequestReview() -> Bool {
    var subCriteriaSatisfied = false
        
    // All of the main criterias should be met before additional situation can be checked
    let mainCriteriaSatisfied = AppReviewMainCriteriaType.allCases.allSatisfy(checkMainCriteriaSatisfied(for:))
    
    // Additionally if all main criterias are accomplished one of following conditions must also be met
    if mainCriteriaSatisfied {
      // One of the sub criterias also should be satisfied
      subCriteriaSatisfied = AppReviewSubCriteriaType.allCases.contains(where: checkSubCriteriaSatisfied(for:))
    }
    
    return legacyAppReviewLogicEnabled ? false : mainCriteriaSatisfied && subCriteriaSatisfied
  }
  
  /// This method is for checking App Review Sub Criteria is satisfied for a type
  /// - Parameter type: Main-criteria type
  /// - Returns:Boolean value showing particular criteria is satisfied
  private func checkMainCriteriaSatisfied(for type: AppReviewMainCriteriaType) -> Bool {
    switch type {
    case .launchCount:
      return Preferences.Review.launchCount.value >= launchCountLimit
    case .daysInUse:
      return Preferences.Review.daysInUse.value.count >= daysInUseRequiredPeriod
    case .sessionCrash:
      return !(!Preferences.AppState.backgroundedCleanly.value && AppConstants.buildChannel != .debug)
    }
  }
  
  /// This method is for checking App Review Sub Criteria is satisfied for a type
  /// - Parameter type: Sub-criteria type
  /// - Returns: Boolean value showing particular criteria is satisfied
  private func checkSubCriteriaSatisfied(for type: AppReviewSubCriteriaType) -> Bool {
    switch type {
    case .numberOfBookmarks:
      return Preferences.Review.numberBookmarksAdded.value >= bookmarksCountLimit
    case .paidVPNSubscription:
      if case .purchased(_) = BraveVPN.vpnState {
        return true
      }
      return false
    case .walletConnectedDapp:
      guard let connectedDappDate = Preferences.Review.dateWalletConnectedToDapp.value else {
        return false
      }
      return Date() < connectedDappDate.addingTimeInterval(dappConnectionPeriod)
    case .numberOfPlaylistItems:
      return Preferences.Review.numberPlaylistItemsAdded.value >= playlistCountLimit
    case .syncEnabledWithTabSync:
      return Preferences.Chromium.syncEnabled.value && Preferences.Chromium.syncOpenTabsEnabled.value
    }
  }
}
