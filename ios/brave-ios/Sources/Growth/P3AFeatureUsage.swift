// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import OSLog

/// Handles recording usage of a particular feature
///
/// `P3AFeatureUsage` records first and last usage 
public struct P3AFeatureUsage {
  public let name: String
  public let histogram: String
  public let returningUserHistogram: String?
  
  public let firstUsageOption: Preferences.Option<Date?>
  public let lastUsageOption: Preferences.Option<Date?>
  public let returnedDayAfterOption: Preferences.Option<Bool>
  
  public var date: () -> Date = { Date() }
  
  public init(name: String, histogram: String, returningUserHistogram: String? = nil) {
    self.name = name
    self.histogram = histogram
    self.returningUserHistogram = returningUserHistogram
    self.firstUsageOption = .init(key: "p3a.feature-usage.\(name).first", default: nil)
    self.lastUsageOption = .init(key: "p3a.feature-usage.\(name).last", default: nil)
    self.returnedDayAfterOption = .init(key: "p3a.feature-usage.\(name).returned-day-after", default: false)
  }
  
  /// Resets the storage associated with this feature usage
  public func reset() {
    firstUsageOption.reset()
    lastUsageOption.reset()
    returnedDayAfterOption.reset()
  }
  
  /// Records a histogram entry with the current last usage
  ///
  /// Call this directly if you want to record a histogram without updating the features usage such as
  /// when the app launches.
  public func recordHistogram() {
    // Record last usage to bucket if there's an associated p3a question
    UmaHistogramRecordLastFeatureUsage(histogram, option: lastUsageOption)
    Logger.module.info("Recorded P3A feature usage histogram: \(histogram)")
    
    if firstUsageOption.value != nil {
      recordReturningUsageMetric()
    }
  }
  
  /// Updates the usage and records a histogram with that new usage
  ///
  /// This method also sets the first usage preference if one has not been set yet.
  public func recordUsage() {
    // Update usage prefs
    let calendar = Calendar(identifier: .gregorian)
    let now = calendar.startOfDay(for: date())
    if firstUsageOption.value == nil {
      firstUsageOption.value = now
    }
    lastUsageOption.value = now
    recordHistogram()
    Logger.module.info("Recorded P3A feature usage for \(name): \(now)")
  }
  
  enum ReturningUserState: Int, CaseIterable {
    case neverUsed = 0
    case usedButImNotAFirstTimeUserThisWeek = 1
    case firstTimeUserThisWeekButDidntReturnDuringWeek = 2
    case firstTimeUserThisWeekAndUsedItTheFollowingDay = 3
    case firstTimeUserThisWeekAndUsedItThisWeek = 4
  }
  
  var returningUserState: ReturningUserState {
    let calendar = Calendar(identifier: .gregorian)
    guard let firstUsage = firstUsageOption.value,
          let lastUsage = lastUsageOption.value,
          let firstUsageCutoff = calendar.date(byAdding: .day, value: 7, to: firstUsage) else {
      return .neverUsed
    }
    let today = calendar.startOfDay(for: date())
    // Check if we're passed the first week
    if today > firstUsageCutoff {
      // Used it after the first week
      return .usedButImNotAFirstTimeUserThisWeek
    } else {
      // Today's the last day of the week, and the user did not return
      if today == firstUsageCutoff, lastUsage == firstUsage {
        return .firstTimeUserThisWeekButDidntReturnDuringWeek
      }
      // Last usage happened after yesterday, meaning we used it the following day
      if let yesterday = calendar.date(byAdding: .day, value: -1, to: today), (firstUsage == yesterday && lastUsage == today) || returnedDayAfterOption.value {
        returnedDayAfterOption.value = true
        return .firstTimeUserThisWeekAndUsedItTheFollowingDay
      }
      return .firstTimeUserThisWeekAndUsedItThisWeek
    }
  }
  
  /// Records a typical returning usage metric histogram if you've set `returningUserHistogram`
  private func recordReturningUsageMetric() {
    guard let name = returningUserHistogram else { return }
    let returningUserState = returningUserState
    UmaHistogramEnumeration(name, sample: returningUserState)
    Logger.module.info("Recorded P3A returning usage for \(name): \(String(describing: returningUserState))")
  }
}
