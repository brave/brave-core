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

// MARK: - Adblock Debugging

extension Strings.Shields {
  public static let blockedRequestsTitle = NSLocalizedString(
    "blockedRequestsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Blocked Requests",
    comment:
      "The title displayed in the navigation bar of Blocked Requests view."
  )
  public static let requestURLLabel = NSLocalizedString(
    "requestURLLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Request URL",
    comment:
      "A label displayed above the request url that was blocked in Blocked Requests view."
  )
  public static let sourceURLLabel = NSLocalizedString(
    "sourceURLLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Source URL",
    comment:
      "A label displayed above the source url of a request that was blocked in Blocked Requests view."
  )
  public static let resourceTypeLabel = NSLocalizedString(
    "resourceTypeLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Resource Type",
    comment:
      "A label displayed above the resource type of a request that was blocked in Blocked Requests view."
  )
  public static let aggressiveLabel = NSLocalizedString(
    "aggressiveLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Aggressive",
    comment:
      "A label displayed above a value indicating if the site is in aggressive mode in Blocked Requests view."
  )
  public static let blockedByLabel = NSLocalizedString(
    "blockedByLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Blocked By",
    comment:
      "A label displayed above the location a request was blocked in Blocked Requests view."
  )
  public static let contentBlocker = NSLocalizedString(
    "contentBlocker",
    tableName: "BraveShared",
    bundle: .module,
    value: "Content Blocker",
    comment:
      "Used to describe when a request was blocked by the Content Blocker in Blocked Requests view."
  )
  public static let requestBlocking = NSLocalizedString(
    "requestBlocking",
    tableName: "BraveShared",
    bundle: .module,
    value: "Request Blocking",
    comment:
      "Used to describe when a request was blocked by our request blocking scripts in Blocked Requests view."
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

// MARK: - Forget Me

extension Strings.Shields {
  /// A toggle option that deletes website content when the site is closed
  public static let forgetMeLabel = NSLocalizedString(
    "ForgetMeLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Forget Me When I Close This Site",
    comment: "A toggle option that deletes website content when the site is closed"
  )

  /// A description of what the Enable GPC toggle does
  public static let forgetMeDescription = NSLocalizedString(
    "ForgetMeDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clears cookies and other site data when you close a site",
    comment: "A description of what the forget me toggle does"
  )
}

// MARK: - Shred

extension Strings.Shields {
  /// A list row label for accessing the shred settings screen
  public static let shredSiteData = NSLocalizedString(
    "ShredSiteData",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shred Site Data",
    comment: "A list row label for accessing the shred settings screen"
  )

  /// A button title that shreds site data immediately
  public static let shredSiteDataNow = NSLocalizedString(
    "ShredSiteDataNow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shred Site Data Now",
    comment: "A button title that shreds site data immediately"
  )

  /// A picker title for selecting a automatic shred setting option
  public static let autoShred = NSLocalizedString(
    "AutoShred",
    tableName: "BraveShared",
    bundle: .module,
    value: "Auto Shred",
    comment: "A picker title for selecting a automatic shred setting option"
  )

  /// An option setting for never automatically shreding site data
  public static let shredNever = NSLocalizedString(
    "ShredNever",
    tableName: "BraveShared",
    bundle: .module,
    value: "Never",
    comment: "An option setting for never automatically shreding site data"
  )

  /// An option setting for shredding when the site is closed
  public static let shredOnSiteTabsClosed = NSLocalizedString(
    "ShredOnSiteTabsClosed",
    tableName: "BraveShared",
    bundle: .module,
    value: "Site Tabs Closed",
    comment: "An option setting for automatically shredding when the site is closed"
  )

  /// An option setting for shredding when the app is closed
  public static let shredOnAppClose = NSLocalizedString(
    "ShredOnAppClose",
    tableName: "BraveShared",
    bundle: .module,
    value: "App Close",
    comment: "An option setting for automatically shredding only when the app is closed"
  )

  /// A title for a confirmation window that appears when a user clicks on 'Shred Data'
  public static let shredSiteDataConfirmationTitle = NSLocalizedString(
    "ShredSiteDataConfirmationTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shred Site Data?",
    comment: "A title for a confirmation window that appears when a user clicks on 'Shred Data'"
  )

  /// A message for a confirmation window that appears when a user clicks on 'Shred Data'.
  public static let shredSiteDataConfirmationMessage = NSLocalizedString(
    "ShredSiteDataConfirmationMessage",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Shredding will close all tabs open to this site, and delete all site data. This cannot be undone.",
    comment: """
      A message for a confirmation window that appears when a user clicks on 'Shred Data'.
      """
  )

  /// A list row label for accessing the shred settings screen
  public static let shredDataButtonTitle = NSLocalizedString(
    "ShredDataButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shred Data",
    comment: "A button title when confirming to shred website data"
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

// MARK: - Filter lists

extension Strings.Shields {
  public static let contentFiltering = NSLocalizedString(
    "ContentFiltering",
    tableName: "BraveShared",
    bundle: .module,
    value: "Content Filtering",
    comment:
      "A title to the content filtering page under global shield settings and the title on the Content filtering page"
  )
  public static let blockMobileAnnoyances = NSLocalizedString(
    "blockMobileAnnoyances",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block 'Switch to App' Notices",
    comment: "A title for setting which blocks 'switch to app' popups"
  )
  public static let contentFilteringDescription = NSLocalizedString(
    "ContentFilteringDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Enable custom filters that block regional and language-specific trackers and Annoyances",
    comment: "A description of the content filtering page."
  )
  public static let defaultFilterLists = NSLocalizedString(
    "DefaultFilterLists",
    tableName: "BraveShared",
    bundle: .module,
    value: "Default Filter Lists",
    comment:
      "A section title that contains default (predefined) filter lists a user can enable/diable."
  )
  public static let filterListsDescription = NSLocalizedString(
    "FilterListsDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Additional popular community lists. Note that enabling too many filters will degrade browsing speeds.",
    comment: "A description on the content filtering screen for the filter lists section."
  )
  public static let addFilterByURL = NSLocalizedString(
    "AddFilterByURL",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add Filter By URL",
    comment: "A title within a cell where a user can navigate to an add screen."
  )
  public static let customFilterList = NSLocalizedString(
    "CustomFilterList",
    tableName: "BraveShared",
    bundle: .module,
    value: "Custom Filter List",
    comment: "Title for the custom filter list add screen found in the navigation bar."
  )
  public static let externalFilterLists = NSLocalizedString(
    "ExternalFilterLists",
    tableName: "BraveShared",
    bundle: .module,
    value: "External Filter Lists",
    comment: "A title for a section that contains all external filter lists"
  )
  public static let customFilterListURL = NSLocalizedString(
    "CustomFilterListsURL",
    tableName: "BraveShared",
    bundle: .module,
    value: "Custom Filter List URL",
    comment: "A section heading above a cell that allows you to enter a filter list URL."
  )
  public static let addCustomFilterListDescription = NSLocalizedString(
    "AddCustomFilterListDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add additional lists created and maintained by your trusted community.",
    comment:
      "A description of a section in a list that allows you to add custom filter lists found in the footer of the add custom url screen"
  )
  public static let addCustomFilterListWarning = NSLocalizedString(
    "AddCustomFilterListWarning",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "**Only subscribe to lists from entities you trust**. Your browser will periodically check for list updates from the URL you enter.",
    comment: "Warning text found in the footer of the add custom filter list url screen."
  )
  public static let filterListsLastUpdated = NSLocalizedString(
    "FilterListsLastUpdatedLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Last updated %@",
    comment:
      "A label that shows when the filter list was last updated. Do not translate the '%@' placeholder. The %@ will be replaced with a relative date. For example, '5 minutes ago' or '1 hour ago'. So the full string will read something like 'Last updated 5 minutes ago'."
  )
  public static let filterListsDownloadPending = NSLocalizedString(
    "FilterListsDownloadPending",
    tableName: "BraveShared",
    bundle: .module,
    value: "Pending download",
    comment:
      "If a filter list is not yet downloaded this label shows up instead of a last download date, signifying that the download is still pending."
  )
  public static let filterListsEnterFilterListURL = NSLocalizedString(
    "FilterListsEnterFilterListURL",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enter filter list URL",
    comment: "This is a placeholder for an input field that takes a custom filter list URL."
  )
  public static let filterListsAdd = NSLocalizedString(
    "FilterListsAdd",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add",
    comment:
      "This is a button on the top navigation that takes the user to an add custom filter list url to the list"
  )
  public static let filterListsEdit = NSLocalizedString(
    "FilterListsEdit",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit",
    comment:
      "This is a button on the top navigation that takes the user to an add custom filter list url to the list"
  )
  public static let filterListURLTextFieldPlaceholder = NSLocalizedString(
    "FilterListURLTextFieldPlaceholder",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enter filter list URL here ",
    comment:
      "This is a placeholder for the custom filter list url text field where a user may enter a custom filter list URL"
  )
  public static let filterListsDownloadFailed = NSLocalizedString(
    "FilterListsDownloadFailed",
    tableName: "BraveShared",
    bundle: .module,
    value: "Download failed",
    comment: "This is a generic error message when downloading a filter list fails."
  )
  public static let filterListAddInvalidURLError = NSLocalizedString(
    "FilterListAddInvalidURLError",
    tableName: "BraveShared",
    bundle: .module,
    value: "The URL entered is invalid",
    comment:
      "This is an error message when a user tries to enter an invalid URL into the custom filter list URL text field."
  )
  public static let filterListAddOnlyHTTPSAllowedError = NSLocalizedString(
    "FilterListAddOnlyHTTPSAllowedError",
    tableName: "BraveShared",
    bundle: .module,
    value: "Only secure (https) URLs are allowed for custom filter lists",
    comment:
      "This is an error message when a user tries to enter a non-https scheme URL into the 'add custom filter list URL' input field"
  )
  public static let updateLists = NSLocalizedString(
    "UpdateLists",
    tableName: "BraveShared",
    bundle: .module,
    value: "Update Lists",
    comment: "This is a label for a button which when pressed updates all the filter lists"
  )
  public static let updatingLists = NSLocalizedString(
    "UpdatingLists",
    tableName: "BraveShared",
    bundle: .module,
    value: "Updating Lists",
    comment:
      "This is a label on a button that updates filter lists which signifies that lista are being updated"
  )
  public static let listsUpdated = NSLocalizedString(
    "ListsUpdated",
    tableName: "BraveShared",
    bundle: .module,
    value: "Lists Updated",
    comment:
      "This is a label on a button that updates filter lists which signifies that lists have been updated"
  )
}

// MARK: - Create custom filters

extension Strings.Shields {
  public static let customFilters = NSLocalizedString(
    "CustomFilters",
    tableName: "BraveShared",
    bundle: .module,
    value: "Custom Filters",
    comment: "A title for a section that allows a user to insert custom filter list text"
  )
  public static let customFiltersDescription = NSLocalizedString(
    "CustomFiltersDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Add custom filters here. Be sure to use the Adblock filter syntax.",
    comment: "A description of the custom filters section"
  )
  /// A placeholder when custom filter lists are empty
  public static let customFiltersPlaceholder = NSLocalizedString(
    "CustomFiltersPlaceholder",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add your custom filters here, one per line.",
    comment: "A placeholder when custom filter lists are empty"
  )
  /// An error message telling the user that they crossed the line limit
  public static let customFiltersTooManyLinesError = NSLocalizedString(
    "CustomFiltersTooManyLinesError",
    tableName: "BraveShared",
    bundle: .module,
    value: "Custom filters do not support more than %i lines",
    comment:
      "An error message telling the user that they crossed the line limit"
  )
  /// An error message telling the user that they crossed the line limit
  public static let customFiltersInvalidRuleError = NSLocalizedString(
    "CustomFiltersInvalidRuleError",
    tableName: "BraveShared",
    bundle: .module,
    value: "Invalid rule `%@` on line %i",
    comment:
      "An error message telling the user that a rule is invalid"
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
