// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct Shields {
    public static let shieldsAdStats = NSLocalizedString(
      "AdsrBlocked",
      bundle: .module,
      value: "Ads \nBlocked",
      comment: "Shields Ads Stat"
    )
    public static let shieldsAdAndTrackerStats = NSLocalizedString(
      "AdsAndTrackersrBlocked",
      bundle: .module,
      value: "Trackers & ads blocked",
      comment: "Shields Ads Stat"
    )
    public static let shieldsTrackerStats = NSLocalizedString(
      "TrackersrBlocked",
      bundle: .module,
      value: "Trackers \nBlocked",
      comment: "Shields Trackers Stat"
    )
    public static let dataSavedStat = NSLocalizedString(
      "DataSavedStat",
      bundle: .module,
      value: "Est. Data \nSaved",
      comment: "Data Saved Shield Stat"
    )
    public static let shieldsTimeStats = NSLocalizedString(
      "EstTimerSaved",
      bundle: .module,
      value: "Est. Time \nSaved",
      comment: "Shields Time Saved Stat"
    )
    public static let shieldsTimeStatsHour = NSLocalizedString(
      "ShieldsTimeStatsHour",
      bundle: .module,
      value: "h",
      comment: "Time Saved Hours"
    )
    public static let shieldsTimeStatsMinutes = NSLocalizedString(
      "ShieldsTimeStatsMinutes",
      bundle: .module,
      value: "min",
      comment: "Time Saved Minutes"
    )
    public static let shieldsTimeStatsSeconds = NSLocalizedString(
      "ShieldsTimeStatsSeconds",
      bundle: .module,
      value: "s",
      comment: "Time Saved Seconds"
    )
    public static let shieldsTimeStatsDays = NSLocalizedString(
      "ShieldsTimeStatsDays",
      bundle: .module,
      value: "d",
      comment: "Time Saved Days"
    )
  }
}

// MARK: - Trackers and Ad-Blocking

extension Strings.Shields {
  /// A label for a shield option that allows you to switch between different blocking levels for tracker and ads blocking. Options include disabled, standard and aggressive.
  public static let trackersAndAdsBlocking = NSLocalizedString(
    "TrackersAndAdsBlocking",
    tableName: "BraveShared",
    bundle: .module,
    value: "Trackers & Ads Blocking",
    comment:
      "A label for a shield option that allows you to switch between different blocking levels for tracker and ads blocking. Options include disabled, standard and aggressive."
  )
  /// A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive.
  public static let trackersAndAdsBlockingDescription = NSLocalizedString(
    "BlockAdsAndTrackingDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Prevents ads, popups, and trackers from loading.",
    comment:
      "A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive."
  )
  /// The option the user can select to disable ad and tracker blocking
  public static let trackersAndAdsBlockingDisabled = NSLocalizedString(
    "BlockAdsAndTrackingDisabled",
    tableName: "BraveShared",
    bundle: .module,
    value: "Disabled",
    comment: "The option the user can select to disable ad and tracker blocking"
  )
  /// The option the user can select to do aggressive ad and tracker blocking
  public static let trackersAndAdsBlockingAggressive = NSLocalizedString(
    "BlockAdsAndTrackingAggressive",
    tableName: "BraveShared",
    bundle: .module,
    value: "Aggressive",
    comment: "The option the user can select to do aggressive ad and tracker blocking"
  )
  /// The option the user can select to do standard (non-aggressive) ad and tracker blocking
  public static let trackersAndAdsBlockingStandard = NSLocalizedString(
    "BlockAdsAndTrackingStandard",
    tableName: "BraveShared",
    bundle: .module,
    value: "Standard",
    comment:
      "The option the user can select to do standard (non-aggressive) ad and tracker blocking"
  )
}

// MARK: - Anti Ad-Block Warning

extension Strings.Shields {
  /// A title for a popup that tells the user we recommend turning shields off for this site
  public static let antiAdBlockWarningTitle = NSLocalizedString(
    "AntiAdBlockWarningTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Adjust Ad Block Settings For This Site",
    comment:
      "A title for a popup that tells the user we recommend turning shields off for this site."
  )

  /// A descriptive message explaining this site's ad-blocking crackdown
  public static let antiAdBlockWarningDescription = NSLocalizedString(
    "AntiAdBlockWarningDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "This site has begun blocking some ad blockers, which means their site may not work as expected.",
    comment: "A descriptive message explaining this site's ad-blocking crackdown."
  )

  /// A descriptive message explaining to disable shields on this site
  public static let antiAdBlockWarningDescription2 = NSLocalizedString(
    "AntiAdBlockWarningDescription2",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "To address this issue, Brave can adjust your shields settings for you. Once adjusted, you can try watching this content in Brave Player instead.",
    comment: "A descriptive message explaining to disable shields on this site."
  )

  /// A button that disables ad-blocking and uses brave player
  public static let antiAdBlockWarningConfirmationButton = NSLocalizedString(
    "AntiAdBlockWarningConfirmationButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Adjust Shields For Me",
    comment: "A button that disables ad-blocking and uses brave player."
  )

  /// A button that dismisses the warning and does nothing
  public static let antiAdBlockWarningDismissButton = NSLocalizedString(
    "AntiAdBlockWarningDismissButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Keep Current Settings",
    comment: "A button that dismisses the warning and does nothing."
  )

  /// A discription of the Brave Player
  public static let antiAdBlockWarningBravePlayerDescription = NSLocalizedString(
    "AntiAdBlockWarningBravePlayerDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "**Brave Player** is your ticket to an ad-free and uninterrupted video experience.",
    comment: "A discription of the Brave Player."
  )
}

// MARK: - Brave Player

extension Strings.Shields {
  /// Title for the brave player feature
  public static let bravePlayer = NSLocalizedString(
    "BravePlayer",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Player",
    comment: "Title for the brave player feature"
  )

  /// Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar
  public static let bravePlayerInfoTitle = NSLocalizedString(
    "BravePlayerInfoTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Watch In Brave Player Instead",
    comment:
      "Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar."
  )

  /// A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time
  public static let bravePlayerInfoMessage = NSLocalizedString(
    "BravePlayerInfoMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Player lets you watch videos without interruptions.",
    comment:
      "A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time."
  )

  /// A label for a toggle that enables automatic launching of brave player for certain sites
  public static let bravePlayerAlwaysOpenVideoLinks = NSLocalizedString(
    "BravePlayerAlwaysOpenYouTubeLinks",
    tableName: "BraveShared",
    bundle: .module,
    value: "Always open videos from this site with Brave Player",
    comment:
      "A label for a toggle that enables automatic launching of brave player for certain sites"
  )

  /// A button that confirms to use the brave player
  public static let bravePlayerConfirmButton = NSLocalizedString(
    "BravePlayerConfirmButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Try It Out",
    comment: "A button that confirms to use the brave player."
  )

  /// A button that ignores the brave player
  public static let bravePlayerDismissButton = NSLocalizedString(
    "BravePlayerDismissButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Not Now",
    comment: "A button that ignores the brave player"
  )
}

// MARK: - GPC

extension Strings.Shields {
  /// A label of the GPC toggle
  public static let enableGPCLabel = NSLocalizedString(
    "EnableGPCLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable Global Privacy Control",
    comment: "A label of the GPC toggle"
  )

  /// A description of what the Enable GPC toggle does
  public static let enableGPCDescription = NSLocalizedString(
    "EnableGPCDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable the Global Privacy Control JS API",
    comment: "A description of what the Enable GPC toggle does"
  )
}

// MARK: - Blocked Page

extension Strings.Shields {
  /// A tab title that appears when a page was blocked
  public static let domainBlockedTitle = NSLocalizedString(
    "DomainBlockedTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Domain Blocked",
    comment: "A tab title for the warning page that appears when a page was blocked"
  )

  /// A title in the warning page that appears when a page was blocked
  public static let domainBlockedPageTitle = NSLocalizedString(
    "DomainBlockedPageTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "This Site May Attempt to Track You Across Other Sites",
    comment: "A title in the warning page that appears when a page was blocked"
  )

  /// A title in the warning page that appears when a page was blocked
  public static let domainBlockedPageMessage = NSLocalizedString(
    "DomainBlockedPageMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave has prevented the following site from loading:",
    comment: "A message in the warning page that appears when a page was blocked"
  )

  /// A description in the warning page that appears when a page was blocked
  public static let domainBlockedPageDescription = NSLocalizedString(
    "DomainBlockedPageDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Because you requested to aggressively block trackers and ads, Brave is blocking this site before the first network connection.",
    comment: "A description in the warning page that appears when a page was blocked"
  )

  /// Text for a button in a blocked page info screen that allows you to proceed regardless of the privacy warning
  public static let domainBlockedProceedAction = NSLocalizedString(
    "DomainBlockedProceedAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Proceed",
    comment:
      "Text for a button in a blocked page info screen that allows you to proceed regardless of the privacy warning"
  )

  /// A description in the warning page that appears when a page was blocked
  public static let domainBlockedGoBackAction = NSLocalizedString(
    "DomainBlockedGoBackAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Go Back",
    comment:
      "Text for a button in a blocked page info screen that takes you back where you came from"
  )
}

// MARK: - HTTPS Upgrades

extension Strings.Shields {
  /// The option the user can select to do aggressive ad and tracker blocking
  public static let httpsUpgradeLevelStrict = NSLocalizedString(
    "HttpsUpgradeLevelStrict",
    tableName: "BraveShared",
    bundle: .module,
    value: "Strict",
    comment: "The option the user can select to do strict https upgrading"
  )
  /// The option the user can select for the type of https upgrading
  public static let upgradeConnectionsToHTTPS = NSLocalizedString(
    "UpgradeConnectionsToHTTPS",
    tableName: "BraveShared",
    bundle: .module,
    value: "Upgrade Connections to HTTPS",
    comment: "The option the user can select for the type of https upgrading"
  )

  /// A page title for the warning page that appears when http was blocked
  public static let siteIsNotSecure = NSLocalizedString(
    "SiteIsNotSecure",
    tableName: "BraveShared",
    bundle: .module,
    value: "Site is not secure",
    comment: "A page title for the warning page that appears when http was blocked"
  )

  /// A page title for the warning page that appears when http was blocked
  public static let theConnectionIsNotSecure = NSLocalizedString(
    "TheConnectionIsNotSecure",
    tableName: "BraveShared",
    bundle: .module,
    value: "The connection to %@ is not secure",
    comment: "A page title for the warning page that appears when http was blocked"
  )

  /// A tab title that appears when a page was blocked
  public static let httpBlockedDescription = NSLocalizedString(
    "YourConnectionIsNotPrivate",
    tableName: "BraveShared",
    bundle: .module,
    value: "You are seeing this warning because this site does not support HTTPS.",
    comment: "A description shown an a page where the http page was blocked"
  )
}
