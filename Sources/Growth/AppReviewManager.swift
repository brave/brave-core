// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import Shared
import Preferences
import StoreKit
import UIKit
import BraveVPN

/// Singleton Manager handles App Review Criteria
public class AppReviewManager: ObservableObject {
  
  struct Constants {
    // Legacy Review Constants
    static let firstThreshold = 14
    static let secondThreshold = 41
    static let lastThreshold = 121
    static let minimumDaysBetweenReviewRequest = 60

    // Revised Review Constants
    static let launchCountLimit = 5
    static let bookmarksCountLimit = 5
    static let playlistCountLimit = 5
    static let dappConnectionPeriod = AppConstants.buildChannel.isPublic ? 7.days : 7.minutes
    static let daysInUseMaxPeriod = AppConstants.buildChannel.isPublic ? 7.days : 7.minutes
    static let daysInUseRequiredPeriod = 4
    static let revisedMinimumDaysBetweenReviewRequest = 30
  }
  
  /// A enumeration for which type of App Review Logic will be used
  /// Helper for quick changes between different types of logic
  /// Active request review type can be changed using activeAppReviewLogicType
  public enum AppReviewLogicType: CaseIterable {
    // Legacy Review Logic used as baseline
    // Only checking Launch Count and Days in Between reviews
    // Performing Rating Request in App Launch
    case legacy
    // Revised Review Logic used to test
    // various success scenarios for review request
    // Checking various main criteria and sub criteria
    // Performing Rating Request as a result of some actions
    // This logic is reverted to legacy logic later
    // Context: https://github.com/brave/brave-ios/pull/6210
    case revised
    // Revised Review Logic which aligns with Android platform
    // Checking various main criteria and sub criteria
    // Performing Rating Request in App Launch
    case revisedCrossPlatform
    
    var mainCriteria: [AppReviewMainCriteriaType] {
      switch self {
      case .legacy:
        return [.threshold]
      case .revised:
        return [.launchCount, .daysInUse, .sessionCrash]
      case .revisedCrossPlatform:
        return [.launchCount, .daysInUse, .sessionCrash, .daysInBetweenReview]
      }
    }
    
    var subCriteria: [AppReviewSubCriteriaType] {
      switch self {
      case .legacy:
        return []
      case .revised:
        return [.numberOfBookmarks, .paidVPNSubscription, .walletConnectedDapp,
                .numberOfPlaylistItems, .syncEnabledWithTabSync]
      case .revisedCrossPlatform:
        return [.numberOfBookmarks, .paidVPNSubscription]
      }
    }
  }
  
  /// A main criteria that should be satisfied before checking sub-criteria
  public enum AppReviewMainCriteriaType: CaseIterable {
    case threshold
    case launchCount
    case daysInUse
    case sessionCrash
    case daysInBetweenReview
  }
  
  /// A sub-criteria that should be satisfied if all main criterias are valid
  public enum AppReviewSubCriteriaType: CaseIterable {
    case numberOfBookmarks
    case paidVPNSubscription
    case walletConnectedDapp
    case numberOfPlaylistItems
    case syncEnabledWithTabSync
  }
    
  @Published public var isRevisedReviewRequired = false
  private var activeAppReviewLogicType: AppReviewLogicType = .legacy
  
  // MARK: Lifecycle
  
  public static var shared = AppReviewManager()
  
  // MARK: Review Request Handling
  
  public func handleAppReview(for logicType: AppReviewLogicType, using controller: UIViewController) {
    guard logicType == activeAppReviewLogicType else {
      return
    }
    
    if shouldRequestReview(for: logicType) {
      guard AppConstants.buildChannel.isPublic else {
        let alert = UIAlertController(
          title: "Show App Rating",
          message: "Criteria is satified to Request Review for Logic Type \(logicType)",
          preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
        controller.present(alert, animated: true)
        
        return
      }
      
      DispatchQueue.main.async {
        if let windowScene = controller.currentScene {
          SKStoreReviewController.requestReview(in: windowScene)
        }
      }
    }
  }
  
  // MARK: Review Request Inquiry

  public func shouldRequestReview(for logicType: AppReviewLogicType, date: Date = Date()) -> Bool {
    // All of the main criterias should be met before additional situation can be checked
    let mainCriteriaSatisfied = logicType.mainCriteria.allSatisfy({ criteria in
      checkMainCriteriaSatisfied(for: criteria, date: date)
    })
    
    var subCriteriaSatisfied = true
    if !logicType.subCriteria.isEmpty {
      // Additionally if all main criterias are accomplished one of following conditions must also be met
      if mainCriteriaSatisfied {
        subCriteriaSatisfied = logicType.subCriteria.contains(where: checkSubCriteriaSatisfied(for:))
      }
    }
    
    return mainCriteriaSatisfied && subCriteriaSatisfied
  }
  
  // MARK: Review Criteria Process

  /// Method for handling changes to main criteria inside the various parts in application
  /// - Parameter mainCriteria: Type of the main Criteria
  public func processMainCriteria(for mainCriteria: AppReviewMainCriteriaType) {
    switch mainCriteria {
    case .daysInUse:
      var daysInUse = Preferences.Review.daysInUse.value
      
      daysInUse.append(Date())
      daysInUse = daysInUse.filter { $0 < Date().addingTimeInterval(Constants.daysInUseMaxPeriod) }
      
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
  
  /// This method is for checking App Review Sub Criteria is satisfied for a type
  /// - Parameter type: Main-criteria type
  /// - Returns:Boolean value showing particular criteria is satisfied
  private func checkMainCriteriaSatisfied(for type: AppReviewMainCriteriaType, date: Date = Date()) -> Bool {
    switch type {
    case .threshold:
      let launchCount = Preferences.Review.launchCount.value
      let threshold = Preferences.Review.threshold.value

      var daysSinceLastRequest = 0
      if let previousRequest = Preferences.Review.lastReviewDate.value {
        daysSinceLastRequest = Calendar.current.dateComponents([.day], from: previousRequest, to: date).day ?? 0
      } else {
        daysSinceLastRequest = Constants.minimumDaysBetweenReviewRequest
      }

      if launchCount <= threshold || daysSinceLastRequest < Constants.minimumDaysBetweenReviewRequest {
        return false
      }

      Preferences.Review.lastReviewDate.value = date

      switch threshold {
      case Constants.firstThreshold:
        Preferences.Review.threshold.value = Constants.secondThreshold
      case Constants.secondThreshold:
        Preferences.Review.threshold.value = Constants.lastThreshold
      default:
        break
      }

      return true
    case .launchCount:
      return Preferences.Review.launchCount.value >= Constants.launchCountLimit
    case .daysInUse:
      return Preferences.Review.daysInUse.value.count >= Constants.daysInUseRequiredPeriod
    case .sessionCrash:
      return !(!Preferences.AppState.backgroundedCleanly.value && AppConstants.buildChannel != .debug)
    case .daysInBetweenReview:
      var daysSinceLastRequest = 0
      if let previousRequest = Preferences.Review.lastReviewDate.value {
        daysSinceLastRequest = Calendar.current.dateComponents([.day], from: previousRequest, to: date).day ?? 0
      } else {
        Preferences.Review.lastReviewDate.value = date
        return true
      }

      if daysSinceLastRequest < Constants.revisedMinimumDaysBetweenReviewRequest {
        return false
      }

      Preferences.Review.lastReviewDate.value = date
      return true
    }
  }
  
  /// This method is for checking App Review Sub Criteria is satisfied for a type
  /// - Parameter type: Sub-criteria type
  /// - Returns: Boolean value showing particular criteria is satisfied
  private func checkSubCriteriaSatisfied(for type: AppReviewSubCriteriaType) -> Bool {
    switch type {
    case .numberOfBookmarks:
      return Preferences.Review.numberBookmarksAdded.value >= Constants.bookmarksCountLimit
    case .paidVPNSubscription:
      if case .purchased(_) = BraveVPN.vpnState {
        return true
      }
      return false
    case .walletConnectedDapp:
      guard let connectedDappDate = Preferences.Review.dateWalletConnectedToDapp.value else {
        return false
      }
      return Date() < connectedDappDate.addingTimeInterval(Constants.dappConnectionPeriod)
    case .numberOfPlaylistItems:
      return Preferences.Review.numberPlaylistItemsAdded.value >= Constants.playlistCountLimit
    case .syncEnabledWithTabSync:
      return Preferences.Chromium.syncEnabled.value && Preferences.Chromium.syncOpenTabsEnabled.value
    }
  }
}
