// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct Shields {
    public static let shieldsAdStats = NSLocalizedString("AdsrBlocked", bundle: .module, value: "Ads \nBlocked", comment: "Shields Ads Stat")
    public static let shieldsAdAndTrackerStats = NSLocalizedString("AdsAndTrackersrBlocked", bundle: .module, value: "Trackers & ads blocked", comment: "Shields Ads Stat")
    public static let shieldsTrackerStats = NSLocalizedString("TrackersrBlocked", bundle: .module, value: "Trackers \nBlocked", comment: "Shields Trackers Stat")
    public static let dataSavedStat = NSLocalizedString("DataSavedStat", bundle: .module, value: "Est. Data \nSaved", comment: "Data Saved Shield Stat")
    public static let shieldsTimeStats = NSLocalizedString("EstTimerSaved", bundle: .module, value: "Est. Time \nSaved", comment: "Shields Time Saved Stat")
    public static let shieldsTimeStatsHour = NSLocalizedString("ShieldsTimeStatsHour", bundle: .module, value: "h", comment: "Time Saved Hours")
    public static let shieldsTimeStatsMinutes = NSLocalizedString("ShieldsTimeStatsMinutes", bundle: .module, value: "min", comment: "Time Saved Minutes")
    public static let shieldsTimeStatsSeconds = NSLocalizedString("ShieldsTimeStatsSeconds", bundle: .module, value: "s", comment: "Time Saved Seconds")
    public static let shieldsTimeStatsDays = NSLocalizedString("ShieldsTimeStatsDays", bundle: .module, value: "d", comment: "Time Saved Days")
  }
}

// MARK: - Trackers and Ad-Blocking

public extension Strings.Shields {
  /// A label for a shield option that allows you to switch between different blocking levels for tracker and ads blocking. Options include disabled, standard and aggressive.
  static let trackersAndAdsBlocking = NSLocalizedString(
    "TrackersAndAdsBlocking", tableName: "BraveShared", bundle: .module,
    value: "Trackers & Ads Blocking",
    comment: "A label for a shield option that allows you to switch between different blocking levels for tracker and ads blocking. Options include disabled, standard and aggressive."
  )
  /// A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive.
  static let trackersAndAdsBlockingDescription = NSLocalizedString(
    "BlockAdsAndTrackingDescription", tableName: "BraveShared", bundle: .module,
    value: "Prevents ads, popups, and trackers from loading.",
    comment: "A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive."
  )
  /// The option the user can select to disable ad and tracker blocking
  static let trackersAndAdsBlockingDisabled = NSLocalizedString(
    "BlockAdsAndTrackingDisabled", tableName: "BraveShared", bundle: .module,
    value: "Disabled",
    comment: "The option the user can select to disable ad and tracker blocking"
  )
  /// The option the user can select to do aggressive ad and tracker blocking
  static let trackersAndAdsBlockingAggressive = NSLocalizedString(
    "BlockAdsAndTrackingAggressive", tableName: "BraveShared", bundle: .module,
    value: "Aggressive",
    comment: "The option the user can select to do aggressive ad and tracker blocking"
  )
  /// The option the user can select to do standard (non-aggressive) ad and tracker blocking
  static let trackersAndAdsBlockingStandard = NSLocalizedString(
    "BlockAdsAndTrackingStandard", tableName: "BraveShared", bundle: .module,
    value: "Standard",
    comment: "The option the user can select to do standard (non-aggressive) ad and tracker blocking"
  )
}
