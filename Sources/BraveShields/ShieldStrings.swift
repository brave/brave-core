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

// MARK: - Anti Ad-Block Warning

public extension Strings.Shields {
  /// A title for a popup that tells the user we recommend turning shields off for this site
  static let antiAdBlockWarningTitle = NSLocalizedString(
    "AntiAdBlockWarningTitle", tableName: "BraveShared", bundle: .module,
    value: "Adjust Ad Block Settings For This Site",
    comment: "A title for a popup that tells the user we recommend turning shields off for this site."
  )
  
  /// A descriptive message explaining this site's ad-blocking crackdown
  static let antiAdBlockWarningDescription = NSLocalizedString(
    "AntiAdBlockWarningDescription", tableName: "BraveShared", bundle: .module,
    value: "This site has begun blocking some ad blockers, which means their site may not work as expected.",
    comment: "A descriptive message explaining this site's ad-blocking crackdown."
  )
  
  /// A descriptive message explaining to disable shields on this site
  static let antiAdBlockWarningDescription2 = NSLocalizedString(
    "AntiAdBlockWarningDescription2", tableName: "BraveShared", bundle: .module,
    value: "To address this issue, Brave can adjust your shields settings for you. Once adjusted, you can try watching this content in Brave Player instead.",
    comment: "A descriptive message explaining to disable shields on this site."
  )
  
  /// A button that disables ad-blocking and uses brave player
  static let antiAdBlockWarningConfirmationButton = NSLocalizedString(
    "AntiAdBlockWarningConfirmationButton", tableName: "BraveShared", bundle: .module,
    value: "Adjust Shields For Me",
    comment: "A button that disables ad-blocking and uses brave player."
  )
  
  /// A button that dismisses the warning and does nothing
  static let antiAdBlockWarningDismissButton = NSLocalizedString(
    "AntiAdBlockWarningDismissButton", tableName: "BraveShared", bundle: .module,
    value: "Keep Current Settings",
    comment: "A button that dismisses the warning and does nothing."
  )
  
  /// A discription of the Brave Player
  static let antiAdBlockWarningBravePlayerDescription = NSLocalizedString(
    "AntiAdBlockWarningBravePlayerDescription", tableName: "BraveShared", bundle: .module,
    value: "**Brave Player** is your ticket to an ad-free and uninterrupted video experience.",
    comment: "A discription of the Brave Player."
  )
}

// MARK: - Brave Player

public extension Strings.Shields {
  /// Title for the brave player feature
  static let bravePlayer = NSLocalizedString(
    "BravePlayer", tableName: "BraveShared", bundle: .module,
    value: "Brave Player",
    comment: "Title for the brave player feature"
  )
  
  /// Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar
  static let bravePlayerInfoTitle = NSLocalizedString(
    "BravePlayerInfoTitle", tableName: "BraveShared", bundle: .module,
    value: "Watch In Brave Player Instead",
    comment: "Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar."
  )
  
  /// A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time
  static let bravePlayerInfoMessage = NSLocalizedString(
    "BravePlayerInfoMessage", tableName: "BraveShared", bundle: .module,
    value: "Brave Player lets you watch videos without interruptions.",
    comment: "A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time."
  )
  
  /// A label for a toggle that enables automatic launching of brave player for certain sites
  static let bravePlayerAlwaysOpenVideoLinks = NSLocalizedString(
    "BravePlayerAlwaysOpenYouTubeLinks", tableName: "BraveShared", bundle: .module,
    value: "Always open videos from this site with Brave Player",
    comment: "A label for a toggle that enables automatic launching of brave player for certain sites"
  )
  
  /// A button that confirms to use the brave player
  static let bravePlayerConfirmButton = NSLocalizedString(
    "BravePlayerConfirmButton", tableName: "BraveShared", bundle: .module,
    value: "Try It Out",
    comment: "A button that confirms to use the brave player."
  )
  
  /// A button that ignores the brave player
  static let bravePlayerDismissButton = NSLocalizedString(
    "BravePlayerDismissButton", tableName: "BraveShared", bundle: .module,
    value: "Not Now",
    comment: "A button that ignores the brave player"
  )
}

// MARK: - Shields
public extension Strings.Shields {
  /// A label of the GPC toggle
  static let enableGPCLabel = NSLocalizedString(
    "EnableGPCLabel", tableName: "BraveShared", bundle: .module,
    value: "Enable Global Privacy Control",
    comment: "A label of the GPC toggle"
  )
  
  /// A description of what the Enable GPC toggle does
  static let enableGPCDescription = NSLocalizedString(
    "EnableGPCDescription", tableName: "BraveShared", bundle: .module,
    value: "Enable the Global Privacy Control JS API",
    comment: "A description of what the Enable GPC toggle does"
  )
}
