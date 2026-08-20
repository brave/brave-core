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
    bundle: .module,
    value: "Trackers & Ads Blocking",
    comment:
      "A label for a shield option that allows you to switch between different blocking levels for tracker and ads blocking. Options include disabled, standard and aggressive."
  )
  /// A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive.
  public static let trackersAndAdsBlockingDescription = NSLocalizedString(
    "BlockAdsAndTrackingDescription",
    bundle: .module,
    value: "Prevents ads, popups, and trackers from loading.",
    comment:
      "A description for a shield options that allows you to switch between different blocking levels for trackers and ads blocking. Options include disabled, standard and aggressive."
  )
  /// The option the user can select to disable ad and tracker blocking
  public static let trackersAndAdsBlockingDisabled = NSLocalizedString(
    "BlockAdsAndTrackingDisabled",
    bundle: .module,
    value: "Disabled",
    comment: "The option the user can select to disable ad and tracker blocking"
  )
  /// The option the user can select to do aggressive ad and tracker blocking
  public static let trackersAndAdsBlockingAggressive = NSLocalizedString(
    "BlockAdsAndTrackingAggressive",
    bundle: .module,
    value: "Aggressive",
    comment: "The option the user can select to do aggressive ad and tracker blocking"
  )
  /// The option the user can select to do standard (non-aggressive) ad and tracker blocking
  public static let trackersAndAdsBlockingStandard = NSLocalizedString(
    "BlockAdsAndTrackingStandard",
    bundle: .module,
    value: "Standard",
    comment:
      "The option the user can select to do standard (non-aggressive) ad and tracker blocking"
  )
}

// MARK: - Adblock Debugging

extension Strings.Shields {
  public static let blockedRequestsTitle = NSLocalizedString(
    "shields.blockedRequestsTitle",
    bundle: .module,
    value: "Blocked Requests",
    comment:
      "The title displayed in the navigation bar of Blocked Requests view."
  )
  public static let requestURLLabel = NSLocalizedString(
    "shields.requestURLLabel",
    bundle: .module,
    value: "Request URL",
    comment:
      "A label displayed above the request url that was blocked in Blocked Requests view."
  )
  public static let sourceURLLabel = NSLocalizedString(
    "shields.sourceURLLabel",
    bundle: .module,
    value: "Source URL",
    comment:
      "A label displayed above the source url of a request that was blocked in Blocked Requests view."
  )
  public static let resourceTypeLabel = NSLocalizedString(
    "shields.resourceTypeLabel",
    bundle: .module,
    value: "Resource Type",
    comment:
      "A label displayed above the resource type of a request that was blocked in Blocked Requests view."
  )
  public static let aggressiveLabel = NSLocalizedString(
    "shields.aggressiveLabel",
    bundle: .module,
    value: "Aggressive",
    comment:
      "A label displayed above a value indicating if the site is in aggressive mode in Blocked Requests view."
  )
  public static let blockedByLabel = NSLocalizedString(
    "shields.blockedByLabel",
    bundle: .module,
    value: "Blocked By",
    comment:
      "A label displayed above the location a request was blocked in Blocked Requests view."
  )
  public static let contentBlocker = NSLocalizedString(
    "shields.contentBlocker",
    bundle: .module,
    value: "Content Blocker",
    comment:
      "Used to describe when a request was blocked by the Content Blocker in Blocked Requests view."
  )
  public static let requestBlocking = NSLocalizedString(
    "shields.requestBlocking",
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
    bundle: .module,
    value: "Adjust Ad Block Settings For This Site",
    comment:
      "A title for a popup that tells the user we recommend turning shields off for this site."
  )

  /// A descriptive message explaining this site's ad-blocking crackdown
  public static let antiAdBlockWarningDescription = NSLocalizedString(
    "AntiAdBlockWarningDescription",
    bundle: .module,
    value:
      "This site has begun blocking some ad blockers, which means their site may not work as expected.",
    comment: "A descriptive message explaining this site's ad-blocking crackdown."
  )

  /// A descriptive message explaining to disable shields on this site
  public static let antiAdBlockWarningDescription2 = NSLocalizedString(
    "AntiAdBlockWarningDescription2",
    bundle: .module,
    value:
      "To address this issue, Brave can adjust your shields settings for you. Once adjusted, you can try watching this content in Brave Player instead.",
    comment: "A descriptive message explaining to disable shields on this site."
  )

  /// A button that disables ad-blocking and uses brave player
  public static let antiAdBlockWarningConfirmationButton = NSLocalizedString(
    "AntiAdBlockWarningConfirmationButton",
    bundle: .module,
    value: "Adjust Shields For Me",
    comment: "A button that disables ad-blocking and uses brave player."
  )

  /// A button that dismisses the warning and does nothing
  public static let antiAdBlockWarningDismissButton = NSLocalizedString(
    "AntiAdBlockWarningDismissButton",
    bundle: .module,
    value: "Keep Current Settings",
    comment: "A button that dismisses the warning and does nothing."
  )

  /// A discription of the Brave Player
  public static let antiAdBlockWarningBravePlayerDescription = NSLocalizedString(
    "AntiAdBlockWarningBravePlayerDescription",
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
    bundle: .module,
    value: "Brave Player",
    comment: "Title for the brave player feature"
  )

  /// Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar
  public static let bravePlayerInfoTitle = NSLocalizedString(
    "BravePlayerInfoTitle",
    bundle: .module,
    value: "Watch In Brave Player Instead",
    comment:
      "Title for the brave player info popup which appears when clicking on the brave player icon on the navigation bar."
  )

  /// A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time
  public static let bravePlayerInfoMessage = NSLocalizedString(
    "BravePlayerInfoMessage",
    bundle: .module,
    value: "Brave Player lets you watch videos without interruptions.",
    comment:
      "A description of the brave player that is presented on the info panel when clicing on the brave player icon for the first time."
  )

  /// A label for a toggle that enables automatic launching of brave player for certain sites
  public static let bravePlayerAlwaysOpenVideoLinks = NSLocalizedString(
    "BravePlayerAlwaysOpenYouTubeLinks",
    bundle: .module,
    value: "Always open videos from this site with Brave Player",
    comment:
      "A label for a toggle that enables automatic launching of brave player for certain sites"
  )

  /// A button that confirms to use the brave player
  public static let bravePlayerConfirmButton = NSLocalizedString(
    "BravePlayerConfirmButton",
    bundle: .module,
    value: "Try It Out",
    comment: "A button that confirms to use the brave player."
  )

  /// A button that ignores the brave player
  public static let bravePlayerDismissButton = NSLocalizedString(
    "BravePlayerDismissButton",
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
    bundle: .module,
    value: "Enable Global Privacy Control",
    comment: "A label of the GPC toggle"
  )

  /// A description of what the Enable GPC toggle does
  public static let enableGPCDescription = NSLocalizedString(
    "EnableGPCDescription",
    bundle: .module,
    value:
      "Ask websites not to sell or share your data. [Learn more](https://support.brave.app/hc/en-us/articles/360017989132-How-do-I-change-my-Privacy-Settings#h_01HHFRRT6B5YTRREA4ZDARGTWM)",
    comment: "A description of what the Enable GPC toggle does"
  )
}

// MARK: - Shred

extension Strings.Shields {
  /// A list row label for accessing the shred settings screen
  public static let shredSiteData = NSLocalizedString(
    "ShredSiteData",
    bundle: .module,
    value: "Shred Site Data",
    comment: "A list row label for accessing the shred settings screen"
  )

  /// A button title that shreds site data immediately
  public static let shredSiteDataNow = NSLocalizedString(
    "ShredSiteDataNow",
    bundle: .module,
    value: "Shred Site Data Now",
    comment: "A button title that shreds site data immediately"
  )

  /// A picker title for selecting a automatic shred setting option
  public static let autoShred = NSLocalizedString(
    "AutoShred",
    bundle: .module,
    value: "Auto Shred",
    comment: "A picker title for selecting a automatic shred setting option"
  )

  /// An option setting for never automatically shreding site data
  public static let shredNever = NSLocalizedString(
    "ShredNever",
    bundle: .module,
    value: "Never",
    comment: "An option setting for never automatically shreding site data"
  )

  /// The description for never automatically shreding site data
  public static let shredNeverDescription = NSLocalizedString(
    "ShredNeverDescription",
    bundle: .module,
    value: "Browsing data is never shred automatically",
    comment: "The description for never automatically shreding site data"
  )

  /// An option setting for shredding when the site is closed
  public static let shredOnSiteTabsClosed = NSLocalizedString(
    "ShredOnSiteTabsClosed",
    bundle: .module,
    value: "Site Tabs Closed",
    comment: "An option setting for automatically shredding when the site is closed"
  )

  /// The description for shredding when the site is closed
  public static let shredOnSiteTabsClosedDescription = NSLocalizedString(
    "ShredOnSiteTabsClosedDescription",
    bundle: .module,
    value: "Browsing data is automatically shredded when all tabs open to a site are closed",
    comment: "The description for shredding when the site is closed"
  )

  /// An option setting for shredding when the app is closed
  public static let shredOnAppClose = NSLocalizedString(
    "ShredOnAppClose",
    bundle: .module,
    value: "App Close",
    comment: "An option setting for automatically shredding only when the app is closed"
  )

  /// The description for shredding when the app is closed
  public static let shredOnAppCloseDescription = NSLocalizedString(
    "ShredOnAppCloseDescription",
    bundle: .module,
    value: "Browsing data is automatically shredded when the Brave app is closed / restarted",
    comment: "The description for shredding when the app is closed"
  )

  /// The title for the auto shred section in settings
  public static let autoShredSectionTitle = NSLocalizedString(
    "AutoShredSectionTitle",
    bundle: .module,
    value: "Automatically Shred Browsing Data",
    comment: "The description for shredding when the app is closed"
  )

  /// A title for a confirmation window that appears when a user clicks on 'Shred Data'
  public static let shredSiteDataConfirmationTitle = NSLocalizedString(
    "ShredSiteDataConfirmationTitle",
    bundle: .module,
    value: "Shred Site Data?",
    comment: "A title for a confirmation window that appears when a user clicks on 'Shred Data'"
  )

  /// A message for a confirmation window that appears when a user clicks on 'Shred Data'.
  public static let shredSiteDataConfirmationMessage = NSLocalizedString(
    "ShredSiteDataConfirmationMessage",
    bundle: .module,
    value:
      "Shredding will close all tabs open to this site, and delete all site data. This cannot be undone.",
    comment: """
      A message for a confirmation window that appears when a user clicks on 'Shred Data'.
      """
  )

  /// A message for a confirmation window that appears when a user clicks on 'Shred All Tabs'.
  public static let shredSiteAllTabsConfirmationMessage = NSLocalizedString(
    "ShredSiteAllTabsConfirmationMessage",
    bundle: .module,
    value:
      "Shredding will close all tabs, and delete all site data. This cannot be undone.",
    comment: """
      A message for a confirmation window that appears when a user clicks on 'Shred All Tabs'.
      """
  )

  /// A message for a confirmation window that appears when a user clicks on 'Shred' with multiple
  /// tabs selected.
  public static let shredSiteSelectedTabsConfirmationMessage = NSLocalizedString(
    "shredSiteSelectedTabsConfirmationMessage",
    bundle: .module,
    value:
      "Shredding will close the selected tabs, and delete all site data. This cannot be undone.",
    comment: """
      A message for a confirmation window that appears when a user clicks on 'Shred' with multiple tabs selected.
      """
  )

  /// A list row label for accessing the shred settings screen
  public static let shredDataButtonTitle = NSLocalizedString(
    "ShredDataButtonTitle",
    bundle: .module,
    value: "Shred Data",
    comment: "A button title when confirming to shred website data"
  )

  public static let shredRowTitle = NSLocalizedString(
    "shields.shredRowTitle",
    bundle: .module,
    value: "Shred",
    comment: "A row title that appears in the Shields & Privacy settings."
  )
  public static let shredRowDescription = NSLocalizedString(
    "shields.shredRowDescription",
    bundle: .module,
    value: "Update Auto Shred and adjust what data is shred.",
    comment: "A row description that appears in the Shields & Privacy settings."
  )
  public static let shredSettingsViewTitle = NSLocalizedString(
    "shields.shredSettingsViewTitle",
    bundle: .module,
    value: "Shred",
    comment: "A title for the Shred settings screen, within Shields & Privacy settings."
  )
  public static let shredHistoryRowTitle = NSLocalizedString(
    "shields.shredHistoryRowTitle",
    bundle: .module,
    value: "Shred Removes History",
    comment:
      "A title beside a toggle that appears in the Shred settings screen, within Shields & Privacy settings."
  )
  public static let shredHistoryRowDescription = NSLocalizedString(
    "shields.shredHistoryRowDescription",
    bundle: .module,
    value: "If Shred should remove history items.",
    comment:
      "A description beside a toggle that appears in the Shred settings screen, within Shields & Privacy settings."
  )
}

// MARK: - Blocked Page

extension Strings.Shields {
  /// A tab title that appears when a page was blocked
  public static let domainBlockedTitle = NSLocalizedString(
    "DomainBlockedTitle",
    bundle: .module,
    value: "Domain Blocked",
    comment: "A tab title for the warning page that appears when a page was blocked"
  )

  /// A title in the warning page that appears when a page was blocked
  public static let domainBlockedPageTitle = NSLocalizedString(
    "DomainBlockedPageTitle",
    bundle: .module,
    value: "This Site May Attempt to Track You Across Other Sites",
    comment: "A title in the warning page that appears when a page was blocked"
  )

  /// A title in the warning page that appears when a page was blocked
  public static let domainBlockedPageMessage = NSLocalizedString(
    "DomainBlockedPageMessage",
    bundle: .module,
    value: "Brave has prevented the following site from loading:",
    comment: "A message in the warning page that appears when a page was blocked"
  )

  /// A description in the warning page that appears when a page was blocked
  public static let domainBlockedPageDescription = NSLocalizedString(
    "DomainBlockedPageDescription",
    bundle: .module,
    value:
      "Because you requested to aggressively block trackers and ads, Brave is blocking this site before the first network connection.",
    comment: "A description in the warning page that appears when a page was blocked"
  )

  /// Text for a button in a blocked page info screen that allows you to proceed regardless of the privacy warning
  public static let domainBlockedProceedAction = NSLocalizedString(
    "DomainBlockedProceedAction",
    bundle: .module,
    value: "Proceed",
    comment:
      "Text for a button in a blocked page info screen that allows you to proceed regardless of the privacy warning"
  )

  /// A description in the warning page that appears when a page was blocked
  public static let domainBlockedGoBackAction = NSLocalizedString(
    "DomainBlockedGoBackAction",
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
    bundle: .module,
    value: "Content Filtering",
    comment:
      "A title to the content filtering page under global shield settings and the title on the Content filtering page"
  )
  public static let blockMobileAnnoyances = NSLocalizedString(
    "blockMobileAnnoyances",
    bundle: .module,
    value: "Block 'Switch to App' Notices",
    comment: "A title for setting which blocks 'switch to app' popups"
  )
  public static let contentFilteringDescription = NSLocalizedString(
    "ContentFilteringDescription",
    bundle: .module,
    value:
      "Enable custom filters that block regional and language-specific trackers and Annoyances",
    comment: "A description of the content filtering page."
  )
  public static let defaultFilterLists = NSLocalizedString(
    "DefaultFilterLists",
    bundle: .module,
    value: "Default Filter Lists",
    comment:
      "A section title that contains default (predefined) filter lists a user can enable/diable."
  )
  public static let filterListsDescription = NSLocalizedString(
    "FilterListsDescription",
    bundle: .module,
    value:
      "Additional popular community lists. Note that enabling too many filters will degrade browsing speeds.",
    comment: "A description on the content filtering screen for the filter lists section."
  )
  public static let addFilterByURL = NSLocalizedString(
    "AddFilterByURL",
    bundle: .module,
    value: "Add Filter By URL",
    comment: "A title within a cell where a user can navigate to an add screen."
  )
  public static let customFilterList = NSLocalizedString(
    "CustomFilterList",
    bundle: .module,
    value: "Custom Filter List",
    comment: "Title for the custom filter list add screen found in the navigation bar."
  )
  public static let externalFilterLists = NSLocalizedString(
    "ExternalFilterLists",
    bundle: .module,
    value: "External Filter Lists",
    comment: "A title for a section that contains all external filter lists"
  )
  public static let customFilterListURL = NSLocalizedString(
    "CustomFilterListsURL",
    bundle: .module,
    value: "Custom Filter List URL",
    comment: "A section heading above a cell that allows you to enter a filter list URL."
  )
  public static let addCustomFilterListDescription = NSLocalizedString(
    "AddCustomFilterListDescription",
    bundle: .module,
    value: "Add additional lists created and maintained by your trusted community.",
    comment:
      "A description of a section in a list that allows you to add custom filter lists found in the footer of the add custom url screen"
  )
  public static let addCustomFilterListWarning = NSLocalizedString(
    "AddCustomFilterListWarning",
    bundle: .module,
    value:
      "**Only subscribe to lists from entities you trust**. Your browser will periodically check for list updates from the URL you enter.",
    comment: "Warning text found in the footer of the add custom filter list url screen."
  )
  public static let filterListsLastUpdated = NSLocalizedString(
    "FilterListsLastUpdatedLabel",
    bundle: .module,
    value: "Last updated %@",
    comment:
      "A label that shows when the filter list was last updated. Do not translate the '%@' placeholder. The %@ will be replaced with a relative date. For example, '5 minutes ago' or '1 hour ago'. So the full string will read something like 'Last updated 5 minutes ago'."
  )
  public static let filterListsDownloadPending = NSLocalizedString(
    "FilterListsDownloadPending",
    bundle: .module,
    value: "Pending download",
    comment:
      "If a filter list is not yet downloaded this label shows up instead of a last download date, signifying that the download is still pending."
  )
  public static let filterListsEnterFilterListURL = NSLocalizedString(
    "FilterListsEnterFilterListURL",
    bundle: .module,
    value: "Enter filter list URL",
    comment: "This is a placeholder for an input field that takes a custom filter list URL."
  )
  public static let filterListsAdd = NSLocalizedString(
    "FilterListsAdd",
    bundle: .module,
    value: "Add",
    comment:
      "This is a button on the top navigation that takes the user to an add custom filter list url to the list"
  )
  public static let filterListsEdit = NSLocalizedString(
    "FilterListsEdit",
    bundle: .module,
    value: "Edit",
    comment:
      "This is a button on the top navigation that takes the user to an add custom filter list url to the list"
  )
  public static let filterListURLTextFieldPlaceholder = NSLocalizedString(
    "FilterListURLTextFieldPlaceholder",
    bundle: .module,
    value: "Enter filter list URL here ",
    comment:
      "This is a placeholder for the custom filter list url text field where a user may enter a custom filter list URL"
  )
  public static let filterListsDownloadFailed = NSLocalizedString(
    "FilterListsDownloadFailed",
    bundle: .module,
    value: "Download failed",
    comment: "This is a generic error message when downloading a filter list fails."
  )
  public static let filterListAddInvalidURLError = NSLocalizedString(
    "FilterListAddInvalidURLError",
    bundle: .module,
    value: "The URL entered is invalid",
    comment:
      "This is an error message when a user tries to enter an invalid URL into the custom filter list URL text field."
  )
  public static let filterListAddOnlyHTTPSAllowedError = NSLocalizedString(
    "FilterListAddOnlyHTTPSAllowedError",
    bundle: .module,
    value: "Only secure (https) URLs are allowed for custom filter lists",
    comment:
      "This is an error message when a user tries to enter a non-https scheme URL into the 'add custom filter list URL' input field"
  )
  public static let updateLists = NSLocalizedString(
    "UpdateLists",
    bundle: .module,
    value: "Update Lists",
    comment: "This is a label for a button which when pressed updates all the filter lists"
  )
  public static let updatingLists = NSLocalizedString(
    "UpdatingLists",
    bundle: .module,
    value: "Updating Lists",
    comment:
      "This is a label on a button that updates filter lists which signifies that lista are being updated"
  )
  public static let listsUpdated = NSLocalizedString(
    "ListsUpdated",
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
    bundle: .module,
    value: "Custom Filters",
    comment: "A title for a section that allows a user to insert custom filter list text"
  )
  public static let customFiltersDescription = NSLocalizedString(
    "CustomFiltersDescription",
    bundle: .module,
    value:
      "Add custom filters here. Be sure to use the Adblock filter syntax.",
    comment: "A description of the custom filters section"
  )
  /// A placeholder when custom filter lists are empty
  public static let customFiltersPlaceholder = NSLocalizedString(
    "CustomFiltersPlaceholder",
    bundle: .module,
    value: "Add Custom Filters",
    comment: "A placeholder when custom filter lists are empty"
  )
  public static let editCustomFiltersLabel = NSLocalizedString(
    "EditCustomFiltersLabel",
    bundle: .module,
    value: "Edit Custom Filters",
    comment: "A placeholder when custom filter lists are empty"
  )
  /// An error message telling the user that they crossed the line limit
  public static let customFiltersTooManyLinesError = NSLocalizedString(
    "CustomFiltersTooManyLinesError",
    bundle: .module,
    value: "Custom filters do not support more than %i lines",
    comment:
      "An error message telling the user that they crossed the line limit"
  )
  /// An error message telling the user that they crossed the line limit
  public static let customFiltersInvalidRuleError = NSLocalizedString(
    "CustomFiltersInvalidRuleError",
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
    bundle: .module,
    value: "Strict",
    comment: "The option the user can select to do strict https upgrading"
  )
  /// The option the user can select for the type of https upgrading
  public static let upgradeConnectionsToHTTPS = NSLocalizedString(
    "UpgradeConnectionsToHTTPS",
    bundle: .module,
    value: "Upgrade Connections to HTTPS",
    comment: "The option the user can select for the type of https upgrading"
  )

  /// A page title for the warning page that appears when http was blocked
  public static let siteIsNotSecure = NSLocalizedString(
    "SiteIsNotSecure",
    bundle: .module,
    value: "Site is not secure",
    comment: "A page title for the warning page that appears when http was blocked"
  )

  /// A page title for the warning page that appears when http was blocked
  public static let theConnectionIsNotSecure = NSLocalizedString(
    "TheConnectionIsNotSecure",
    bundle: .module,
    value: "The connection to %@ is not secure",
    comment: "A page title for the warning page that appears when http was blocked"
  )

  /// A tab title that appears when a page was blocked
  public static let httpBlockedDescription = NSLocalizedString(
    "YourConnectionIsNotPrivate",
    bundle: .module,
    value: "You are seeing this warning because this site does not support HTTPS.",
    comment: "A description shown an a page where the http page was blocked"
  )
  public static let httpsEverywhereDescription = NSLocalizedString(
    "httpsEverywhereDescription",
    bundle: .module,
    value: "Opens sites using secure HTTPS instead of HTTP when possible.",
    comment: ""
  )
}

// MARK: Shields Settings
extension Strings.Shields {
  public static let shieldsDefaults = NSLocalizedString(
    "ShieldsDefaults",
    bundle: .module,
    value: "Brave Shields Global Defaults",
    comment: "Section title for adbblock, tracking protection, HTTPS-E, and cookies"
  )
  public static let shieldsDefaultsFooter = NSLocalizedString(
    "ShieldsDefaultsFooter",
    bundle: .module,
    value:
      "These are the default Shields settings for new sites. Changing these won't affect your existing per-site settings.",
    comment: "Section footer for global shields defaults"
  )
  public static let blockScripts = NSLocalizedString(
    "BlockScripts",
    bundle: .module,
    value: "Block Scripts",
    comment: ""
  )
  public static let blockScriptsDescription = NSLocalizedString(
    "BlockScriptsDescription",
    bundle: .module,
    value: "Blocks JavaScript (may break sites).",
    comment: ""
  )
  public static let fingerprintingProtection = NSLocalizedString(
    "FingerprintingProtection",
    bundle: .module,
    value: "Block Fingerprinting",
    comment: ""
  )
  public static let fingerprintingProtectionDescription = NSLocalizedString(
    "FingerprintingProtectionDescription",
    bundle: .module,
    value: "Makes it harder for sites to recognize your device's distinctive characteristics. ",
    comment: ""
  )
  public static let autoRedirectAMPPages = NSLocalizedString(
    "AutoRedirectAMPPages",
    bundle: .module,
    value: "Auto-Redirect AMP Pages",
    comment:
      "This is a title for a setting toggle that enables/disables auto-redirect of Google's AMP (Accelerated Mobile Page) pages to the original (non-AMP) pages. The text 'AMP' is not to be translated."
  )
  public static let autoRedirectAMPPagesDescription = NSLocalizedString(
    "AutoRedirectAMPPagesDescription",
    bundle: .module,
    value:
      "Always visit original (non-AMP) page URLs, instead of Google's Accelerated Mobile Page versions",
    comment:
      "This is a description for a setting toggle that enables/disables auto-redirect of Google's AMP (Accelerated Mobile Page) pages to the original (non-AMP) pages. The text 'AMP' and 'Accelerated Mobile Page' is not to be translated."
  )
  public static let autoRedirectTrackingURLs = NSLocalizedString(
    "AutoRedirectTrackingURLs",
    bundle: .module,
    value: "Auto-Redirect Tracking URLs",
    comment:
      "This is a title for a setting toggle that enables/disables auto-redirection of tracking pages (Debouncing). Debouncing skips certain intermediate tracker pages and goes directly to the target without the intermediate tracker page."
  )
  public static let autoRedirectTrackingURLsDescription = NSLocalizedString(
    "AutoRedirectTrackingURLsDescription",
    bundle: .module,
    value: "Enable support for bypassing top-level redirect tracking URLs",
    comment:
      "This is a description for a setting toggle that enables/disables auto-redirect of tracking URLs (i.e. Debouncing)."
  )
}

extension Strings.Shields {
  public static let toggleHint = NSLocalizedString(
    "BraveShieldsToggleHint",
    bundle: .module,
    value: "Double-tap to toggle Brave Shields",
    comment: "The accessibility hint spoken when focused on the main shields toggle"
  )
  public static let statusTitle = NSLocalizedString(
    "BraveShieldsStatusTitle",
    bundle: .module,
    value: "Brave Shields",
    comment: "Context: 'Brave Shields Up' or 'Brave Shields Down'"
  )
  public static let statusValueUp = NSLocalizedString(
    "BraveShieldsStatusValueUp",
    bundle: .module,
    value: "Up",
    comment: "Context: The 'Up' in 'Brave Shields Up'"
  )
  public static let statusValueDown = NSLocalizedString(
    "BraveShieldsStatusValueDown",
    bundle: .module,
    value: "Down",
    comment: "Context: The 'Down' in 'Brave Shields Down'"
  )
  public static let blockedCountLabel = NSLocalizedString(
    "BraveShieldsBlockedCountLabel",
    bundle: .module,
    value: "Ads and other creepy things blocked",
    comment: "The number of ads and trackers blocked will be next to this"
  )
  public static let blockedInfoButtonAccessibilityLabel = NSLocalizedString(
    "BraveShieldsBlockedInfoButtonAccessibilityLabel",
    bundle: .module,
    value: "Learn more",
    comment:
      "What the screen reader will read out when the user has VoiceOver on and taps on the question-mark info button on the shields panel"
  )
  public static let siteBroken = NSLocalizedString(
    "BraveShieldsSiteBroken",
    bundle: .module,
    value: "If this site appears broken, try Shields down",
    comment: ""
  )
  public static let advancedControls = NSLocalizedString(
    "BraveShieldsAdvancedControls",
    bundle: .module,
    value: "Advanced controls",
    comment: ""
  )
  public static let aboutBraveShieldsTitle = NSLocalizedString(
    "AboutBraveShields",
    bundle: .module,
    value: "About Brave Shields",
    comment: "The title of the screen explaining Brave Shields"
  )
  public static let aboutBraveShieldsBody = NSLocalizedString(
    "AboutBraveShieldsBody",
    bundle: .module,
    value:
      "Sites often include cookies and scripts which try to identify you and your device. They want to work out who you are and follow you across the web — tracking what you do on every site.\n\nBrave blocks these things so that you can browse without being followed around.",
    comment: "The body of the screen explaining Brave Shields"
  )
  public static let shieldsDownDisclaimer = NSLocalizedString(
    "ShieldsDownDisclaimer",
    bundle: .module,
    value:
      "You're browsing this site without Brave's privacy protections. Does it not work right with Shields up?",
    comment: ""
  )
  public static let globalControls = NSLocalizedString(
    "BraveShieldsGlobalControls",
    bundle: .module,
    value: "Global Controls",
    comment: ""
  )
  public static let globalChangeButton = NSLocalizedString(
    "BraveShieldsGlobalChangeButton",
    bundle: .module,
    value: "Change Shields Global Defaults",
    comment: ""
  )
  public static let siteReportedTitle = NSLocalizedString(
    "SiteReportedTitle",
    bundle: .module,
    value: "Thank You",
    comment: ""
  )
  public static let siteReportedBody = NSLocalizedString(
    "SiteReportedBody",
    bundle: .module,
    value:
      "Thanks for letting Brave's developers know that there's something wrong with this site. We'll do our best to fix it!",
    comment: ""
  )
  public static let braveShieldsSaveContactInfo = NSLocalizedString(
    "BraveShieldsSaveContactInfo",
    bundle: .module,
    value: "Store contact information for future broken site reports",
    comment:
      "Shields panel toggle label that would save webcompat report contact info data when enabled."
  )
  public static let braveShieldsSaveContactInfoDescription = NSLocalizedString(
    "BraveShieldsSaveContactInfoDescription",
    bundle: .module,
    value: "If you provide contact info it will be stored for future reports",
    comment:
      "Description for shields panel toggle label that would save webcompat report contact info data when enabled."
  )

  // MARK: Submit report
  public static let reportABrokenSite = NSLocalizedString(
    "ReportABrokenSite",
    bundle: .module,
    value: "Report a Broken Site",
    comment: ""
  )
  public static let reportBrokenSiteBody1 = NSLocalizedString(
    "ReportBrokenSiteBody1",
    bundle: .module,
    value: "Let Brave's developers know that this site isn't working as expected:",
    comment:
      "First part of the report a broken site copy. After the colon is a new line and then a website address"
  )
  public static let reportBrokenSiteBody2 = NSLocalizedString(
    "ReportBrokenSiteBody2",
    bundle: .module,
    value:
      "Note: The report sent to Brave servers will include the site address, Brave version number, Shields settings, VPN status, and language settings.",
    comment:
      "This is the info text that is presented when a user is submitting a web-compatibility report."
  )
  public static let reportBrokenSubmitButtonTitle = NSLocalizedString(
    "ReportBrokenSubmitButtonTitle",
    bundle: .module,
    value: "Submit",
    comment: ""
  )

  /// A label for a text entry field where the user can provide additional details for a web-compatibility report
  public static let reportBrokenAdditionalDetails = NSLocalizedString(
    "ReportBrokenAdditionalDetails",
    bundle: .module,
    value: "Additional details (optional)",
    comment:
      "A label for a text entry field where the user can provide additional details for a web-compatibility report"
  )

  /// A label for a text entry field where the user can provide additional details for a web-compatibility report
  public static let reportBrokenAdditionalDetailsRequired = NSLocalizedString(
    "reportBrokenAdditionalDetailsRequired",
    bundle: .module,
    value: "Additional details",
    comment:
      "A label for a text entry field where the user can provide additional details for a web-compatibility report"
  )

  /// A label for drop down where the user can provide a category for a web-compatibility report
  public static let reportBrokenCategory = NSLocalizedString(
    "reportBrokenCategory",
    bundle: .module,
    value: "What's the main issue you're seeing?",
    comment:
      "A label for a drop down where the user provides a category for a web-compatibility report"
  )

  /// A placeholder for the drop down where the user can provide a category for a web-compatibility report
  public static let reportBrokenPlaceholder = NSLocalizedString(
    "ReportBrokenPlaceholder",
    bundle: .module,
    value: "Select one option",
    comment:
      "A placeholder for a drop down where the user provides a category for a web-compatibility report"
  )

  public static let reportBrokenContactMe = NSLocalizedString(
    "ReportBrokenContactMe",
    bundle: .module,
    value: "Contact me at: (optional)",
    comment:
      "A label for a text entry field where the user can provide contact details within a web-compatibilty report"
  )

  public static let reportBrokenContactMeSuggestions = NSLocalizedString(
    "ReportBrokenContactMeSuggestions",
    bundle: .module,
    value: "Email, Twitter, etc.",
    comment:
      "A placeholder for a text entry field within a web-compatibilty report which shows a few suggestions of what the user should enter for their contact contact details (in a 'Contact me at: (optional)' field)."
  )

  public static let reportBrokenContactMeDescription = NSLocalizedString(
    "ReportBrokenContactMeDescription",
    bundle: .module,
    value:
      "If you provide contact info it will be stored for reporting broken sites in the future.",
    comment: "Contact info storing description"
  )
}
