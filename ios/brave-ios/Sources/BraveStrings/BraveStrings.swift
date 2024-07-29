// swift-format-ignore-file

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

// MARK: - Common Strings
extension Strings {
  public static let cancelButtonTitle = NSLocalizedString(
    "CancelButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cancel",
    comment: ""
  )
  /// A confirmation title that appears in an alert to dismiss changes in a form or text input
  public static let dismissChangesConfirmationTitle = NSLocalizedString(
    "DismissChangesConfirmationTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Dismiss Changes?",
    comment:
      "A confirmation title that appears in an alert to dismiss changes in a form or text input"
  )
  /// A confirmation message that appears in an alert to dismiss changes in a form or text input
  public static let dismissChangesConfirmationMessage = NSLocalizedString(
    "DismissChangesConfirmationMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Are you sure you want to dismiss these changes? Any modifications will be lost.",
    comment:
      "A confirmation message that appears in an alert to dismiss changes in a form or text input"
  )
  /// A confirmation button title for a alert option to dismiss changes in a form or text input
  public static let dismissChangesButtonTitle = NSLocalizedString(
    "DismissChangesButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Dismiss Changes",
    comment:
      "A confirmation button title for a alert option to dismiss changes in a form or text input"
  )
  public static let unlockButtonTitle = NSLocalizedString(
    "UnlockButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Unlock",
    comment: ""
  )
  public static let webContentAccessibilityLabel = NSLocalizedString(
    "WebContentAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Web content",
    comment: "Accessibility label for the main web content view"
  )
  public static let shareLinkActionTitle = NSLocalizedString(
    "ShareLinkActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Share Link",
    comment: "Context menu item for sharing a link URL"
  )
  public static let showTabs = NSLocalizedString(
    "ShowTabs",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Tabs",
    comment: "Accessibility Label for the tabs button in the browser toolbar"
  )
  public static let copyLinkActionTitle = NSLocalizedString(
    "CopyLinkActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy Link",
    comment: "Context menu item for copying a link URL to the clipboard"
  )
  public static let openNewPrivateTabButtonTitle = NSLocalizedString(
    "OpenNewPrivateTabButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open in New Private Tab",
    comment: "Context menu option for opening a link in a new private tab"
  )
  public static let deleteLoginButtonTitle = NSLocalizedString(
    "DeleteLoginButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Delete",
    comment: "Label for the button used to delete the current login."
  )
  public static let saveButtonTitle = NSLocalizedString(
    "SaveButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Save",
    comment: "Label for the button used to save data"
  )
  public static let saveChangesButtonTitle = NSLocalizedString(
    "SaveChangesButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Save changes",
    comment: "Label for the button used to save changes in some text input."
  )
  public static let share = NSLocalizedString(
    "CommonShare",
    tableName: "BraveShared",
    bundle: .module,
    value: "Share",
    comment: "Text to select sharing something (example: image, video, URL)"
  )
  public static let download = NSLocalizedString(
    "CommonDownload",
    tableName: "BraveShared",
    bundle: .module,
    value: "Download",
    comment: "Text to choose for downloading a file (example: saving an image to phone)"
  )
  public static let showLinkPreviewsActionTitle = NSLocalizedString(
    "ShowLinkPreviewsActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Link Previews",
    comment: "Context menu item for showing link previews"
  )
  public static let hideLinkPreviewsActionTitle = NSLocalizedString(
    "HideLinkPreviewsActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hide Link Previews",
    comment: "Context menu item for hiding link previews"
  )
  public static let learnMore = NSLocalizedString(
    "learnMore",
    tableName: "BraveShared",
    bundle: .module,
    value: "Learn More",
    comment: ""
  )
  public static let termsOfService = NSLocalizedString(
    "TermsOfService",
    tableName: "BraveShared",
    bundle: .module,
    value: "Terms of Service",
    comment: ""
  )
  public static let title = NSLocalizedString(
    "Title",
    tableName: "BraveShared",
    bundle: .module,
    value: "Title",
    comment: ""
  )
  public static let monthAbbreviation =
    NSLocalizedString(
      "monthAbbreviation",
      tableName: "BraveShared",
      bundle: .module,
      value: "mo.",
      comment:
        "Abbreviation for 'Month', use full word' Month' if this word can't be shortened in your language"
    )
  public static let yearAbbreviation =
    NSLocalizedString(
      "yearAbbreviation",
      tableName: "BraveShared",
      bundle: .module,
      value: "yr.",
      comment:
        "Abbreviation for 'Year', use full word' Yeara' if this word can't be shortened in your language"
    )
  public static let sendButtonTitle = NSLocalizedString(
    "sendButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Send",
    comment: ""
  )
}

// MARK:-  UIAlertControllerExtensions.swift
extension Strings {
  public static let sendCrashReportAlertTitle = NSLocalizedString(
    "SendCrashReportAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Oops! Brave crashed",
    comment: "Title for prompt displayed to user after the app crashes"
  )
  public static let sendCrashReportAlertMessage = NSLocalizedString(
    "SendCrashReportAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Send a crash report so Brave can fix the problem?",
    comment:
      "Message displayed in the crash dialog above the buttons used to select when sending reports"
  )
  public static let sendReportButtonTitle = NSLocalizedString(
    "SendReportButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Send Report",
    comment: "Used as a button label for crash dialog prompt"
  )
  public static let alwaysSendButtonTitle = NSLocalizedString(
    "AlwaysSendButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Always Send",
    comment: "Used as a button label for crash dialog prompt"
  )
  public static let dontSendButtonTitle = NSLocalizedString(
    "DontSendButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Don’t Send",
    comment: "Used as a button label for crash dialog prompt"
  )
  public static let restoreTabOnCrashAlertTitle = NSLocalizedString(
    "RestoreTabOnCrashAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Closed Unexpectedly.",
    comment: "Restore Tabs Prompt Title"
  )
  public static let restoreTabOnCrashAlertMessage = NSLocalizedString(
    "RestoreTabOnCrashAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Unfortunately, Brave crashed or did not close properly.\nRestore your tabs?",
    comment: "Restore Tabs Prompt Description"
  )
  public static let restoreTabNegativeButtonTitle = NSLocalizedString(
    "RestoreTabNegativeButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "No",
    comment: "Restore Tabs Negative Action"
  )
  public static let restoreTabAffirmativeButtonTitle = NSLocalizedString(
    "RestoreTabAffirmativeButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Yes",
    comment: "Restore Tabs Affirmative Action"
  )
  public static let clearPrivateDataAlertCancelButtonTitle = NSLocalizedString(
    "ClearPrivateDataAlertCancelButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cancel",
    comment: "The cancel button when confirming clear private data."
  )
  public static let clearPrivateDataAlertOkButtonTitle = NSLocalizedString(
    "ClearPrivateDataAlertOkButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "OK",
    comment: "The button that clears private data."
  )
  public static let clearSyncedHistoryAlertMessage = NSLocalizedString(
    "ClearSyncedHistoryAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "This action will clear all of your private data, including history from your synced devices.",
    comment:
      "Description of the confirmation dialog shown when a user tries to clear history that's synced to another device."
  )
  public static let clearSyncedHistoryAlertCancelButtoTitle = NSLocalizedString(
    "ClearSyncedHistoryAlertCancelButtoTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cancel",
    comment: "The cancel button when confirming clear history."
  )
  public static let clearSyncedHistoryAlertOkButtoTitle = NSLocalizedString(
    "ClearSyncedHistoryAlertOkButtoTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "OK",
    comment: "The confirmation button that clears history even when Sync is connected."
  )
  public static let deleteLoginAlertTitle = NSLocalizedString(
    "DeleteLoginAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Are you sure?",
    comment: "Prompt title when deleting logins"
  )
  public static let deleteLoginAlertLocalMessage = NSLocalizedString(
    "DeleteLoginAlertLocalMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Logins will be permanently removed.",
    comment:
      "Prompt message warning the user that deleting non-synced logins will permanently remove them"
  )
  public static let deleteLoginAlertSyncedDevicesMessage = NSLocalizedString(
    "DeleteLoginAlertSyncedDevicesMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Logins will be removed from all connected devices.",
    comment:
      "Prompt message warning the user that deleted logins will remove logins from all connected devices"
  )
  public static let deleteLoginAlertCancelActionTitle = NSLocalizedString(
    "DeleteLoginAlertCancelActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cancel",
    comment: "Prompt option for cancelling out of deletion"
  )
  public static let genericErrorTitle = NSLocalizedString(
    "genericErrorTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Error",
    comment: ""
  )
  public static let genericErrorBody = NSLocalizedString(
    "genericErrorBody",
    tableName: "BraveShared",
    bundle: .module,
    value: "Oops! Something went wrong. Please try again.",
    comment: ""
  )
}

// MARK:-  SearchViewController.swift
extension Strings {
  public static let searchSettingsButtonTitle = NSLocalizedString(
    "SearchSettingsButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Settings",
    comment: "Label for search settings button."
  )
  public static let searchEngineFormatText = NSLocalizedString(
    "SearchEngineFormatText",
    tableName: "BraveShared",
    bundle: .module,
    value: "%@ search",
    comment:
      "Label for search engine buttons. The argument corresponds to the name of the search engine."
  )
  public static let searchSuggestionFromFormatText = NSLocalizedString(
    "SearchSuggestionFromFormatText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search suggestions from %@",
    comment:
      "Accessibility label for image of default search engine displayed left to the actual search suggestions from the engine. The parameter substituted for \"%@\" is the name of the search engine. E.g.: Search suggestions from Google"
  )
  public static let searchesForSuggestionButtonAccessibilityText = NSLocalizedString(
    "SearchesForSuggestionButtonAccessibilityText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Searches for the suggestion",
    comment:
      "Accessibility hint describing the action performed when a search suggestion is clicked"
  )
  public static let searchSuggestionSectionTitleFormat = NSLocalizedString(
    "SearchSuggestionSectionTitleFormat",
    tableName: "BraveShared",
    bundle: .module,
    value: "%@ Search",
    comment:
      "Section Title when showing search suggestions. The parameter substituted for \"%@\" is the name of the search engine. E.g.: Google Search"
  )
  public static let turnOnSearchSuggestions = NSLocalizedString(
    "Turn on search suggestions?",
    tableName: "BraveShared",
    bundle: .module,
    comment: "Prompt shown before enabling provider search queries"
  )
  public static let searchSuggestionSectionTitleNoSearchFormat = NSLocalizedString(
    "SearchSuggestionSectionTitleNoSearchFormat",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search",
    comment:
      "Section Title when showing search suggestions and the engine does not contain the word 'Search'."
  )
  public static let noSearchResultsfound = NSLocalizedString(
    "noSearchResultsfound",
    tableName: "BraveShared",
    bundle: .module,
    value: "No search results found.",
    comment: "The information title displayed when there is no search reault found"
  )
  public static let searchBookmarksTitle = NSLocalizedString(
    "searchBookmarksTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Bookmarks",
    comment: "The placeholder text for bookmark search"
  )
}

// MARK:-  Authenticator.swift
extension Strings {
  public static let authPromptAlertCancelButtonTitle = NSLocalizedString(
    "AuthPromptAlertCancelButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cancel",
    comment: "Label for Cancel button"
  )
  public static let authPromptAlertLogInButtonTitle = NSLocalizedString(
    "AuthPromptAlertLogInButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Log in",
    comment: "Authentication prompt log in button"
  )
  public static let authPromptAlertTitle = NSLocalizedString(
    "AuthPromptAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Authentication required",
    comment: "Authentication prompt title"
  )
  public static let authPromptAlertFormatRealmMessageText = NSLocalizedString(
    "AuthPromptAlertFormatRealmMessageText",
    tableName: "BraveShared",
    bundle: .module,
    value: "A username and password are being requested by %@. The site says: %@",
    comment:
      "Authentication prompt message with a realm. First parameter is the hostname. Second is the realm string"
  )
  public static let authPromptAlertMessageText = NSLocalizedString(
    "AuthPromptAlertMessageText",
    tableName: "BraveShared",
    bundle: .module,
    value: "A username and password are being requested by %@.",
    comment: "Authentication prompt message with no realm. Parameter is the hostname of the site"
  )
  public static let authPromptAlertUsernamePlaceholderText = NSLocalizedString(
    "AuthPromptAlertUsernamePlaceholderText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Username",
    comment: "Username textbox in Authentication prompt"
  )
  public static let authPromptAlertPasswordPlaceholderText = NSLocalizedString(
    "AuthPromptAlertPasswordPlaceholderText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Password",
    comment: "Password textbox in Authentication prompt"
  )
  public static let authenticationLoginsTouchReason = NSLocalizedString(
    "AuthenticationLoginsTouchReason",
    tableName: "BraveShared",
    bundle: .module,
    value: "This authenticates your access to Brave",
    comment:
      "Touch ID or PIN entry prompt subtitle when accessing Brave with the Browser Lock feature enabled"
  )
}

// MARK:-  BrowserViewController.swift
extension Strings {
  public static let openNewTabButtonTitle = NSLocalizedString(
    "OpenNewTabButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open in New Tab",
    comment: "Context menu item for opening a link in a new tab"
  )

  public static let openImageInNewTabActionTitle = NSLocalizedString(
    "OpenImageInNewTab",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Image In New Tab",
    comment: "Context menu for opening image in new tab"
  )
  public static let saveImageActionTitle = NSLocalizedString(
    "SaveImageActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Save Image",
    comment: "Context menu item for saving an image"
  )
  public static let accessPhotoDeniedAlertTitle = NSLocalizedString(
    "AccessPhotoDeniedAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave would like to access your Photos",
    comment: "See http://mzl.la/1G7uHo7"
  )
  public static let accessPhotoDeniedAlertMessage = NSLocalizedString(
    "AccessPhotoDeniedAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "This allows you to save the image to your Camera Roll.",
    comment: "See http://mzl.la/1G7uHo7"
  )
  public static let openPhoneSettingsActionTitle = NSLocalizedString(
    "OpenPhoneSettingsActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Settings",
    comment: "See http://mzl.la/1G7uHo7"
  )
  public static let copyImageActionTitle = NSLocalizedString(
    "CopyImageActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy Image",
    comment: "Context menu item for copying an image to the clipboard"
  )
  public static let closeAllTabsTitle = NSLocalizedString(
    "CloseAllTabsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Close All %i Tabs",
    comment: ""
  )
  public static let closeAllOtherTabsTitle = NSLocalizedString(
    "closeAllOtherTabsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Close All Other Tabs",
    comment: "Title for asking user to close all tabs except the active tab."
  )
  public static let closeAllTabsPrompt =
    NSLocalizedString(
      "closeAllTabsPrompt",
      tableName: "BraveShared",
      bundle: .module,
      value: "Are you sure you want to close all open tabs?",
      comment: "We ask users this prompt before attempting to close multiple tabs via context menu"
    )
  public static let closeAllOtherTabsPrompt =
    NSLocalizedString(
      "closeAllOtherTabsPrompt",
      tableName: "BraveShared",
      bundle: .module,
      value: "Are you sure you want to close all other open tabs?",
      comment: "We ask users this prompt before attempting to close all tabs except active one via context menu"
    )
  public static let savedTabsFolderTitle = NSLocalizedString(
    "SavedTabsFolderTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Saved Tabs",
    comment: "The title for the folder created when all bookmarks are being "
  )
  public static let bookmarkAllTabsTitle = NSLocalizedString(
    "BookmarkAllTabsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add Bookmark for %lld Tabs",
    comment:
      "Action item title of long press for Adding Bookmark for All Tabs in Tab List - The parameter indicates the number of tabs like Add Bookmark for 5 Tabs"
  )
  public static let duplicateActiveTab =
    NSLocalizedString(
      "DuplicateActiveTab",
      tableName: "BraveShared",
      bundle: .module,
      value: "Duplicate Active Tab",
      comment:
        "Action item title of long press for Opening the existing - active Tab url in a new Tab"
    )
  public static let suppressAlertsActionTitle = NSLocalizedString(
    "SuppressAlertsActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Suppress Alerts",
    comment: "Title of alert that seeks permission from user to suppress future JS alerts"
  )
  public static let suppressAlertsActionMessage = NSLocalizedString(
    "SuppressAlertsActionMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Prevent this page from creating additional alerts",
    comment: "Message body of alert that seeks permission from user to suppress future JS alerts"
  )
  public static let openDownloadsFolderErrorDescription =
    NSLocalizedString(
      "OpenDownloadsFolderErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "An unknown error occurred while opening the Downloads folder in the Files app.",
      comment: "Error description when there is an error while navigating to Files App"
    )
  public static let openInNewWindowTitle = NSLocalizedString(
    "OpenInNewWindowTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open in New Window",
    comment: "Context menu item for opening a link in a new window"
  )
  public static let openInNewPrivateWindowTitle = NSLocalizedString(
    "OpenInNewPrivateWindowTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open in New Private Window",
    comment: "Context menu item for opening a link in a new private browsing window"
  )
  public static let newWindowTitle = NSLocalizedString(
    "NewWindowTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Window",
    comment: "Context menu item for opening a new window"
  )
  public static let newPrivateWindowTitle = NSLocalizedString(
    "NewPrivateWindowTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Private Window",
    comment: "Context menu item for opening a new private browsing window"
  )
}

// MARK:-  DefaultBrowserIntroCalloutViewController.swift
extension Strings {
  public struct DefaultBrowserCallout {
    public static let introPrimaryText =
      NSLocalizedString(
        "defaultBrowserCallout.introPrimaryText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open all links with Brave to protect your privacy",
        comment: "Primary text on default browser popup screen"
      )
    public static let introSecondaryText =
      NSLocalizedString(
        "defaultBrowserCallout.introSecondaryText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Brave Shields block trackers & ads, saves data, and saves you time on every site you visit",
        comment: "Secondary text on default browser popup."
      )
    public static let introTertiaryText =
      NSLocalizedString(
        "defaultBrowserCallout.introTertiaryText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Settings, tap Default Browser App, and select Brave.",
        comment: "Tertiary text on default browser popup screen"
      )
    public static let introOpenSettingsButtonText =
      NSLocalizedString(
        "defaultBrowserCallout.introOpenSettingsButtonText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Settings",
        comment: "Button text to open app settings"
      )
    public static let introSkipButtonText =
      NSLocalizedString(
        "defaultBrowserCallout.introCancelButtonText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Not Now",
        comment: "Button text to close the default browser popup."
      )
    public static let notificationTitle =
      NSLocalizedString(
        "defaultBrowserCallout.notificationTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Get Brave protection, on every link",
        comment: "Notification title to promote setting Brave app as default browser"
      )

    public static let notificationBody =
      NSLocalizedString(
        "defaultBrowserCallout.notificationBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Set Brave as your default browser",
        comment: "Notification body to promote setting Brave app as default browser"
      )
  }
}

// MARK:  Callouts

extension Strings {
  public struct Callout {
    public static let defaultBrowserCalloutTitle =
      NSLocalizedString(
        "callout.defaultBrowserTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Privacy. Made Simple.",
        comment: "Title for Default Browser Full Screen Callout"
      )
    public static let defaultBrowserCalloutDescription =
      NSLocalizedString(
        "callout.defaultBrowserCalloutDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "With Brave as default, every link you tap opens with Brave's privacy protections.",
        comment: "Subtitle - Description for Default Browser Full Screen Callout"
      )
    public static let defaultBrowserCalloutButtonDescription =
      NSLocalizedString(
        "callout.defaultBrowserCalloutButtonDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Set Brave as Default Browser?",
        comment:
          "Description - Description used for main button in Default Browser Full Screen Callout"
      )
    public static let defaultBrowserCalloutPrimaryButtonTitle =
      NSLocalizedString(
        "callout.defaultBrowserCalloutPrimaryButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Set Default",
        comment: "Title for main button in Default Browser Full Screen Callout"
      )
    public static let defaultBrowserCalloutSecondaryButtonTitle =
      NSLocalizedString(
        "callout.defaultBrowserCalloutSecondaryButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Skip This",
        comment: "Title for secondary button in Default Browser Full Screen Callout"
      )
    public static let defaultBrowserCalloutSecondaryButtonDescription =
      NSLocalizedString(
        "callout.defaultBrowserCalloutSecondaryButtonDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Already Default?",
        comment: "Description for secondary button in Default Browser Full Screen Callout"
      )
    public static let privacyEverywhereCalloutTitle =
      NSLocalizedString(
        "callout.privacyEverywhereCalloutTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Privacy. Everywhere.",
        comment: "Title for Privacy Everywhere Full Screen Callout"
      )
    public static let privacyEverywhereCalloutDescription =
      NSLocalizedString(
        "callout.privacyEverywhereCalloutDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Get Brave privacy on your computer or tablet, and sync bookmarks & extensions between devices.",
        comment: "Subtitle - Description for Privacy Everywhere Full Screen Callout"
      )
    public static let privacyEverywhereCalloutPrimaryButtonTitle =
      NSLocalizedString(
        "callout.privacyEverywhereCalloutPrimaryButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Now",
        comment: "Title for button in Default Browser Full Screen Callout"
      )
    public static let tabReceivedCalloutTitle =
      NSLocalizedString(
        "callout.tabReceivedCalloutTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Tab Received",
        comment:
          "Title for 'Tab Received' Callout, This is shown in the message when a Tab information is received from another sync device. "
      )
    public static let p3aCalloutTitle =
      NSLocalizedString(
        "callout.p3aCalloutTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Help make Brave better.",
        comment: "Title for p3a (Privacy Preserving Analytics) Full Screen Callout"
      )
    public static let p3aCalloutDescription =
      NSLocalizedString(
        "callout.p3aCalloutDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This helps us learn what Brave features are used most often. Change at any time in Brave Settings under ‘Brave Shields and Other Privacy Settings.",
        comment: "Subtitle - Description for p3a (Privacy Preserving Analytics) Full Screen Callout"
      )
    public static let p3aCalloutToggleTitle =
      NSLocalizedString(
        "callout.p3aCalloutToggleTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Share Private and Anonymous Product Insights.",
        comment:
          "Title for toggle for enabling p3a (Privacy Preserving Analytics) Full Screen Callout"
      )
    public static let p3aCalloutCloseAccessibilityLabel =
      NSLocalizedString(
        "p3aCalloutCloseAccessibilityLabel",
        tableName: "BraveShared",
        bundle: .module,
        value: "Close Privacy Preserving Product Callout",
        comment: ""
      )
    public static let p3aCalloutLinkTitle =
      NSLocalizedString(
        "callout.p3aCalloutLinkTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Learn more about our Privacy Preserving Product Analytics (P3A).",
        comment:
          "Title for the link that navigates to a webpage showing information about p3a (Privacy Preserving Analytics)"
      )
    public static let bottomBarCalloutTitle =
      NSLocalizedString(
        "callout.bottomBarCalloutTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Customize Brave Tabs Bar",
        comment: "Title for Bottom Bar Callout View"
      )
    public static let bottomBarCalloutDescription =
      NSLocalizedString(
        "callout.bottomBarCalloutDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Move the address bar, tabs, bookmarks, and other tools to the bottom of the browser.",
        comment: "Description for Bottom Bar Callout View"
      )
    public static let bottomBarCalloutButtonTitle =
      NSLocalizedString(
        "callout.bottomBarCalloutButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Move Tabs Bar",
        comment: "Button title for Bottom Bar Callout View"
      )
    public static let bottomBarCalloutDismissButtonTitle = NSLocalizedString(
      "braveSearchPromotion.bottomBarCalloutDismissButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Maybe later",
      comment: "Button title for Bottom Bar Callout View for action later"
    )
  }
}

// MARK:  Onboarding

extension Strings {
  public struct Onboarding {
    public static let welcomeScreenTitle =
      NSLocalizedString(
        "onboarding.welcomeScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Welcome to Brave",
        comment: "Title for Welcome Screen in Onboarding"
      )
    public static let ntpOnboardingPopOverTrackerDescription =
      NSLocalizedString(
        "onboarding.ntpOnboardingPopOverTrackerDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "By blocking trackers & ads, websites use less data and load way faster.",
        comment: "Description for the NTP pop-over that describes the tracking information on NTP"
      )
    public static let blockedAdsOnboardingInstructionsText =
      NSLocalizedString(
        "onboarding.blockedAdsOnboardingInstructionsText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Tap to view Brave Shields",
        comment: "The text describing user where they should press in order to open Shield panel"
      )
    public static let blockedAdsOnboardingFootnoteText =
      NSLocalizedString(
        "onboarding.blockedAdsOnboardingFootnoteText",
        tableName: "BraveShared",
        bundle: .module,
        value: "See all the bad stuff Brave blocked, on every page, with Shields.",
        comment: "The footnote indicating what Brave shields is blocking"
      )
    public static let blockedAdsOnboardingNoBigTechInformationText =
      NSLocalizedString(
        "onboarding.blockedAdsOnboardingNoBigTechInformationText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Ads & trackers blocked",
        comment:
          "The description text shown when there is no big tech tracker among trackers. Example usage: 31 Ads & trackers blocked. The number here is presented separately so it is not in translation."
      )
    public static let blockedAdsOnboardingBigTechInformationText =
      NSLocalizedString(
        "onboarding.blockedAdsOnboardingBigTechInformationText",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ and %lld other ads & trackers blocked",
        comment:
          "The description text shown when there is big tech tracker among trackers. Example usage: Google, Amazon and 32 other trackers blocked"
      )
    public static let navigateSettingsOnboardingScreenTitle =
      NSLocalizedString(
        "onboarding.navigateSettingsOnboardingScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Taking you to Settings...",
        comment: "Title for Navigate Settings Screen in Onboarding"
      )
    public static let navigateSettingsOnboardingScreenDescription =
      NSLocalizedString(
        "onboarding.navigateSettingsOnboardingScreenDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Look for 'Default Browser App'",
        comment:
          "Description for Navigate Settings Screen in Onboarding. This part indicating navigation ('Default Browser App') should match the option in iPhone Settings in that Language."
      )
    public static let omniboxOnboardingPopOverTitle =
      NSLocalizedString(
        "onboarding.omniboxOnboardingPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Type a website name or URL",
        comment: "Title for the Omnibox (URL Bar) pop-over that describes faster load times."
      )
    public static let omniboxOnboardingPopOverDescription =
      NSLocalizedString(
        "onboarding.omniboxOnboardingPopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "See the Brave difference:\nNo ads. No trackers. Way faster page load.",
        comment: "Description for the Omnibox (URL Bar) pop-over that describes faster load times."
      )

    public static let linkReceiptTitle =
      NSLocalizedString(
        "onboarding.linkReceiptTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Extend your Brave Firewall + VPN protection",
        comment: "Popup title to let users know they can use the vpn on all their devices"
      )

    public static let linkReceiptDescription =
      NSLocalizedString(
        "onboarding.linkReceiptDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Your Brave VPN subscription can protect up to 10 devices, across Android, iOS, and desktop. Just link your App Store subscription to your Brave account.",
        comment: "Popup description to let users know they can use the vpn on all their devices"
      )

    public static let linkReceiptButton =
      NSLocalizedString(
        "onboarding.linkReceiptButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Link your VPN subscription",
        comment: "Button text to let users know they can use the vpn on all their devices"
      )
  }
}

// MARK:-  ErrorPageHelper.swift
extension Strings {
  public static let errorPageReloadButtonTitle = NSLocalizedString(
    "ErrorPageReloadButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Reload",
    comment: "Shown in error pages on a button that will try to load the page again"
  )
  public static let errorPageOpenInSafariButtonTitle = NSLocalizedString(
    "ErrorPageOpenInSafariButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open in Safari",
    comment: "Shown in error pages for files that can't be shown and need to be downloaded."
  )
  public static let errorPageCantBeReachedTry =
    NSLocalizedString(
      "errorPageCantBeReachedTry",
      tableName: "BraveShared",
      bundle: .module,
      value: "Try re-typing the URL, or opening a search engine and searching for the new URL.",
      comment: "Shown in error pages to suggest a fix to the user."
    )
}

// MARK:-  FindInPageBar.swift
extension Strings {
  public static let findInPagePreviousResultButtonAccessibilityLabel = NSLocalizedString(
    "FindInPagePreviousResultButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Previous in-page result",
    comment: "Accessibility label for previous result button in Find in Page Toolbar."
  )
  public static let findInPageNextResultButtonAccessibilityLabel = NSLocalizedString(
    "FindInPageNextResultButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Next in-page result",
    comment: "Accessibility label for next result button in Find in Page Toolbar."
  )
  public static let findInPageDoneButtonAccessibilityLabel = NSLocalizedString(
    "FindInPageDoneButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Done",
    comment: "Done button in Find in Page Toolbar."
  )
  public static let findInPageFormat = NSLocalizedString(
    "FindInPageFormat",
    tableName: "BraveShared",
    bundle: .module,
    value: "Find \"%@\"",
    comment: "Find %@ text in page."
  )
}

// MARK:-  ReaderModeBarView.swift
extension Strings {
  public static let readerModeDisplaySettingsButtonTitle = NSLocalizedString(
    "ReaderModeDisplaySettingsButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Display Settings",
    comment:
      "Name for display settings button in reader mode. Display in the meaning of presentation, not monitor."
  )
}

// MARK:-  TabLocationView.swift
extension Strings {
  public static let tabToolbarStopButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarStopButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Stop",
    comment: "Accessibility Label for the tab toolbar Stop button"
  )
  public static let tabToolbarPlaylistButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarPlaylistButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Playlist",
    comment: "Accessibility Label for the tab toolbar Playlist button"
  )
  public static let tabToolbarAddToPlaylistButtonAccessibilityLabel = NSLocalizedString(
    "tabToolbarAddToPlaylistButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add to Playlist",
    comment: "Accessibility Label for the tab toolbar Add to Playlist button"
  )
  public static let tabToolbarReloadButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarReloadButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Reload",
    comment: "Accessibility Label for the tab toolbar Reload button"
  )
  public static let tabToolbarVoiceSearchButtonAccessibilityLabel = NSLocalizedString(
    "tabToolbarVoiceSearchButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Voice Search",
    comment: "Accessibility Label for the tab toolbar Reload button"
  )
  public static let tabToolbarSearchAddressPlaceholderText = NSLocalizedString(
    "TabToolbarSearchAddressPlaceholderText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search or enter address",
    comment: "The text shown in the URL bar on about:home"
  )
  public static let tabToolbarWarningImageAccessibilityLabel = NSLocalizedString(
    "TabToolbarWarningImageAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Insecure connection",
    comment:
      "Accessibility label for the lock icon, which is only present if the connection is insecure"
  )
  public static let tabToolbarLockImageAccessibilityLabel = NSLocalizedString(
    "TabToolbarLockImageAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Secure connection",
    comment:
      "Accessibility label for the lock icon, which is only present if the connection is secure"
  )
  public static let tabToolbarReaderViewButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarReaderViewButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Reader View",
    comment: "Accessibility label for the Reader View button"
  )
  public static let tabToolbarReaderViewButtonTitle = NSLocalizedString(
    "TabToolbarReaderViewButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add to Reading List",
    comment: "Accessibility label for action adding current page to reading list."
  )
  public static let searchSuggestionsSectionHeader = NSLocalizedString(
    "SearchSuggestionsSectionHeader",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Suggestions",
    comment: "Section header for search suggestions option"
  )
  public static let findOnPageSectionHeader = NSLocalizedString(
    "FindOnPageSectionHeader",
    tableName: "BraveShared",
    bundle: .module,
    value: "On This Page",
    comment: "Section header for find in page option"
  )
  public static let searchHistorySectionHeader = NSLocalizedString(
    "SearchHistorySectionHeader",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Tabs & Bookmarks & History",
    comment: "Section header for history and bookmarks and open tabs option"
  )
  public static let searchSuggestionOpenTabActionTitle = NSLocalizedString(
    "searchSuggestionOpenTabActionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Switch to this tab",
    comment:
      "Action title for Switching to an existing tab for the suggestion item shown on the table list"
  )
  public static let tabToolbarNotSecureTitle = NSLocalizedString(
    "tabToolbarNotSecureTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Not Secure",
    comment: "A label shown next to a URL that loaded in some insecure way"
  )
}

// MARK: - PageSecurityView.swift
extension Strings {
  public enum PageSecurityView {
    public static let pageUnknownStatusTitle = NSLocalizedString(
      "pageSecurityView.pageUnknownStatusTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Site connection not verified.",
      comment: ""
    )
    public static let pageUnknownStatusWarning = NSLocalizedString(
      "pageSecurityView.pageUnknownStatusWarning",
      tableName: "BraveShared",
      bundle: .module,
      value: "Your connection to this site could not be fully verified. You may proceed, or try reloading the page to establish a secure connection.",
      comment: ""
    )
    public static let pageNotSecureTitle = NSLocalizedString(
      "pageSecurityView.pageNotSecureTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Your connection to this site is not secure.",
      comment: ""
    )
    public static let pageNotFullySecureTitle = NSLocalizedString(
      "pageSecurityView.pageNotFullySecureTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Your connection to this site is not fully secure.",
      comment: ""
    )
    public static let pageNotSecureDetailedWarning = NSLocalizedString(
      "pageSecurityView.pageNotSecureDetailedWarning",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "You should not enter any sensitive information on this site (for example, passwords or credit cards), because it could be stolen by attackers.",
      comment: ""
    )
    public static let viewCertificateButtonTitle = NSLocalizedString(
      "pageSecurityView.viewCertificateButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "View Certificate",
      comment: ""
    )
    public static let signIntoWebsiteURLBarTitle = NSLocalizedString(
      "pageSecurityView.signIntoWebsiteURLBarTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sign in to website",
      comment: ""
    )
  }
}

// MARK:-  TabToolbar.swift
extension Strings {
  public static let tabToolbarBackButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarBackButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Back",
    comment: "Accessibility label for the Back button in the tab toolbar."
  )
  public static let tabToolbarForwardButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarForwardButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Forward",
    comment: "Accessibility Label for the tab toolbar Forward button"
  )
  public static let tabToolbarShareButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarShareButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Share",
    comment: "Accessibility Label for the browser toolbar Share button"
  )
  public static let tabToolbarMenuButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarMenuButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Menu",
    comment: "Accessibility Label for the browser toolbar Menu button"
  )
  public static let tabToolbarAddTabButtonAccessibilityLabel = NSLocalizedString(
    "TabToolbarAddTabButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add Tab",
    comment: "Accessibility label for the Add Tab button in the Tab Tray."
  )
  public static let tabToolbarAccessibilityLabel = NSLocalizedString(
    "TabToolbarAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Navigation Toolbar",
    comment: "Accessibility label for the navigation toolbar displayed at the bottom of the screen."
  )
}

// MARK:-  TabTrayController.swift
extension Strings {
  public static let tabAccessibilityCloseActionLabel = NSLocalizedString(
    "TabAccessibilityCloseActionLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Close",
    comment: "Accessibility label for action denoting closing a tab in tab list (tray)"
  )
  public static let tabTrayAccessibilityLabel = NSLocalizedString(
    "TabTrayAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Tabs Tray",
    comment: "Accessibility label for the Tabs Tray view."
  )
  public static let tabTrayEmptyVoiceOverText = NSLocalizedString(
    "TabTrayEmptyVoiceOverText",
    tableName: "BraveShared",
    bundle: .module,
    value: "No tabs",
    comment: "Message spoken by VoiceOver to indicate that there are no tabs in the Tabs Tray"
  )
  public static let tabTraySingleTabPositionFormatVoiceOverText = NSLocalizedString(
    "TabTraySingleTabPositionFormatVoiceOverText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Tab %@ of %@",
    comment:
      "Message spoken by VoiceOver saying the position of the single currently visible tab in Tabs Tray, along with the total number of tabs. E.g. \"Tab 2 of 5\" says that tab 2 is visible (and is the only visible tab), out of 5 tabs total."
  )
  public static let tabTrayMultiTabPositionFormatVoiceOverText = NSLocalizedString(
    "TabTrayMultiTabPositionFormatVoiceOverText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Tabs %@ to %@ of %@",
    comment:
      "Message spoken by VoiceOver saying the range of tabs that are currently visible in Tabs Tray, along with the total number of tabs. E.g. \"Tabs 8 to 10 of 15\" says tabs 8, 9 and 10 are visible, out of 15 tabs total."
  )
  public static let tabTrayClosingTabAccessibilityNotificationText = NSLocalizedString(
    "TabTrayClosingTabAccessibilityNotificationText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Closing tab",
    comment:
      "Accessibility label (used by assistive technology) notifying the user that the tab is being closed."
  )
  public static let tabTrayCellCloseAccessibilityHint = NSLocalizedString(
    "TabTrayCellCloseAccessibilityHint",
    tableName: "BraveShared",
    bundle: .module,
    value: "Swipe right or left with three fingers to close the tab.",
    comment: "Accessibility hint for tab tray's displayed tab."
  )
  public static let tabTrayAddTabAccessibilityLabel = NSLocalizedString(
    "TabTrayAddTabAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add Tab",
    comment: "Accessibility label for the Add Tab button in the Tab Tray."
  )
  public static let `private` = NSLocalizedString(
    "Private",
    tableName: "BraveShared",
    bundle: .module,
    value: "Private",
    comment: "Private button title"
  )
  public static let privateBrowsing = NSLocalizedString(
    "PrivateBrowsing",
    tableName: "BraveShared",
    bundle: .module,
    value: "Private Browsing",
    comment: ""
  )
  public static let tabTraySearchBarTitle = NSLocalizedString(
    "TabTraySearchBarTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Tabs",
    comment: "Title displayed for placeholder inside Search Bar in Tab Tray"
  )
}

// MARK:-  TabTrayButtonExtensions.swift
extension Strings {
  public static let tabPrivateModeToggleAccessibilityLabel = NSLocalizedString(
    "TabPrivateModeToggleAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Private Mode",
    comment: "Accessibility label for toggling on/off private mode"
  )
  public static let tabPrivateModeToggleAccessibilityHint = NSLocalizedString(
    "TabPrivateModeToggleAccessibilityHint",
    tableName: "BraveShared",
    bundle: .module,
    value: "Turns private mode on or off",
    comment: "Accessiblity hint for toggling on/off private mode"
  )
  public static let tabPrivateModeToggleAccessibilityValueOn = NSLocalizedString(
    "TabPrivateModeToggleAccessibilityValueOn",
    tableName: "BraveShared",
    bundle: .module,
    value: "On",
    comment: "Toggled ON accessibility value"
  )
  public static let tabPrivateModeToggleAccessibilityValueOff = NSLocalizedString(
    "TabPrivateModeToggleAccessibilityValueOff",
    tableName: "BraveShared",
    bundle: .module,
    value: "Off",
    comment: "Toggled OFF accessibility value"
  )
  public static let tabTrayNewTabButtonAccessibilityLabel = NSLocalizedString(
    "TabTrayNewTabButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Tab",
    comment: "Accessibility label for the New Tab button in the tab toolbar."
  )
}

// MARK:-  URLBarView.swift
extension Strings {
  public static let URLBarViewLocationTextViewAccessibilityLabel = NSLocalizedString(
    "URLBarViewLocationTextViewAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Address and Search",
    comment:
      "Accessibility label for address and search field, both words (Address, Search) are therefore nouns."
  )
}

// MARK:-  LoginListViewController.swift
extension Strings {
  // Titles for selection/deselect/delete buttons
  public static let loginListDeselectAllButtonTitle = NSLocalizedString(
    "LoginListDeselectAllButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Deselect All",
    comment: "Label for the button used to deselect all logins."
  )
  public static let loginListSelectAllButtonTitle = NSLocalizedString(
    "LoginListSelectAllButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Select All",
    comment: "Label for the button used to select all logins."
  )
  public static let loginListScreenTitle = NSLocalizedString(
    "LoginListScreenTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Logins",
    comment: "Title for Logins List View screen."
  )
  public static let loginListNoLoginTitle = NSLocalizedString(
    "LoginListNoLoginTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "No logins found",
    comment: "Label displayed when no logins are found after searching."
  )
}

// MARK:-  LoginDetailViewController.swift
extension Strings {
  public static let loginDetailUsernameCellTitle = NSLocalizedString(
    "LoginDetailUsernameCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "username",
    comment: "Label displayed above the username row in Login Detail View."
  )
  public static let loginDetailPasswordCellTitle = NSLocalizedString(
    "LoginDetailPasswordCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "password",
    comment: "Label displayed above the password row in Login Detail View."
  )
  public static let loginDetailWebsiteCellTitle = NSLocalizedString(
    "LoginDetailWebsiteCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "website",
    comment: "Label displayed above the website row in Login Detail View."
  )
  public static let loginDetailLastModifiedCellFormatTitle = NSLocalizedString(
    "LoginDetailLastModifiedCellFormatTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Last modified %@",
    comment:
      "Footer label describing when the current login was last modified with the timestamp as the parameter."
  )
}

// MARK:-  ReaderModeHandler.swift
extension Strings {
  public static let readerModeLoadingContentDisplayText = NSLocalizedString(
    "ReaderModeLoadingContentDisplayText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Loading content…",
    comment:
      "Message displayed when the reader mode page is loading. This message will appear only when sharing to Brave reader mode from another app."
  )
  public static let readerModePageCantShowDisplayText = NSLocalizedString(
    "ReaderModePageCantShowDisplayText",
    tableName: "BraveShared",
    bundle: .module,
    value: "The page could not be displayed in Reader View.",
    comment:
      "Message displayed when the reader mode page could not be loaded. This message will appear only when sharing to Brave reader mode from another app."
  )
  public static let readerModeLoadOriginalLinkText = NSLocalizedString(
    "ReaderModeLoadOriginalLinkText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Load original page",
    comment:
      "Link for going to the non-reader page when the reader view could not be loaded. This message will appear only when sharing to Brave reader mode from another app."
  )
  public static let readerModeErrorConvertDisplayText = NSLocalizedString(
    "ReaderModeErrorConvertDisplayText",
    tableName: "BraveShared",
    bundle: .module,
    value: "There was an error converting the page",
    comment: "Error displayed when reader mode cannot be enabled"
  )
}

// MARK:-  ReaderModeStyleViewController.swift
extension Strings {
  public static let readerModeBrightSliderAccessibilityLabel = NSLocalizedString(
    "ReaderModeBrightSliderAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brightness",
    comment: "Accessibility label for brightness adjustment slider in Reader Mode display settings"
  )
  public static let readerModeFontTypeButtonAccessibilityHint = NSLocalizedString(
    "ReaderModeFontTypeButtonAccessibilityHint",
    tableName: "BraveShared",
    bundle: .module,
    value: "Changes font type.",
    comment: "Accessibility hint for the font type buttons in reader mode display settings"
  )
  public static let readerModeFontButtonSansSerifTitle = NSLocalizedString(
    "ReaderModeFontButtonSansSerifTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Sans-serif",
    comment: "Font type setting in the reading view settings"
  )
  public static let readerModeFontButtonSerifTitle = NSLocalizedString(
    "ReaderModeFontButtonSerifTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Serif",
    comment: "Font type setting in the reading view settings"
  )
  public static let readerModeSmallerFontButtonTitle = NSLocalizedString(
    "ReaderModeSmallerFontButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "-",
    comment:
      "Button for smaller reader font size. Keep this extremely short! This is shown in the reader mode toolbar."
  )
  public static let readerModeSmallerFontButtonAccessibilityLabel = NSLocalizedString(
    "ReaderModeSmallerFontButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Decrease text size",
    comment:
      "Accessibility label for button decreasing font size in display settings of reader mode"
  )
  public static let readerModeBiggerFontButtonTitle = NSLocalizedString(
    "ReaderModeBiggerFontButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "+",
    comment:
      "Button for larger reader font size. Keep this extremely short! This is shown in the reader mode toolbar."
  )
  public static let readerModeBiggerFontButtonAccessibilityLabel = NSLocalizedString(
    "ReaderModeBiggerFontButtonAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Increase text size",
    comment:
      "Accessibility label for button increasing font size in display settings of reader mode"
  )
  public static let readerModeFontSizeLabelText = NSLocalizedString(
    "ReaderModeFontSizeLabelText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Aa",
    comment:
      "Button for reader mode font size. Keep this extremely short! This is shown in the reader mode toolbar."
  )
  public static let readerModeThemeButtonAccessibilityHint = NSLocalizedString(
    "ReaderModeThemeButtonAccessibilityHint",
    tableName: "BraveShared",
    bundle: .module,
    value: "Changes color theme.",
    comment:
      "Accessibility hint for the color theme setting buttons in reader mode display settings"
  )

  public static let readerModeButtonTitle =
    NSLocalizedString(
      "readerModeSettingsButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "Reader Mode",
      comment: "Title of a bar that show up when you enter reader mode."
    )
}

// MARK: -  SearchSettingsViewController

extension Strings {
  public static let searchSettingNavTitle = NSLocalizedString(
    "SearchSettingNavTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search",
    comment: "Navigation title for search settings."
  )
  public static let searchSettingSuggestionCellTitle = NSLocalizedString(
    "SearchSettingSuggestionCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Search Suggestions",
    comment: "Label for show search suggestions setting."
  )
  public static let searchSettingRecentSearchesCellTitle = NSLocalizedString(
    "SearchSettingRecentSearchesCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Recent Searches",
    comment: "Label for showing recent search setting."
  )
  public static let searchSettingBrowserSuggestionCellTitle =
    NSLocalizedString(
      "SearchSettingBrowserSuggestionCellTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show Browser Suggestions",
      comment: "Label for showing browser suggestion setting"
    )
  public static let searchSettingBrowserSuggestionCellDescription =
    NSLocalizedString(
      "SearchSettingBrowserSuggestionCellDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn this on to include results from your History and Bookmarks when searching",
      comment: "Label for showing browser suggestion setting description"
    )
  public static let searchSettingAddCustomEngineCellTitle =
    NSLocalizedString(
      "searchSettingAddCustomEngineCellTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add Custom Search Engine",
      comment: "Add Custom Search Engine Table Cell Row Title"
    )
}

// MARK: - CustomEngineViewController

extension Strings {
  public struct CustomSearchEngine {
    public static let customEngineNavigationTitle = NSLocalizedString(
      "customSearchEngine.navigationTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add Search Engine",
      comment: "Navigation Bar title"
    )

    public static let customEngineAddDesription = NSLocalizedString(
      "customSearchEngine.addEngineDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Write the search url and replace the query with %s.\nFor example: https://youtube.com/search?q=%s \n(If the site supports OpenSearch an option to add automatically will be provided while editing this field.)",
      comment: "Label explaining how to add search engine."
    )

    public static let customEngineAutoAddTitle = NSLocalizedString(
      "customSearchEngine.autoAddTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto Add",
      comment: "Button title for Auto Add in header"
    )

    public static let thirdPartySearchEngineAddErrorTitle = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineAddErrorTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Custom Search Engine Error",
      comment: "A title explaining that there is error while adding a search engine"
    )

    public static let thirdPartySearchEngineAddErrorDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineAddErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "The custom search engine could not be added. Please try again later.",
      comment: "A descriotion explaining that there is error while adding a search engine"
    )

    public static let thirdPartySearchEngineMissingInfoErrorDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineMissingInfoErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Please fill both Title and URL fields.",
      comment: "A descriotion explaining that the fields must filled while adding a search engine. "
    )

    public static let thirdPartySearchEngineIncorrectFormErrorTitle = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineIncorrectFormErrorTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Search URL Query Error ",
      comment: "A title explaining that there is a formatting error in URL field"
    )

    public static let thirdPartySearchEngineIncorrectFormErrorDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineIncorrectFormErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Write the search url and replace the query with %s. ",
      comment: "A description explaining that there is a formatting error in URL field"
    )

    public static let thirdPartySearchEngineDuplicateErrorDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineDuplicateErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "A search engine with this title or URL has already been added.",
      comment: "A message explaining a replica search engine is already added"
    )

    public static let thirdPartySearchEngineInsecureURLErrorDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineInsecureURLErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "The copied text should be a valid secure URL which starts with 'https://'",
      comment: "A description explaining the copied url should be secure"
    )

    public static let thirdPartySearchEngineAddedToastTitle = NSLocalizedString(
      "custonmSearchEngine.thirdPartySearchEngineAddedToastTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Added Search engine!",
      comment: "The success message that appears after a user sucessfully adds a new search engine"
    )

    public static let thirdPartySearchEngineAddAlertTitle = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineAddAlertTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add Search Provider?",
      comment: "The title that asks the user to Add the search provider"
    )

    public static let thirdPartySearchEngineAddAlertDescription = NSLocalizedString(
      "customSearchEngine.thirdPartySearchEngineAddAlertDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "The new search engine will appear in the quick search bar.",
      comment:
        "The message that asks the user to Add the search provider explaining where the search engine will appear"
    )

    public static let deleteEngineAlertTitle = NSLocalizedString(
      "customSearchEngine.deleteEngineAlertTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Are you sure you want to delete %@?",
      comment:
        "The alert title shown to user when custom search engine will be deleted while it is default search engine. The parameter will be replace with name of the search engine."
    )

    public static let deleteEngineAlertDescription = NSLocalizedString(
      "customSearchEngine.deleteEngineAlertDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Deleting a custom search engine while it is default will switch default engine automatically.",
      comment:
        "The warning description shown to user when custom search engine will be deleted while it is default search engine."
    )

    public static let customSearchEngineAddErrorTitle = NSLocalizedString(
      "customSearchEngine.customSearchEngineAddErrorTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Error Adding Custom Search Engine",
      comment: "A title explaining that an error shown while adding custom search engine"
    )

    public static let insecureSearchTemplateURLErrorDescription = NSLocalizedString(
      "customSearchEngine.insecureSearchTemplateURLErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Insecure Custom Search Template for",
      comment:
        "A description explaining that search template url is insecure, it is used for instance - Insecure Custom Search Template for Brave Search, Brave Search is a search engineand on a new seperate line"
    )

    public static let insecureSuggestionTemplateURLErrorDescription = NSLocalizedString(
      "customSearchEngine.insecureSuggestionTemplateURLErrorDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Insecure Custom Suggestion Template for",
      comment:
        "A description explaining that suggestion template url is insecure, it is used for instance - Insecure Custom Suggestion Template for Brave Search, Brave Search is name of search engine on a new seperate line"
    )

    public static let searchTemplateTitle = NSLocalizedString(
      "customSearchEngine.searchTemplateTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Search Template:",
      comment:
        "Search Template title - for instance it will be used Search Template: Brave Search - Brave Search is the name of Search Engine on  seperate line"
    )

    public static let suggestionTemplateTitle = NSLocalizedString(
      "customSearchEngine.suggestionTemplateTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Suggestion Template:",
      comment:
        "Suggestion Template title - for instance it will be used Suggestion Template: Brave Search - Brave Search is the name of Search Engine on  seperate line"
    )

    public static let engineExistsAlertDescription = NSLocalizedString(
      "customSearchEngine.engineAlertExistsAlertDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "A search engine with the same name already exists.",
      comment: "The warning description shown to user when custom search engine already exists."
    )
  }
}

// MARK: - OptionsMenu

extension Strings {
  public struct OptionsMenu {
    public static let menuSectionTitle = NSLocalizedString(
      "optionsMenu.menuSectionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Features",
      comment: "Privacy Features Section title"
    )
    public static let braveVPNItemTitle = NSLocalizedString(
      "optionsMenu.braveVPNItemTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "VPN",
      comment: "Brave VPN Item Menu title"
    )
    public static let braveVPNItemDescription = NSLocalizedString(
      "optionsMenu.braveVPNItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Protect your entire device online",
      comment: "The subtitle description of menu item Brave VPN"
    )
    public static let braveTalkItemTitle = NSLocalizedString(
      "optionsMenu.braveTalkItemTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Talk",
      comment: "Brave Talk Item Menu title"
    )
    public static let braveTalkItemDescription = NSLocalizedString(
      "optionsMenu.braveTalkItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Private video calls, right in your browser",
      comment: "The subtitle description of menu item Brave Talk"
    )
    public static let braveLeoItemDescription = NSLocalizedString(
      "optionsMenu.braveLeoItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Leo is an AI-powered smart assistant, built right into the browser",
      comment:
        "The subtitle description of menu item Brave Leo. Brave Leo is the name of a product and should not be translated."
    )
    public static let braveNewsItemTitle = NSLocalizedString(
      "optionsMenu.braveNewsItemTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "News",
      comment: "Brave News Item Menu title"
    )
    public static let braveNewsItemDescription = NSLocalizedString(
      "optionsMenu.braveNewsItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Today's top stories in a private news feed",
      comment: "The subtitle description of menu item Brave News"
    )
    public static let bravePlaylistItemTitle = NSLocalizedString(
      "optionsMenu.bravePlaylistItemTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Playlist",
      comment: "Brave News Item Menu title"
    )
    public static let bravePlaylistItemDescription = NSLocalizedString(
      "optionsMenu.bravePlaylistItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Keep an offline playlist of any video/stream",
      comment: "The subtitle description of menu item Brave Playlist"
    )
    public static let braveWalletItemDescription = NSLocalizedString(
      "optionsMenu.braveWalletItemDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "The secure crypto wallet, no extension required",
      comment: "The subtitle description of menu item Brave Wallet"
    )
  }
}

// MARK: - BraveSearch Promotion

extension Strings {
  public struct BraveSearchPromotion {
    public static let braveSearchPromotionBannerTitle = NSLocalizedString(
      "braveSearchPromotion.bannerTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Support independent search with better privacy",
      comment: "Brave Search Banner Promotion title in Search Suggestions"
    )
    public static let braveSearchPromotionBannerDescription = NSLocalizedString(
      "braveSearchPromotion.bannerDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Search doesn't profile you.",
      comment: "Brave Search Banner Promotion description content in Search Suggestions"
    )
    public static let braveSearchPromotionBannerTryButtonTitle = NSLocalizedString(
      "braveSearchPromotion.bannerTryButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Try Brave Search",
      comment: "Brave Search Banner Promotion title for try button in Search Suggestions"
    )
    public static let braveSearchPromotionBannerMaybeLaterButtonTitle = NSLocalizedString(
      "braveSearchPromotion.bannerMaybeLaterButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Maybe later",
      comment:
        "Brave Search Banner Promotion title for maybe later button to activate promotion later in Search Suggestions"
    )
    public static let braveSearchPromotionBannerDismissButtonTitle = NSLocalizedString(
      "braveSearchPromotion.braveSearchPromotionBannerDismissButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Dismiss",
      comment:
        "Brave Search Banner Promotion title for dismiss button to remove promotion in Search Suggestions"
    )
  }
}

// MARK:-  SettingsContentViewController.swift
extension Strings {
  public static let settingsContentLoadErrorMessage = NSLocalizedString(
    "SettingsContentLoadErrorMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Could not load page.",
    comment: "Error message that is shown in settings when there was a problem loading"
  )
}

// MARK:-  SearchInputView.swift
extension Strings {
  public static let searchInputViewTextFieldAccessibilityLabel = NSLocalizedString(
    "SearchInputViewTextFieldAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Input Field",
    comment: "Accessibility label for the search input field in the Logins list"
  )
  public static let searchInputViewTitle = NSLocalizedString(
    "SearchInputViewTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search",
    comment: "Title for the search field at the top of the Logins list screen"
  )
  public static let searchInputViewClearButtonTitle = NSLocalizedString(
    "SearchInputViewClearButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear Search",
    comment:
      "Accessibility message e.g. spoken by VoiceOver after the user taps the close button in the search field to clear the search and exit search mode"
  )
  public static let searchInputViewOverlayAccessibilityLabel = NSLocalizedString(
    "SearchInputViewOverlayAccessibilityLabel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enter Search Mode",
    comment: "Accessibility label for entering search mode for logins"
  )
}

// MARK:-  MenuHelper.swift
extension Strings {
  public static let menuItemRevealPasswordTitle = NSLocalizedString(
    "MenuItemRevealPasswordTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Reveal",
    comment: "Reveal password text selection menu item"
  )
  public static let menuItemHidePasswordTitle = NSLocalizedString(
    "MenuItemHidePasswordTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hide",
    comment: "Hide password text selection menu item"
  )
  public static let menuItemCopyTitle = NSLocalizedString(
    "MenuItemCopyTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy",
    comment: "Copy password text selection menu item"
  )
  public static let menuItemOpenWebsiteTitle = NSLocalizedString(
    "MenuItemOpenTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Website",
    comment: "Open and Fill website text selection menu item"
  )
}

// MARK:- Settings.
extension Strings {
  public static let clearPrivateData = NSLocalizedString(
    "ClearPrivateData",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear Private Data",
    comment: "Section title in settings panel"
  )
  public static let clearPrivateDataAlertTitle = NSLocalizedString(
    "ClearPrivateDataAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear Data",
    comment: ""
  )
  public static let clearPrivateDataAlertMessage = NSLocalizedString(
    "ClearPrivateDataAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Are you sure?",
    comment: ""
  )
  public static let clearPrivateDataAlertYesAction = NSLocalizedString(
    "ClearPrivateDataAlertYesAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Yes, Delete",
    comment: ""
  )
  public static let clearDataNow = NSLocalizedString(
    "ClearPrivateDataNow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear Data Now",
    comment: "Button in settings that clears private data for the selected items."
  )
  public static let displaySettingsSection = NSLocalizedString(
    "DisplaySettingsSection",
    tableName: "BraveShared",
    bundle: .module,
    value: "Display",
    comment: "Section name for display preferences."
  )
  public static let tabsSettingsSectionTitle = NSLocalizedString(
    "SettingsTabsSectionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Tabs",
    comment: "Tabs settings section title"
  )
  public static let tabsOptionTopBar = NSLocalizedString(
    "SettingsTabsOptionTopBar",
    tableName: "BraveShared",
    bundle: .module,
    value: "Top Bar",
    comment:
      "An option for the URL bar position. (Top Bar and Bottom Bar). Refers to the location of the url bar"
  )
  public static let tabsOptionBottomBar = NSLocalizedString(
    "SettingsTabsOptionBottomBar",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bottom Bar",
    comment:
      "An option for the URL bar position. (Top Bar and Bottom Bar). Refers to the location of the url bar"
  )
  public static let otherSettingsSection = NSLocalizedString(
    "OtherSettingsSection",
    tableName: "BraveShared",
    bundle: .module,
    value: "Other Settings",
    comment: "Section name for other settings."
  )
  public static let otherPrivacySettingsSection = NSLocalizedString(
    "OtherPrivacySettingsSection",
    tableName: "BraveShared",
    bundle: .module,
    value: "Other Privacy Settings",
    comment: "Section name for other privacy settings"
  )
  public static let braveRewardsTitle = NSLocalizedString(
    "BraveRewardsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Rewards",
    comment: "Brave Rewards title"
  )
  public static let braveRewardsSettingsTitle = NSLocalizedString(
    "BraveRewardsSettingsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Rewards",
    comment: "Brave Rewards title in settings"
  )
  public static let hideRewardsIcon = NSLocalizedString(
    "HideRewardsIcon",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hide Brave Rewards Icon",
    comment: "Hides the rewards icon"
  )
  public static let hideRewardsIconSubtitle = NSLocalizedString(
    "HideRewardsIconSubtitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hides the Brave Rewards icon when Brave Rewards is not enabled",
    comment: "Hide the rewards icon explination."
  )
  public static let walletCreationDate = NSLocalizedString(
    "WalletCreationDate",
    tableName: "BraveShared",
    bundle: .module,
    value: "Wallet Creation Date",
    comment: "The date your wallet was created"
  )
  public static let copyWalletSupportInfo = NSLocalizedString(
    "CopyWalletSupportInfo",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy Support Info",
    comment: "Copy rewards internals info for support"
  )
  public static let settingsLicenses = NSLocalizedString(
    "SettingsLicenses",
    tableName: "BraveShared",
    bundle: .module,
    value: "Licenses",
    comment: "Row name for licenses."
  )
  public static let openBraveRewardsSettings = NSLocalizedString(
    "OpenBraveRewardsSettings",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Brave Rewards Settings",
    comment: "Button title for opening the Brave Rewards panel to settings"
  )
  public static let setDefaultBrowserSettingsCell =
    NSLocalizedString(
      "setDefaultBrowserSettingsCell",
      tableName: "BraveShared",
      bundle: .module,
      value: "Set as Default Browser",
      comment: "Settings item to set the Brave as a default browser on the iOS device."
    )
  public static let setDefaultBrowserCalloutTitle =
    NSLocalizedString(
      "setDefaultBrowserCalloutTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave can now be set as your default browser in iOS. Tap here to open settings.",
      comment: ""
    )
  public static let defaultBrowserCalloutCloseAccesabilityLabel =
    NSLocalizedString(
      "defaultBrowserCalloutCloseAccesabilityLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Close default browser callout",
      comment: ""
    )
  public static let enablePullToRefresh =
    NSLocalizedString(
      "enablePullToRefresh",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enable Pull-to-refresh",
      comment:
        "Describes whether or not the feature that allows the user to pull down from the top of a web page a certain amount before it triggers a page refresh"
    )
}

extension Strings {
  public struct Settings {
    public static let autocloseTabsSetting =
      NSLocalizedString(
        "settings.autocloseTabsSetting",
        tableName: "BraveShared",
        bundle: .module,
        value: "Close Tabs",
        comment: "Name of app setting that allows users to automatically close tabs."
      )
    public static let autocloseTabsSettingFooter =
      NSLocalizedString(
        "settings.autocloseTabsSettingFooter",
        tableName: "BraveShared",
        bundle: .module,
        value: "Allow Brave to automatically close tabs that haven't recently been viewed.",
        comment: "Description of autoclose tabs feature."
      )
    public static let autocloseTabsManualOption =
      NSLocalizedString(
        "settings.autocloseTabsManualOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "Manually",
        comment: "Settings option to never close tabs automatically, must be done manually"
      )
    public static let autocloseTabsOneDayOption =
      NSLocalizedString(
        "settings.autocloseTabsOneDayOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "After One Day",
        comment: "Settings option to close old tabs after 1 day"
      )
    public static let autocloseTabsOneWeekOption =
      NSLocalizedString(
        "settings.autocloseTabsOneWeekOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "After One Week",
        comment: "Settings option to close old tabs after 1 week"
      )
    public static let autocloseTabsOneMonthOption =
      NSLocalizedString(
        "settings.autocloseTabsOneMonthOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "After One Month",
        comment: "Settings option to close old tabs after 1 month"
      )

    public static let mediaRootSetting =
      NSLocalizedString(
        "settings.media.rootSetting",
        tableName: "BraveShared",
        bundle: .module,
        value: "Media",
        comment:
          "Setting title for 'Media' sections. Media section offerts various options to manipulate media playback"
      )

    public static let mediaGeneralSection =
      NSLocalizedString(
        "settings.media.general",
        tableName: "BraveShared",
        bundle: .module,
        value: "General",
        comment: "Header for the general settings section"
      )

    public static let youtube =
      NSLocalizedString(
        "settings.youtube",
        tableName: "BraveShared",
        bundle: .module,
        value: "YouTube",
        comment: "Header for the YouTube settings section"
      )

    public static let openYouTubeInBrave =
      NSLocalizedString(
        "settings.openYouTubeInBrave",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open YouTube links in Brave",
        comment: "A toggle label which lets the user always open YouTube urls in Brave"
      )

    public static let highestQualityPlayback =
      NSLocalizedString(
        "settings.highestQualityPlayback",
        tableName: "BraveShared",
        bundle: .module,
        value: "Highest Quality Playback",
        comment: "Label for the navigation link to quality settings view"
      )

    public static let highestQualityPlaybackDetail =
      NSLocalizedString(
        "settings.highestQualityPlaybackDetail",
        tableName: "BraveShared",
        bundle: .module,
        value: "Allows high-quality resolution",
        comment: "Detail text for the navigation link to quality settings view"
      )

    public static let qualitySettings =
      NSLocalizedString(
        "settings.qualitySettings",
        tableName: "BraveShared",
        bundle: .module,
        value: "Quality Settings",
        comment: "Title for the quality settings view"
      )

    public static let sendUsagePingTitle =
      NSLocalizedString(
        "settings.sendUsagePingTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Automatically send daily usage ping to Brave",
        comment: "Title to explain the daily usage ping toggle"
      )

    public static let sendUsagePingDescription =
      NSLocalizedString(
        "settings.sendUsagePingDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "This private ping lets Brave estimate active users.",
        comment: "Description to explain the daily usage ping toggle"
      )
  }
}

// MARK:- Error pages.
extension Strings {
  public static let errorPagesCertWarningTitle = NSLocalizedString(
    "ErrorPagesCertWarningTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Your connection is not private",
    comment: "Title on the certificate error page"
  )

  public static let errorPagesCertErrorTitle = NSLocalizedString(
    "ErrorPagesCertErrorTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "This site can’t provide a secure connection",
    comment: "Title on the certificate error page"
  )

  public static let errorPagesMoreDetailsButton = NSLocalizedString(
    "ErrorPagesMoreDetailsButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "More details",
    comment: "Label for button to perform advanced actions on the error page"
  )

  public static let errorPagesHideDetailsButton = NSLocalizedString(
    "ErrorPagesHideDetailsButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hide details",
    comment: "Label for button to hide advanced actions on the error page"
  )

  public static let errorPagesLearnMoreButton = NSLocalizedString(
    "ErrorPagesLearnMoreButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Learn more",
    comment: "Label for learn more link on error page"
  )

  public static let errorPagesAdvancedWarningTitle = NSLocalizedString(
    "ErrorPagesAdvancedWarningTitle",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Attackers might be trying to steal your information from %@ (for example, passwords, messages, or credit cards).",
    comment: "Warning text when clicking the Advanced button on error pages"
  )

  public static let errorPagesAdvancedWarningDetails = NSLocalizedString(
    "ErrorPagesAdvancedWarningDetails",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "This server could not prove that it is %@; its security certificate is not trusted by your device's operating system. This may be caused by a misconfiguration or an attacker trying to intercept your connection.",
    comment: "Additional warning text when clicking the Advanced button on error pages"
  )

  public static let errorPagesBackToSafetyButton = NSLocalizedString(
    "ErrorPagesBackToSafetyButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Back to safety",
    comment: "Label for button to go back from the error page"
  )

  public static let errorPagesProceedAnywayButton = NSLocalizedString(
    "ErrorPagesProceedAnywayButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Proceed to %@ (unsafe)",
    comment: "Button label to temporarily continue to the site from the certificate error page"
  )

  public static let errorPagesNoInternetTitle = NSLocalizedString(
    "ErrorPagesNoInternetTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "No internet access",
    comment: "Title of the No Internet error page"
  )

  public static let errorPagesNoInternetTry = NSLocalizedString(
    "ErrorPagesNoInternetTry",
    tableName: "BraveShared",
    bundle: .module,
    value: "It appears you're not online. To fix this, try",
    comment: "Text telling the user to Try: The following list of things"
  )

  public static let errorPagesNoInternetTryItem1 = NSLocalizedString(
    "ErrorPagesNoInternetTryItem1",
    tableName: "BraveShared",
    bundle: .module,
    value: "Checking the network cables, modem, and router",
    comment: "List of things to try when internet is not working"
  )

  public static let errorPagesNoInternetTryItem2 = NSLocalizedString(
    "ErrorPagesNoInternetTryItem2",
    tableName: "BraveShared",
    bundle: .module,
    value: "Reconnecting to Wi-Fi",
    comment: "List of things to try when internet is not working"
  )

  public static let errorPagesAdvancedErrorPinningDetails = NSLocalizedString(
    "ErrorPagesAdvancedErrorPinningDetails",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "%@ normally uses encryption to protect your information. When Brave tried to connect to %@ this time, the website sent back unusual and incorrect credentials. This may happen when an attacker is trying to pretend to be %@, or a Wi-Fi sign-in screen has interrupted the connection. Your information is still secure because Brave stopped the connection before any data was exchanged.<br />You cannot visit %@ right now because the website uses certificate pinning. Network errors and attacks are usually temporary, so this page will probably work later.",
    comment:
      "Additional warning text when clicking the Advanced button on error pages. %@ is a placeholder, do not localize it. Do not localize <br />."
  )
}

// MARK: - Sync
extension Strings {
  /// A menu option available when long pressing on a link which allows you to copy a clean version of the url which strips out some query parameters.
  public static let copyCleanLink = NSLocalizedString(
    "CopyCleanLink",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy Clean Link",
    comment:
      "A menu option available when long pressing on a link which allows you to copy a clean version of the url which strips out some query parameters."
  )

}

extension Strings {
  public static let home = NSLocalizedString(
    "Home",
    tableName: "BraveShared",
    bundle: .module,
    value: "Home",
    comment: ""
  )
  public static let clearingData = NSLocalizedString(
    "ClearData",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clearing Data",
    comment: ""
  )
}

extension Strings {

  public static let newFolder = NSLocalizedString(
    "NewFolder",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Folder",
    comment: "Title for new folder popup"
  )
  public static let enterFolderName = NSLocalizedString(
    "EnterFolderName",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enter folder name",
    comment: "Description for new folder popup"
  )
  public static let edit = NSLocalizedString(
    "Edit",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit",
    comment: ""
  )

  public static let currentlyUsedSearchEngines = NSLocalizedString(
    "CurrentlyUsedSearchEngines",
    tableName: "BraveShared",
    bundle: .module,
    value: "Currently used search engines",
    comment: "Currently usedd search engines section name."
  )
  public static let quickSearchEngines = NSLocalizedString(
    "QuickSearchEngines",
    tableName: "BraveShared",
    bundle: .module,
    value: "Quick-Search Engines",
    comment: "Title for quick-search engines settings section."
  )
  public static let customSearchEngines = NSLocalizedString(
    "CustomSearchEngines",
    tableName: "BraveShared",
    bundle: .module,
    value: "Custom-Search Engines",
    comment: "Title for quick-search engines settings section."
  )
  public static let standardTabSearch = NSLocalizedString(
    "StandardTabSearch",
    tableName: "BraveShared",
    bundle: .module,
    value: "Standard Tab",
    comment: "Open search section of settings"
  )
  public static let privateTabSearch = NSLocalizedString(
    "PrivateTabSearch",
    tableName: "BraveShared",
    bundle: .module,
    value: "Private Tab",
    comment: "Default engine for private search."
  )
  public static let searchEngines = NSLocalizedString(
    "SearchEngines",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Engines",
    comment: "Search engines section of settings"
  )
  public static let settings = NSLocalizedString(
    "Settings",
    tableName: "BraveShared",
    bundle: .module,
    value: "Settings",
    comment: ""
  )
  public static let done = NSLocalizedString(
    "Done",
    tableName: "BraveShared",
    bundle: .module,
    value: "Done",
    comment: ""
  )
  public static let confirm = NSLocalizedString(
    "Confirm",
    tableName: "BraveShared",
    bundle: .module,
    value: "Confirm",
    comment: ""
  )
  public static let privacy = NSLocalizedString(
    "Privacy",
    tableName: "BraveShared",
    bundle: .module,
    value: "Privacy",
    comment: "Settings privacy section title"
  )
  public static let security = NSLocalizedString(
    "Security",
    tableName: "BraveShared",
    bundle: .module,
    value: "Security",
    comment: "Settings security section title"
  )
  public static let saveLogins = NSLocalizedString(
    "SaveLogins",
    tableName: "BraveShared",
    bundle: .module,
    value: "Save Logins",
    comment: "Setting to enable the built-in password manager"
  )
  public static let alwaysRequestDesktopSite = NSLocalizedString(
    "AlwaysRequestDesktopSite",
    tableName: "BraveShared",
    bundle: .module,
    value: "Always Request Desktop Site",
    comment: "Setting to always request the desktop version of a website."
  )
  public static let delete = NSLocalizedString(
    "Delete",
    tableName: "BraveShared",
    bundle: .module,
    value: "Delete",
    comment: ""
  )

  public static let newTab = NSLocalizedString(
    "NewTab",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Tab",
    comment: "New Tab title"
  )
  public static let yes = NSLocalizedString(
    "Yes",
    tableName: "BraveShared",
    bundle: .module,
    comment:
      "For search suggestions prompt. This string should be short so it fits nicely on the prompt row."
  )
  public static let no = NSLocalizedString(
    "No",
    tableName: "BraveShared",
    bundle: .module,
    comment:
      "For search suggestions prompt. This string should be short so it fits nicely on the prompt row."
  )
  public static let openAllBookmarks = NSLocalizedString(
    "OpenAllBookmarks",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open All (%i)",
    comment: "Context menu item for opening all folder bookmarks"
  )

  public static let bookmarkFolder = NSLocalizedString(
    "BookmarkFolder",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bookmark Folder",
    comment: "Bookmark Folder Section Title"
  )
  public static let bookmarkInfo = NSLocalizedString(
    "BookmarkInfo",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bookmark Info",
    comment: "Bookmark Info Section Title"
  )
  public static let name = NSLocalizedString(
    "Name",
    tableName: "BraveShared",
    bundle: .module,
    value: "Name",
    comment: "Bookmark title / Device name"
  )
  public static let URL = NSLocalizedString(
    "URL",
    tableName: "BraveShared",
    bundle: .module,
    value: "URL",
    comment: "Bookmark URL"
  )
  public static let bookmarks = NSLocalizedString(
    "Bookmarks",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bookmarks",
    comment: "title for bookmarks panel"
  )
  public static let today = NSLocalizedString(
    "Today",
    tableName: "BraveShared",
    bundle: .module,
    value: "Today",
    comment: "History tableview section header"
  )
  public static let yesterday = NSLocalizedString(
    "Yesterday",
    tableName: "BraveShared",
    bundle: .module,
    value: "Yesterday",
    comment: "History tableview section header"
  )
  public static let lastWeek = NSLocalizedString(
    "LastWeek",
    tableName: "BraveShared",
    bundle: .module,
    value: "Last week",
    comment: "History tableview section header"
  )
  public static let lastMonth = NSLocalizedString(
    "LastMonth",
    tableName: "BraveShared",
    bundle: .module,
    value: "Last month",
    comment: "History tableview section header"
  )
  public static let earlier = NSLocalizedString(
    "Earlier",
    tableName: "BraveShared",
    bundle: .module,
    value: "Earlier",
    comment: "History tableview section header that indicated history items earlier than last month"
  )
  public static let savedLogins = NSLocalizedString(
    "SavedLogins",
    tableName: "BraveShared",
    bundle: .module,
    value: "Saved Logins",
    comment: "Settings item for clearing passwords and login data"
  )
  public static let downloadedFiles = NSLocalizedString(
    "DownloadedFiles",
    tableName: "BraveShared",
    bundle: .module,
    value: "Downloaded files",
    comment: "Settings item for clearing downloaded files."
  )
  public static let browsingHistory = NSLocalizedString(
    "BrowsingHistory",
    tableName: "BraveShared",
    bundle: .module,
    value: "Browsing History",
    comment: "Settings item for clearing browsing history"
  )
  public static let cache = NSLocalizedString(
    "Cache",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cache",
    comment: "Settings item for clearing the cache"
  )
  public static let cookies = NSLocalizedString(
    "Cookies",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cookies and Site Data",
    comment: "Settings item for clearing cookies and site data"
  )
  public static let findInPage = NSLocalizedString(
    "FindInPage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Find in Page",
    comment: "Share action title"
  )
  public static let searchWithBrave = NSLocalizedString(
    "SearchWithBrave",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search with Brave",
    comment:
      "Title of an action that allows user to perform a one-click web search for selected text"
  )
  public static let forcePaste = NSLocalizedString(
    "ForcePaste",
    tableName: "BraveShared",
    bundle: .module,
    value: "Force Paste",
    comment:
      "A label which when tapped pastes from the users clipboard forcefully (so as to ignore any paste restrictions placed by the website)"
  )
  public static let addToFavorites = NSLocalizedString(
    "AddToFavorites",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add to Favorites",
    comment: "Add to favorites share action."
  )
  public static let createPDF = NSLocalizedString(
    "CreatePDF",
    tableName: "BraveShared",
    bundle: .module,
    value: "Create PDF",
    comment: "Create PDF share action."
  )
  public static let displayCertificate = NSLocalizedString(
    "DisplayCertificate",
    tableName: "BraveShared",
    bundle: .module,
    value: "Security Certificate",
    comment:
      "Button title that when tapped displays a websites HTTPS security certificate information"
  )
  public static let toggleReaderMode = NSLocalizedString(
    "ToggleReaderMode",
    tableName: "BraveShared",
    bundle: .module,
    value: "Toggle Reader Mode",
    comment: "Button title that when tapped toggles the web page in our out of reader mode"
  )

  public static let showBookmarks = NSLocalizedString(
    "ShowBookmarks",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Bookmarks",
    comment: "Button to show the bookmarks list"
  )
  public static let showHistory = NSLocalizedString(
    "ShowHistory",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show History",
    comment: "Button to show the history list"
  )
  public static let editBookmark = NSLocalizedString(
    "EditBookmark",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit Bookmark",
    comment: "Button to edit a bookmark"
  )
  public static let editFavorite = NSLocalizedString(
    "EditFavorite",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit Favorite",
    comment: "Button to edit a favorite"
  )
  public static let removeFavorite = NSLocalizedString(
    "RemoveFavorite",
    tableName: "BraveShared",
    bundle: .module,
    value: "Remove Favorite",
    comment: "Button to remove a favorite"
  )

  public static let deleteBookmarksFolderAlertTitle = NSLocalizedString(
    "DeleteBookmarksFolderAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Delete Folder?",
    comment: "Title for the alert shown when the user tries to delete a bookmarks folder"
  )
  public static let deleteBookmarksFolderAlertMessage = NSLocalizedString(
    "DeleteBookmarksFolderAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "This will delete all folders and bookmarks inside. Are you sure you want to continue?",
    comment: "Message for the alert shown when the user tries to delete a bookmarks folder"
  )
  public static let yesDeleteButtonTitle = NSLocalizedString(
    "YesDeleteButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Yes, Delete",
    comment: "Button title to confirm the deletion of a bookmarks folder"
  )
  public static let close = NSLocalizedString(
    "Close",
    tableName: "BraveShared",
    bundle: .module,
    value: "Close",
    comment: "Button title to close a menu."
  )
  public static let openWebsite = NSLocalizedString(
    "OpenWebsite",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Website",
    comment: "Button title to that opens a website."
  )
  public static let viewOn = NSLocalizedString(
    "ViewOn",
    tableName: "BraveShared",
    bundle: .module,
    value: "View on %@",
    comment:
      "Label that says where to view an item. '%@' is a placeholder and will include things like 'Instagram', 'unsplash'. The full label will look like 'View  on Instagram'."
  )
  public static let photoBy = NSLocalizedString(
    "PhotoBy",
    tableName: "BraveShared",
    bundle: .module,
    value: "Photo by %@",
    comment:
      "Label that says who took a photograph that will be displayed to the user. '%@' is a placeholder and will include be a specific person's name, example 'Bill Gates'."
  )

  public static let features = NSLocalizedString(
    "Features",
    tableName: "BraveShared",
    bundle: .module,
    value: "Features",
    comment: ""
  )

  public static let braveShieldsAndPrivacy = NSLocalizedString(
    "BraveShieldsAndPrivacy",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Shields & Privacy",
    comment: ""
  )
  public static let braveShieldsAndPrivacySettingsTitle = NSLocalizedString(
    "BraveShieldsAndPrivacyTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shields & Privacy",
    comment: ""
  )
  public static let bookmarksImportAction = NSLocalizedString(
    "bookmarksImportAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Import Bookmarks",
    comment: "Action to import bookmarks from a file."
  )
  public static let bookmarksExportAction = NSLocalizedString(
    "bookmarksExportAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Export Bookmarks",
    comment: "Action to export bookmarks to another device."
  )
}

extension Strings {
  public static let blockPopups = NSLocalizedString(
    "BlockPopups",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block Popups",
    comment: "Setting to enable popup blocking"
  )
  public static let followUniversalLinks = NSLocalizedString(
    "FollowUniversalLinks",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow universal links to open in external apps",
    comment: "Setting to follow universal links"
  )
  public static let mediaAutoBackgrounding = NSLocalizedString(
    "MediaAutoBackgrounding",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable Background Audio",
    comment: "Setting to allow media to play in the background"
  )
  public static let youtubeMediaQualitySettingsTitle = NSLocalizedString(
    "YoutubeMediaQualitySettingsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "YouTube Playback Quality",
    comment: "Title for youtube playback quality settings"
  )
  public static let youtubeMediaQuality = NSLocalizedString(
    "YoutubeMediaQuality",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable High-Quality YouTube Videos",
    comment: "Setting to enable high quality youtube videos"
  )
  public static let youtubeMediaQualityDetails = NSLocalizedString(
    "YoutubeMediaQualityDetails",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Enabling this experimental feature will playback video at the maximum quality available and will use additional data.",
    comment: "Description of setting to enable high quality youtube videos"
  )
  public static let youtubeMediaQualityOff = NSLocalizedString(
    "YoutubeMediaQualityOff",
    tableName: "BraveShared",
    bundle: .module,
    value: "Off",
    comment: "Setting that disables highest quality video playback"
  )
  public static let youtubeMediaQualityWifi = NSLocalizedString(
    "YoutubeMediaQualityWifi",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow over Wi-Fi",
    comment: "Setting that enables high quality playback on wifi only"
  )
  public static let youtubeMediaQualityOn = NSLocalizedString(
    "YoutubeMediaQualityOn",
    tableName: "BraveShared",
    bundle: .module,
    value: "On",
    comment: "Setting that enables high quality playback always"
  )
  public static let showTabsBar = NSLocalizedString(
    "ShowTabsBar",
    tableName: "BraveShared",
    bundle: .module,
    value: "Tabs Bar",
    comment: "Setting to show/hide the tabs bar"
  )
  public static let privateBrowsingOnly = NSLocalizedString(
    "PrivateBrowsingOnly",
    tableName: "BraveShared",
    bundle: .module,
    value: "Private Browsing Only",
    comment: "Setting to keep app in private mode"
  )
  public static let persistentPrivateBrowsingAlertTitle = NSLocalizedString(
    "PersistentPrivateBrowsingAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Restore Private Tabs",
    comment: "Persistent private browsing alert title to existing users"
  )
  public static let persistentPrivateBrowsingAlertMessage = NSLocalizedString(
    "PersistentPrivateBrowsingAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allows Brave to restore private browsing tabs, even if you close / re-open the app",
    comment: "Persistent private browsing alert message to existing users"
  )
  public static let persistentPrivateBrowsing = NSLocalizedString(
    "PersistentPrivateBrowsing",
    tableName: "BraveShared",
    bundle: .module,
    value: "Persistent Private Browsing",
    comment: "Setting to allow the app to restore private browsing tabs"
  )
  public static let shieldsDefaults = NSLocalizedString(
    "ShieldsDefaults",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Shields Global Defaults",
    comment: "Section title for adbblock, tracking protection, HTTPS-E, and cookies"
  )
  public static let shieldsDefaultsFooter = NSLocalizedString(
    "ShieldsDefaultsFooter",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "These are the default Shields settings for new sites. Changing these won't affect your existing per-site settings.",
    comment: "Section footer for global shields defaults"
  )
  public static let HTTPSEverywhere = NSLocalizedString(
    "HTTPSEverywhere",
    tableName: "BraveShared",
    bundle: .module,
    value: "Upgrade Connections to HTTPS",
    comment: ""
  )
  public static let HTTPSEverywhereDescription = NSLocalizedString(
    "HTTPSEverywhereDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Opens sites using secure HTTPS instead of HTTP when possible.",
    comment: ""
  )
  public static let googleSafeBrowsing = NSLocalizedString(
    "GoogleSafeBrowsing",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block Dangerous Websites",
    comment: ""
  )
  public static let googleSafeBrowsingUsingWebKitDescription = NSLocalizedString(
    "GoogleSafeBrowsingUsingWebKitDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Safe Browsing protects against websites which are known to be dangerous. [Learn More](%@)",
    comment: ""
  )
  public static let screenTimeSetting = NSLocalizedString(
    "settings.privacy.screenTimeSetting",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable Screen Time",
    comment: ""
  )
  public static let screenTimeSettingDescription = NSLocalizedString(
    "settings.privacy.screenTimeSettingDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "See which websites you spend the most time on, and set browsing limits. [Learn More](%@)",
    comment: ""
  )
  public static let blockScripts = NSLocalizedString(
    "BlockScripts",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block Scripts",
    comment: ""
  )
  public static let blockScriptsDescription = NSLocalizedString(
    "BlockScriptsDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Blocks JavaScript (may break sites).",
    comment: ""
  )
  public static let blockCookiesDescription = NSLocalizedString(
    "BlockCookiesDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Prevents websites from storing information about your previous visits.",
    comment: ""
  )
  public static let fingerprintingProtection = NSLocalizedString(
    "FingerprintingProtection",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block Fingerprinting",
    comment: ""
  )
  public static let fingerprintingProtectionDescription = NSLocalizedString(
    "FingerprintingProtectionDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Makes it harder for sites to recognize your device's distinctive characteristics. ",
    comment: ""
  )
  public static let autoRedirectAMPPages = NSLocalizedString(
    "AutoRedirectAMPPages",
    tableName: "BraveShared",
    bundle: .module,
    value: "Auto-Redirect AMP Pages",
    comment:
      "This is a title for a setting toggle that enables/disables auto-redirect of Google's AMP (Accelerated Mobile Page) pages to the original (non-AMP) pages. The text 'AMP' is not to be translated."
  )
  public static let autoRedirectAMPPagesDescription = NSLocalizedString(
    "AutoRedirectAMPPagesDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Always visit original (non-AMP) page URLs, instead of Google's Accelerated Mobile Page versions",
    comment:
      "This is a description for a setting toggle that enables/disables auto-redirect of Google's AMP (Accelerated Mobile Page) pages to the original (non-AMP) pages. The text 'AMP' and 'Accelerated Mobile Page' is not to be translated."
  )
  public static let autoRedirectTrackingURLs = NSLocalizedString(
    "AutoRedirectTrackingURLs",
    tableName: "BraveShared",
    bundle: .module,
    value: "Auto-Redirect Tracking URLs",
    comment:
      "This is a title for a setting toggle that enables/disables auto-redirection of tracking pages (Debouncing). Debouncing skips certain intermediate tracker pages and goes directly to the target without the intermediate tracker page."
  )
  public static let autoRedirectTrackingURLsDescription = NSLocalizedString(
    "AutoRedirectTrackingURLsDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable support for bypassing top-level redirect tracking URLs",
    comment:
      "This is a description for a setting toggle that enables/disables auto-redirect of tracking URLs (i.e. Debouncing)."
  )
  public static let support = NSLocalizedString(
    "Support",
    tableName: "BraveShared",
    bundle: .module,
    value: "Support",
    comment: "Support section title"
  )
  public static let about = NSLocalizedString(
    "About",
    tableName: "BraveShared",
    bundle: .module,
    value: "About",
    comment: "About settings section title"
  )
  public static let versionTemplate = NSLocalizedString(
    "VersionTemplate",
    tableName: "BraveShared",
    bundle: .module,
    value: "Version %@ (%@)",
    comment: "Version number of Brave shown in settings"
  )
  public static let deviceTemplate = NSLocalizedString(
    "DeviceTemplate",
    tableName: "BraveShared",
    bundle: .module,
    value: "Device %@ (%@)",
    comment: "Current device model and iOS version copied to clipboard."
  )
  public static let copyAppInfoToClipboard = NSLocalizedString(
    "CopyAppInfoToClipboard",
    tableName: "BraveShared",
    bundle: .module,
    value: "Copy app info to clipboard.",
    comment: "Copy app info to clipboard action sheet action."
  )
  public static let copyAppSizeInfoToClipboard =
    NSLocalizedString(
      "copyAppSizeInfoToClipboard",
      tableName: "BraveShared",
      bundle: .module,
      value: "Copy app size info to clipboard.",
      comment: "Copy app info to clipboard action sheet action."
    )
  public static let copyTabsDebugToClipboard =
    NSLocalizedString(
      "copyTabsDebugToClipboard",
      tableName: "BraveShared",
      bundle: .module,
      value: "Copy Tabs info to clipboard.",
      comment: "Copy Tabs info to clipboard action sheet action."
    )
  public static let blockThirdPartyCookies = NSLocalizedString(
    "Block3rdPartyCookies",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block 3rd party cookies",
    comment: "cookie settings option"
  )
  public static let blockAllCookies = NSLocalizedString(
    "BlockAllCookies",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block all Cookies",
    comment: "Title for setting to block all website cookies."
  )
  public static let blockAllCookiesAction = NSLocalizedString(
    "BlockAllCookiesAction",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block All",
    comment: "block all cookies settings action title"
  )
  public static let blockAllCookiesAlertInfo = NSLocalizedString(
    "BlockAllCookiesAlertInfo",
    tableName: "BraveShared",
    bundle: .module,
    value: "Blocking all Cookies will prevent some websites from working correctly.",
    comment: "Information displayed to user when block all cookie is enabled."
  )
  public static let blockAllCookiesAlertTitle = NSLocalizedString(
    "BlockAllCookiesAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block all Cookies?",
    comment: "Title of alert displayed to user when block all cookie is enabled."
  )
  public static let blockAllCookiesFailedAlertMsg = NSLocalizedString(
    "BlockAllCookiesFailedAlertMsg",
    tableName: "BraveShared",
    bundle: .module,
    value: "Failed to set the preference. Please try again.",
    comment: "Message of alert displayed to user when block all cookie operation fails"
  )
  public static let dontBlockCookies = NSLocalizedString(
    "DontBlockCookies",
    tableName: "BraveShared",
    bundle: .module,
    value: "Don't block cookies",
    comment: "cookie settings option"
  )
  public static let cookieControl = NSLocalizedString(
    "CookieControl",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cookie Control",
    comment: "Cookie settings option title"
  )
  public static let neverShow = NSLocalizedString(
    "NeverShow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Off",
    comment: "Setting preference to always hide the browser tabs bar."
  )
  public static let alwaysShow = NSLocalizedString(
    "AlwaysShow",
    tableName: "BraveShared",
    bundle: .module,
    value: "On",
    comment: "Setting preference to always show the browser tabs bar."
  )
  public static let showInLandscapeOnly = NSLocalizedString(
    "ShowInLandscapeOnly",
    tableName: "BraveShared",
    bundle: .module,
    value: "Landscape Only",
    comment:
      "Setting preference to only show the browser tabs bar when the device is in the landscape orientation."
  )
  public static let rateBrave = NSLocalizedString(
    "RateBrave",
    tableName: "BraveShared",
    bundle: .module,
    value: "Rate Brave",
    comment: "Open the App Store to rate Brave."
  )
  public static let reportABug = NSLocalizedString(
    "ReportABug",
    tableName: "BraveShared",
    bundle: .module,
    value: "Report a Bug",
    comment: "Providers the user an email page where they can submit a but report."
  )
  public static let privacyPolicy = NSLocalizedString(
    "PrivacyPolicy",
    tableName: "BraveShared",
    bundle: .module,
    value: "Privacy Policy",
    comment: "Show Brave Browser Privacy Policy page from the Privacy section in the settings."
  )
  public static let termsOfUse = NSLocalizedString(
    "TermsOfUse",
    tableName: "BraveShared",
    bundle: .module,
    value: "Terms of Use",
    comment: "Show Brave Browser TOS page from the Privacy section in the settings."
  )
  public static let privateTabBody = NSLocalizedString(
    "PrivateTabBody",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Private Tabs aren’t saved in Brave, but they don’t make you anonymous online. Sites you visit in a private tab won’t show up in your history and their cookies always vanish when you close them — there won’t be any trace of them left in Brave. However, downloads will be saved.\nYour mobile carrier (or the owner of the Wi-Fi network or VPN you’re connected to) can see which sites you visit and those sites will learn your public IP address, even in Private Tabs.",
    comment: "Private tab details"
  )
  public static let privateTabDetails = NSLocalizedString(
    "PrivateTabDetails",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Using Private Tabs only changes what Brave does on your device, it doesn't change anyone else's behavior.\n\nSites always learn your IP address when you visit them. From this, they can often guess roughly where you are — typically your city. Sometimes that location guess can be much more specific. Sites also know everything you specifically tell them, such as search terms. If you log into a site, they'll know you're the owner of that account. You'll still be logged out when you close the Private Tabs because Brave will throw away the cookie which keeps you logged in.\n\nWhoever connects you to the Internet (your ISP) can see all of your network activity. Often, this is your mobile carrier. If you're connected to a Wi-Fi network, this is the owner of that network, and if you're using a VPN, then it's whoever runs that VPN. Your ISP can see which sites you visit as you visit them. If those sites use HTTPS, they can't make much more than an educated guess about what you do on those sites. But if a site only uses HTTP then your ISP can see everything: your search terms, which pages you read, and which links you follow.\n\nIf an employer manages your device, they might also keep track of what you do with it. Using Private Tabs probably won't stop them from knowing which sites you've visited. Someone else with access to your device could also have installed software which monitors your activity, and Private Tabs won't protect you from this either.",
    comment: "Private tab detail text"
  )
  public static let privateTabLink = NSLocalizedString(
    "PrivateTabLink",
    tableName: "BraveShared",
    bundle: .module,
    value: "Learn about Private Tabs.",
    comment: "Private tab information link"
  )
  public static let privateBrowsingOnlyWarning = NSLocalizedString(
    "PrivateBrowsingOnlyWarning",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Private Browsing Only mode will close all your current tabs and log you out of all sites.",
    comment:
      "When 'Private Browsing Only' is enabled, we need to alert the user of their normal tabs being destroyed"
  )
  public static let bravePanel = NSLocalizedString(
    "BravePanel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Panel",
    comment: "Button to show the brave panel"
  )
  public static let rewardsPanel = NSLocalizedString(
    "RewardsPanel",
    tableName: "BraveShared",
    bundle: .module,
    value: "Rewards Panel",
    comment: "Button to show the rewards panel"
  )
  public static let individualControls = NSLocalizedString(
    "IndividualControls",
    tableName: "BraveShared",
    bundle: .module,
    value: "Individual Controls",
    comment: "title for per-site shield toggles"
  )
  public static let blockingMonitor = NSLocalizedString(
    "BlockingMonitor",
    tableName: "BraveShared",
    bundle: .module,
    value: "Blocking Monitor",
    comment: "title for section showing page blocking statistics"
  )
  public static let siteShieldSettings = NSLocalizedString(
    "SiteShieldSettings",
    tableName: "BraveShared",
    bundle: .module,
    value: "Shields",
    comment: "Brave panel topmost title"
  )
  public static let adsAndTrackers = NSLocalizedString(
    "AdsAndTrackers",
    tableName: "BraveShared",
    bundle: .module,
    value: "Ads and Trackers",
    comment: "individual blocking statistic title"
  )
  public static let HTTPSUpgrades = NSLocalizedString(
    "HTTPSUpgrades",
    tableName: "BraveShared",
    bundle: .module,
    value: "HTTPS Upgrades",
    comment: "individual blocking statistic title"
  )
  public static let scriptsBlocked = NSLocalizedString(
    "ScriptsBlocked",
    tableName: "BraveShared",
    bundle: .module,
    value: "Scripts Blocked",
    comment: "individual blocking statistic title"
  )
  public static let fingerprintingMethods = NSLocalizedString(
    "FingerprintingMethods",
    tableName: "BraveShared",
    bundle: .module,
    value: "Fingerprinting Methods",
    comment: "individual blocking statistic title"
  )
  public static let shieldsOverview = NSLocalizedString(
    "ShieldsOverview",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Site Shields allow you to control when ads and trackers are blocked for each site that you visit. If you prefer to see ads on a specific site, you can enable them here.",
    comment: "shields overview message"
  )
  public static let shieldsOverviewFooter = NSLocalizedString(
    "ShieldsOverviewFooter",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Note: Some sites may require scripts to work properly so this shield is turned off by default.",
    comment: "shields overview footer message"
  )
  public static let useRegionalAdblock = NSLocalizedString(
    "UseRegionalAdblock",
    tableName: "BraveShared",
    bundle: .module,
    value: "Use regional adblock",
    comment:
      "Setting to allow user in non-english locale to use adblock rules specifc to their language"
  )
  public static let newFolderDefaultName = NSLocalizedString(
    "NewFolderDefaultName",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Folder",
    comment: "Default name for creating a new folder."
  )
  public static let newBookmarkDefaultName = NSLocalizedString(
    "NewBookmarkDefaultName",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Bookmark",
    comment: "Default name for creating a new bookmark."
  )
  public static let bookmarkTitlePlaceholderText = NSLocalizedString(
    "BookmarkTitlePlaceholderText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Name",
    comment: "Placeholder text for adding or editing a bookmark"
  )
  public static let bookmarkUrlPlaceholderText = NSLocalizedString(
    "BookmarkUrlPlaceholderText",
    tableName: "BraveShared",
    bundle: .module,
    value: "Address",
    comment: "Placeholder text for adding or editing a bookmark"
  )
  public static let favoritesLocationFooterText = NSLocalizedString(
    "FavoritesLocationFooterText",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Favorites are located on your home screen. These bookmarks are not synchronized with other devices.",
    comment: "Footer text when user selects to save to favorites when editing a bookmark"
  )
  public static let bookmarkRootLevelCellTitle = NSLocalizedString(
    "BookmarkRootLevelCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bookmarks",
    comment: "Title for root level bookmarks cell"
  )
  public static let favoritesRootLevelCellTitle = NSLocalizedString(
    "FavoritesRootLevelCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Favorites",
    comment: "Title for favorites cell"
  )
  public static let addFolderActionCellTitle = NSLocalizedString(
    "AddFolderActionCellTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "New folder",
    comment: "Cell title for add folder action"
  )
  public static let editBookmarkTableLocationHeader = NSLocalizedString(
    "EditBookmarkTableLocationHeader",
    tableName: "BraveShared",
    bundle: .module,
    value: "Location",
    comment: "Header title for bookmark save location"
  )
  public static let newBookmarkTitle = NSLocalizedString(
    "NewBookmarkTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "New bookmark",
    comment: "Title for adding new bookmark"
  )
  public static let newFolderTitle = NSLocalizedString(
    "NewFolderTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "New folder",
    comment: "Title for adding new folder"
  )
  public static let editBookmarkTitle = NSLocalizedString(
    "EditBookmarkTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit bookmark",
    comment: "Title for editing a bookmark"
  )
  public static let editFavoriteTitle = NSLocalizedString(
    "EditFavoriteTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit favorite",
    comment: "Title for editing a favorite bookmark"
  )
  public static let editFolderTitle = NSLocalizedString(
    "EditFolderTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Edit folder",
    comment: "Title for editing a folder"
  )
  public static let historyScreenTitle = NSLocalizedString(
    "HistoryScreenTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "History",
    comment: "Title for history screen"
  )
  public static let bookmarksMenuItem = NSLocalizedString(
    "BookmarksMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Bookmarks",
    comment: "Title for bookmarks menu item"
  )
  public static let historyMenuItem = NSLocalizedString(
    "HistortMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "History",
    comment: "Title for history menu item"
  )
  public static let settingsMenuItem = NSLocalizedString(
    "SettingsMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Settings",
    comment: "Title for settings menu item"
  )
  public static let leoMenuItem = NSLocalizedString(
    "LeoMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Leo",
    comment:
      "Title for Brave Leo menu item. Brave Leo is the name of a product and should not be translated"
  )
  public static let passwordsMenuItem = NSLocalizedString(
    "PasswordsMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Passwords",
    comment: "Title for passwords menu item"
  )
  public static let addToMenuItem = NSLocalizedString(
    "AddToMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Add Bookmark",
    comment: "Title for the button to add a new website bookmark."
  )
  public static let openTabsMenuItem = NSLocalizedString(
    "OpenTabsMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Open Tabs",
    comment: "Title for open tabs menu item"
  )
  public static let shareWithMenuItem = NSLocalizedString(
    "ShareWithMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Share with...",
    comment: "Title for sharing url menu item"
  )
  public static let openExternalAppURLGenericTitle = NSLocalizedString(
    "ExternalAppURLAlertTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow link to switch apps?",
    comment: "Allow link to switch apps?"
  )
  public static let openExternalAppURLTitle = NSLocalizedString(
    "openExternalAppURLTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow %@ to switch apps?",
    comment:
      "This will be used to indicate which website is calling the action. %@ will replace the name of the website for instance: 'Allow google.com to switch apps?'"
  )
  public static let openExternalAppURLMessage = NSLocalizedString(
    "ExternalAppURLAlertMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "%@ will launch an external application",
    comment: "%@ will launch an external application"
  )
  public static let openExternalAppURLHost = NSLocalizedString(
    "openExternalAppURLHost",
    tableName: "BraveShared",
    bundle: .module,
    value: "%@ says:",
    comment:
      "This will be used to indicate which website is calling the action. %s will replace the name of the website for instance: 'google.com says:'"
  )
  public static let openExternalAppURLAllow = NSLocalizedString(
    "ExternalAppURLAllow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow",
    comment: "Allow Brave to open the external app URL"
  )
  public static let openExternalAppURLDontAllow = NSLocalizedString(
    "ExternalAppURLDontAllow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Don't Allow",
    comment: "Don't allow Brave to open the external app URL"
  )
  public static let requestCameraPermissionPrompt = NSLocalizedString(
    "requestCameraPermissionPrompt",
    tableName: "BraveShared",
    bundle: .module,
    value: "\"%@\" Would Like to Access the Camera",
    comment:
      "A title that appears on a permissions dialog on whether or not the user wants to allow a web page to access their device's camera. %@ will be replaced with the websites address (for example: \"talk.brave.com\")"
  )
  public static let requestMicrophonePermissionPrompt = NSLocalizedString(
    "requestMicrophonePermissionPrompt",
    tableName: "BraveShared",
    bundle: .module,
    value: "\"%@\" Would Like to Access the Microphone",
    comment:
      "A title that appears on a permissions dialog on whether or not the user wants to allow a web page to access their device's microphone. %@ will be replaced with the websites address (for example: \"talk.brave.com\")"
  )
  public static let requestCameraAndMicrophonePermissionPrompt = NSLocalizedString(
    "requestCameraAndMicrophonePermissionPrompt",
    tableName: "BraveShared",
    bundle: .module,
    value: "\"%@\" Would Like to Access the Camera and Microphone",
    comment:
      "A title that appears on a permissions dialog on whether or not the user wants to allow a web page to access their device's camera & microphone. %@ will be replaced with the websites address (for example: \"talk.brave.com\")"
  )
  public static let requestCaptureDevicePermissionPrompt = NSLocalizedString(
    "requestCaptureDevicePermissionPrompt",
    tableName: "BraveShared",
    bundle: .module,
    value: "\"%@\" Would Like to Access a Media Capture Device",
    comment:
      "A title that appears on a permissions dialog on whether or not the user wants to allow a web page to access some media capture device such as a camera or microphone. %@ will be replaced with the websites address (for example: \"talk.brave.com\")"
  )
  public static let requestCaptureDevicePermissionAllowButtonTitle = NSLocalizedString(
    "requestCaptureDevicePermissionAllowButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow",
    comment:
      "A button shown in a permission dialog that will grant the website the ability to use the device's camera or microphone"
  )
  public static let downloadsMenuItem = NSLocalizedString(
    "DownloadsMenuItem",
    tableName: "BraveShared",
    bundle: .module,
    value: "Downloads",
    comment: "Title for downloads menu item"
  )
  public static let downloadsPanelEmptyStateTitle = NSLocalizedString(
    "DownloadsPanelEmptyStateTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Downloaded files will show up here.",
    comment:
      "Title for when a user has nothing downloaded onto their device, and the list is empty."
  )

  // MARK: - Themes

  public static let themesDisplayBrightness = NSLocalizedString(
    "ThemesDisplayBrightness",
    tableName: "BraveShared",
    bundle: .module,
    value: "Appearance",
    comment:
      "Setting to choose the user interface theme for normal browsing mode, contains choices like 'light' or 'dark' themes"
  )
  public static let themesDisplayBrightnessFooter = NSLocalizedString(
    "ThemesDisplayBrightnessFooter",
    tableName: "BraveShared",
    bundle: .module,
    value: "These settings are not applied in private browsing mode.",
    comment:
      "Text specifying that the above setting does not impact the user interface while they user is in private browsing mode."
  )
  public static let themesAutomaticOption = NSLocalizedString(
    "ThemesAutomaticOption",
    tableName: "BraveShared",
    bundle: .module,
    value: "Automatic",
    comment: "Selection to automatically color/style the user interface."
  )
  public static let themesLightOption = NSLocalizedString(
    "ThemesLightOption",
    tableName: "BraveShared",
    bundle: .module,
    value: "Light",
    comment: "Selection to color/style the user interface with a light theme."
  )
  public static let themesDarkOption = NSLocalizedString(
    "ThemesDarkOption",
    tableName: "BraveShared",
    bundle: .module,
    value: "Dark",
    comment: "Selection to color/style the user interface with a dark theme"
  )
  public static let themesSettings =
    NSLocalizedString(
      "themesSettings",
      tableName: "BraveShared",
      bundle: .module,
      value: "Themes",
      comment: "Name for app theme settings"
    )
  public static let defaultThemeName =
    NSLocalizedString(
      "defaultThemeName",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave default",
      comment: "Name for default Brave theme."
    )
  public static let themeQRCodeShareTitle =
    NSLocalizedString(
      "themeQRCodeShareTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Share Brave with your friends!",
      comment: "Title for QR popup encouraging users to share the code with their friends."
    )
  public static let themeQRCodeShareButton =
    NSLocalizedString(
      "themeQRCodeShareButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "Share...",
      comment: "Text for button to share referral's QR code."
    )
}

// MARK: - Quick Actions
extension Strings {
  public static let quickActionNewTab = NSLocalizedString(
    "ShortcutItemTitleNewTab",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Tab",
    comment: "Quick Action for 3D-Touch on the Application Icon"
  )
  public static let quickActionNewPrivateTab = NSLocalizedString(
    "ShortcutItemTitleNewPrivateTab",
    tableName: "BraveShared",
    bundle: .module,
    value: "New Private Tab",
    comment: "Quick Action for 3D-Touch on the Application Icon"
  )
  public static let quickActionScanQRCode = NSLocalizedString(
    "ShortcutItemTitleQRCode",
    tableName: "BraveShared",
    bundle: .module,
    value: "Scan QR Code",
    comment: "Quick Action for 3D-Touch on the Application Icon"
  )
}

// MARK: - Onboarding
extension Strings {
  public static let OBContinueButton = NSLocalizedString(
    "OnboardingContinueButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Continue",
    comment: "Continue button to navigate to next onboarding screen."
  )
  public static let OBSkipButton = NSLocalizedString(
    "OnboardingSkipButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Skip",
    comment: "Skip button to skip onboarding and start using the app."
  )
  public static let OBSaveButton = NSLocalizedString(
    "OBSaveButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Save",
    comment: "Save button to save current selection"
  )
  public static let OBFinishButton = NSLocalizedString(
    "OBFinishButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Start browsing",
    comment: "Button to finish onboarding and start using the app."
  )
  public static let OBJoinButton = NSLocalizedString(
    "OBJoinButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Join",
    comment: "Button to join Brave Rewards."
  )
  public static let OBTurnOnButton = NSLocalizedString(
    "OBTurnOnButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Start",
    comment: "Button to show Brave Rewards."
  )
  public static let OBShowMeButton = NSLocalizedString(
    "OBShowMeButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show Me",
    comment: "Button to show the Brave Rewards Ads."
  )
  public static let OBDidntSeeAdButton = NSLocalizedString(
    "OBDidntSeeAdButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "I didn't see an ad",
    comment: "Button to show information on how to enable ads"
  )
  public static let OBSearchEngineTitle = NSLocalizedString(
    "OBSearchEngineTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Welcome to Brave Browser",
    comment: "Title for search engine onboarding screen"
  )
  public static let OBSearchEngineDetail = NSLocalizedString(
    "OBSearchEngineDetail",
    tableName: "BraveShared",
    bundle: .module,
    value: "Select your default search engine",
    comment: "Detail text for search engine onboarding screen"
  )
  public static let OBShieldsTitle = NSLocalizedString(
    "OBShieldsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Shields",
    comment: "Title for shields onboarding screen"
  )
  public static let OBShieldsDetail = NSLocalizedString(
    "OBShieldsDetail",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Block privacy-invading trackers so you can browse without being followed around the web",
    comment: "Detail text for shields onboarding screen"
  )
  public static let OBRewardsTitle = NSLocalizedString(
    "OBRewardsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Rewards",
    comment: "Title for rewards onboarding screen"
  )
  public static let OBAdsOptInTitle = NSLocalizedString(
    "OBAdsOptInTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Ads is here!",
    comment: "Title when opting into brave Ads when region becomes available"
  )
  public static let OBAdsOptInMessage = NSLocalizedString(
    "OBAdsOptInMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Earn tokens and reward creators for great content while you browse.",
    comment: "Message when opting into brave Ads when region becomes available"
  )
  public static let OBAdsOptInMessageJapan = NSLocalizedString(
    "OBAdsOptInMessageJapan",
    tableName: "BraveShared",
    bundle: .module,
    value: "Earn points and reward creators for great content while you browse.",
    comment: "Message when opting into brave Ads when region becomes available"
  )
  public static let OBRewardsDetailInAdRegion = NSLocalizedString(
    "OBRewardsDetailInAdRegion",
    tableName: "BraveShared",
    bundle: .module,
    value: "Earn tokens and reward creators for great content while you browse.",
    comment: "Detail text for rewards onboarding screen"
  )
  public static let OBRewardsDetail = NSLocalizedString(
    "OBRewardsDetail",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Opting into Brave Private Ads supports publishers and content creators with every ad viewed.",
    comment: "Detail text for rewards onboarding screen"
  )
  public static let OBRewardsAgreementTitle = NSLocalizedString(
    "OBRewardsAgreementTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave Rewards",
    comment: "Title for rewards agreement onboarding screen"
  )
  public static let OBRewardsAgreementDetail = NSLocalizedString(
    "OBRewardsAgreementDetail",
    tableName: "BraveShared",
    bundle: .module,
    value: "By tapping Yes, you agree to the",
    comment: "Detail text for rewards agreement onboarding screen"
  )
  public static let OBRewardsAgreementDetailLink = NSLocalizedString(
    "OBRewardsAgreementDetailLink",
    tableName: "BraveShared",
    bundle: .module,
    value: "Terms of Service",
    comment: "Detail text for rewards agreement onboarding screen"
  )
  public static let OBRewardsPrivacyPolicyDetailLink = NSLocalizedString(
    "OBRewardsPrivacyPolicyDetailLink",
    tableName: "BraveShared",
    bundle: .module,
    value: "Privacy Policy",
    comment: "Detail text for rewards agreement onboarding screen"
  )
  public static let OBRewardsAgreementDetailsAnd = NSLocalizedString(
    "OBRewardsAgreementDetailsAnd",
    tableName: "BraveShared",
    bundle: .module,
    value: "and",
    comment: "Detail text for rewards agreement onboarding screen"
  )
  public static let OBAdsTitle = NSLocalizedString(
    "OBAdsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Brave will show your first ad in",
    comment: "Title for ads onboarding screen"
  )
  public static let OBCompleteTitle = NSLocalizedString(
    "OBCompleteTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Now you're ready to go.",
    comment: "Title for when the user completes onboarding"
  )
  public static let OBErrorTitle = NSLocalizedString(
    "OBErrorTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Sorry",
    comment: "A generic error title for onboarding"
  )
  public static let OBErrorDetails = NSLocalizedString(
    "OBErrorDetails",
    tableName: "BraveShared",
    bundle: .module,
    value: "Something went wrong while creating your wallet. Please try again",
    comment: "A generic error body for onboarding"
  )
  public static let OBErrorOkay = NSLocalizedString(
    "OBErrorOkay",
    tableName: "BraveShared",
    bundle: .module,
    value: "Okay",
    comment: ""
  )
  public static let OBPrivacyConsentTitle = NSLocalizedString(
    "OBPrivacyConsentTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Anonymous referral code check",
    comment: ""
  )
  public static let OBPrivacyConsentDetail = NSLocalizedString(
    "OBPrivacyConsentDetail",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "You may have downloaded Brave in support of your referrer. To detect your referrer, Brave performs a one-time check of your clipboard for the matching referral code. This check is limited to the code only and no other personal data will be transmitted.  If you opt out, your referrer won’t receive rewards from Brave.",
    comment: ""
  )
  public static let OBPrivacyConsentClipboardPermission = NSLocalizedString(
    "OBPrivacyConsentClipboardPermission",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow Brave to check my clipboard for a matching referral code",
    comment: ""
  )
  public static let OBPrivacyConsentYesButton = NSLocalizedString(
    "OBPrivacyConsentYesButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Allow one-time clipboard check",
    comment: ""
  )
  public static let OBPrivacyConsentNoButton = NSLocalizedString(
    "OBPrivacyConsentNoButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Do not allow this check",
    comment: ""
  )
}

// MARK: - Bookmark restoration
extension Strings {
  public static let restoredBookmarksFolderName = NSLocalizedString(
    "RestoredBookmarksFolderName",
    tableName: "BraveShared",
    bundle: .module,
    value: "Restored Bookmarks",
    comment: "Name of folder where restored bookmarks are retrieved"
  )
  public static let restoredFavoritesFolderName = NSLocalizedString(
    "RestoredFavoritesFolderName",
    tableName: "BraveShared",
    bundle: .module,
    value: "Restored Favorites",
    comment: "Name of folder where restored favorites are retrieved"
  )
}

// MARK: - User Wallet
extension Strings {
  public static let userWalletCloseButtonTitle = NSLocalizedString(
    "UserWalletCloseButtonTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Close",
    comment: ""
  )
  public static let userWalletGenericErrorTitle = NSLocalizedString(
    "UserWalletGenericErrorTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Sorry, something went wrong",
    comment: ""
  )
  public static let userWalletGenericErrorMessage = NSLocalizedString(
    "UserWalletGenericErrorMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "There was a problem logging into your Uphold account. Please try again",
    comment: ""
  )
}

// MARK: - New tab page
extension Strings {
  public struct NTP {
    public static let turnRewardsTos =
      NSLocalizedString(
        "ntp.turnRewardsTos",
        tableName: "BraveShared",
        bundle: .module,
        value: "By turning on Rewards, you agree to the %@.",
        comment:
          "The placeholder says 'Terms of Service'. So full sentence goes like: 'By turning Rewards, you agree to the Terms of Service'."
      )
    public static let sponsoredImageDescription =
      NSLocalizedString(
        "ntp.sponsoredImageDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "You’re supporting content creators and publishers by viewing this sponsored image.",
        comment: ""
      )
    public static let hideSponsoredImages =
      NSLocalizedString(
        "ntp.hideSponsoredImages",
        tableName: "BraveShared",
        bundle: .module,
        value: "Hide sponsored images",
        comment: ""
      )
    public static let goodJob =
      NSLocalizedString(
        "ntp.goodJob",
        tableName: "BraveShared",
        bundle: .module,
        value: "Way to go!",
        comment:
          "The context is to praise the user that they did a good job, to keep it up. It is used in full sentence like: 'Way to go! You earned 40 BAT last month.'"
      )
    public static let earningsReport =
      NSLocalizedString(
        "ntp.earningsReport",
        tableName: "BraveShared",
        bundle: .module,
        value: "You earned %@ by browsing with Brave.",
        comment: "Placeholder example: 'You earned 42 BAT by browsing with Brave.'"
      )
    public static let claimRewards =
      NSLocalizedString(
        "ntp.claimRewards",
        tableName: "BraveShared",
        bundle: .module,
        value: "Claim Tokens",
        comment: ""
      )

    public static let learnMoreAboutRewards =
      NSLocalizedString(
        "ntp.learnMoreAboutRewards",
        tableName: "BraveShared",
        bundle: .module,
        value: "Learn more about Brave Rewards",
        comment: ""
      )

    public static let learnMoreAboutSI =
      NSLocalizedString(
        "ntp.learnMoreAboutSI",
        tableName: "BraveShared",
        bundle: .module,
        value: "Learn more about sponsored images",
        comment: ""
      )

    public static let braveSupportFavoriteTitle =
      NSLocalizedString(
        "ntp.braveSupportFavoriteTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Support",
        comment: "Bookmark title for Brave Support"
      )

    public static let imageTypeSelectionDescription =
      NSLocalizedString(
        "ntp.imageTypeSelectionDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "'Sponsored' are additional branded backgrounds you can earn from with Brave Rewards enabled.",
        comment: "The text description of image type which is being used NTP"
      )

    public static let settingsTitle = NSLocalizedString(
      "ntp.settingsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "New Tab Page",
      comment: "Title on settings page to adjust the primary home screen functionality."
    )
    public static let settingsBackgroundImages = NSLocalizedString(
      "ntp.settingsBackgroundImages",
      tableName: "BraveShared",
      bundle: .module,
      value: "Background",
      comment: "A setting to enable or disable background on the NTP screen."
    )
    public static let settingsBackgroundImageSubMenu = NSLocalizedString(
      "ntp.settingsBackgroundImageSubMenu",
      tableName: "BraveShared",
      bundle: .module,
      value: "Media Type",
      comment: "A button that leads to a 'more' menu, letting users configure additional settings."
    )
    public static let settingsDefaultImagesOnly = NSLocalizedString(
      "ntp.settingsDefaultImagesOnly",
      tableName: "BraveShared",
      bundle: .module,
      value: "Default Images",
      comment:
        "A selection to let the users only see a set of predefined, default images on the background of the app."
    )
    public static let settingsSponsoredImagesSelection = NSLocalizedString(
      "ntp.settingsSponsoredImagesSelection",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sponsored Images",
      comment:
        "A selection to let the users see sponsored image backgrounds when opening a new tab."
    )
    public static let settingsSponsoredImagesAndVideosSelection = NSLocalizedString(
      "ntp.settingsSponsoredImagesAndVideosSelection",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sponsored Images & Videos",
      comment:
        "A selection to let the users see sponsored image and video backgrounds when opening a new tab."
    )
    public static let settingsAutoOpenKeyboard = NSLocalizedString(
      "ntp.settingsAutoOpenKeyboard",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto Open Keyboard",
      comment:
        "A setting to enable or disable the device's keyboard from opening automatically when creating a new tab."
    )

    public static let showMoreFavorites = NSLocalizedString(
      "ntp.showMoreFavorites",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show More",
      comment: "A button title to show more bookmarks, that opens a new menu."
    )

  }

}

// MARK: - Popover Views
extension Strings {
  public struct Popover {
    public static let closeShieldsMenu = NSLocalizedString(
      "PopoverShieldsMenuClose",
      tableName: "BraveShared",
      bundle: .module,
      value: "Close Shields Menu",
      comment: "Description for closing the `Brave Shields` popover menu that is displayed."
    )
  }
}

// MARK: - Shields
extension Strings {
  public struct Shields {
    public static let toggleHint = NSLocalizedString(
      "BraveShieldsToggleHint",
      tableName: "BraveShared",
      bundle: .module,
      value: "Double-tap to toggle Brave Shields",
      comment: "The accessibility hint spoken when focused on the main shields toggle"
    )
    public static let statusTitle = NSLocalizedString(
      "BraveShieldsStatusTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Shields",
      comment: "Context: 'Brave Shields Up' or 'Brave Shields Down'"
    )
    public static let statusValueUp = NSLocalizedString(
      "BraveShieldsStatusValueUp",
      tableName: "BraveShared",
      bundle: .module,
      value: "Up",
      comment: "Context: The 'Up' in 'Brave Shields Up'"
    )
    public static let statusValueDown = NSLocalizedString(
      "BraveShieldsStatusValueDown",
      tableName: "BraveShared",
      bundle: .module,
      value: "Down",
      comment: "Context: The 'Down' in 'Brave Shields Down'"
    )
    public static let blockedCountLabel = NSLocalizedString(
      "BraveShieldsBlockedCountLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Ads and other creepy things blocked",
      comment: "The number of ads and trackers blocked will be next to this"
    )
    public static let blockedInfoButtonAccessibilityLabel = NSLocalizedString(
      "BraveShieldsBlockedInfoButtonAccessibilityLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Learn more",
      comment:
        "What the screen reader will read out when the user has VoiceOver on and taps on the question-mark info button on the shields panel"
    )
    public static let siteBroken = NSLocalizedString(
      "BraveShieldsSiteBroken",
      tableName: "BraveShared",
      bundle: .module,
      value: "If this site appears broken, try Shields down",
      comment: ""
    )
    public static let advancedControls = NSLocalizedString(
      "BraveShieldsAdvancedControls",
      tableName: "BraveShared",
      bundle: .module,
      value: "Advanced controls",
      comment: ""
    )
    public static let aboutBraveShieldsTitle = NSLocalizedString(
      "AboutBraveShields",
      tableName: "BraveShared",
      bundle: .module,
      value: "About Brave Shields",
      comment: "The title of the screen explaining Brave Shields"
    )
    public static let aboutBraveShieldsBody = NSLocalizedString(
      "AboutBraveShieldsBody",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Sites often include cookies and scripts which try to identify you and your device. They want to work out who you are and follow you across the web — tracking what you do on every site.\n\nBrave blocks these things so that you can browse without being followed around.",
      comment: "The body of the screen explaining Brave Shields"
    )
    public static let shieldsDownDisclaimer = NSLocalizedString(
      "ShieldsDownDisclaimer",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "You're browsing this site without Brave's privacy protections. Does it not work right with Shields up?",
      comment: ""
    )
    public static let globalControls = NSLocalizedString(
      "BraveShieldsGlobalControls",
      tableName: "BraveShared",
      bundle: .module,
      value: "Global Controls",
      comment: ""
    )
    public static let globalChangeButton = NSLocalizedString(
      "BraveShieldsGlobalChangeButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "Change Shields Global Defaults",
      comment: ""
    )
    public static let siteReportedTitle = NSLocalizedString(
      "SiteReportedTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Thank You",
      comment: ""
    )
    public static let siteReportedBody = NSLocalizedString(
      "SiteReportedBody",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Thanks for letting Brave's developers know that there's something wrong with this site. We'll do our best to fix it!",
      comment: ""
    )

    // MARK: Submit report
    public static let reportABrokenSite = NSLocalizedString(
      "ReportABrokenSite",
      tableName: "BraveShared",
      bundle: .module,
      value: "Report a Broken Site",
      comment: ""
    )
    public static let reportBrokenSiteBody1 = NSLocalizedString(
      "ReportBrokenSiteBody1",
      tableName: "BraveShared",
      bundle: .module,
      value: "Let Brave's developers know that this site doesn't work properly with Shields:",
      comment:
        "First part of the report a broken site copy. After the colon is a new line and then a website address"
    )
    public static let reportBrokenSiteBody2 = NSLocalizedString(
      "ReportBrokenSiteBody2",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Note: The report sent to Brave servers will include the site address, Brave version number, Shields settings, VPN status, and language settings.",
      comment:
        "This is the info text that is presented when a user is submitting a web-compatibility report."
    )
    public static let reportBrokenSubmitButtonTitle = NSLocalizedString(
      "ReportBrokenSubmitButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Submit",
      comment: ""
    )

    /// A label for a text entry field where the user can provide additional details for a web-compatibility report
    public static let reportBrokenAdditionalDetails = NSLocalizedString(
      "ReportBrokenAdditionalDetails",
      tableName: "BraveShared",
      bundle: .module,
      value: "Additional details (optional)",
      comment:
        "A label for a text entry field where the user can provide additional details for a web-compatibility report"
    )

    public static let reportBrokenContactMe = NSLocalizedString(
      "ReportBrokenContactMe",
      tableName: "BraveShared",
      bundle: .module,
      value: "Contact me at: (optional)",
      comment:
        "A label for a text entry field where the user can provide contact details within a web-compatibilty report"
    )

    public static let reportBrokenContactMeSuggestions = NSLocalizedString(
      "ReportBrokenContactMeSuggestions",
      tableName: "BraveShared",
      bundle: .module,
      value: "Email, Twitter, etc.",
      comment:
        "A placeholder for a text entry field within a web-compatibilty report which shows a few suggestions of what the user should enter for their contact contact details (in a 'Contact me at: (optional)' field)."
    )

    public static let reportBrokenContactMeDescription = NSLocalizedString(
      "ReportBrokenContactMeDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "If you provide contact info it will be stored for reporting broken sites in the future.",
      comment: "Contact info storing description"
    )
  }
}

// MARK: ShieldEducation

extension Strings {
  public struct ShieldEducation {
    public static let trackerCountShareTitle =
      NSLocalizedString(
        "shieldEducation.trackerCountShareTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "%ld trackers & ads blocked!",
        comment:
          "Title for Shield Education Tracker Count Share. The parameter substituted for \"%ld\" is the count of the trackers and ads blocked in total until present. E.g.: 5000 trackers & ads blocked"
      )

    public static let videoAdBlockTitle =
      NSLocalizedString(
        "shieldEducation.videoAdBlockTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Ads are blocked while watching videos on this website.",
        comment: "Title for Shield Education Tracker Video Ad Block"
      )

    public static let trackerCountShareSubtitle =
      NSLocalizedString(
        "shieldEducation.trackerCountShareSubtitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You're pretty special.",
        comment: "Subtitle for Shield Education Tracker Count Share"
      )

    public static let trackerAdWarningSubtitle =
      NSLocalizedString(
        "shieldEducation.trackerAdWarningSubTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Shields just protected your online privacy.",
        comment: "Subtitle for Shield Education Tracker Ad Warning"
      )

    public static let videoAdBlockSubtitle =
      NSLocalizedString(
        "shieldEducation.videoAdBlockSubtitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Videos without ads use less data.",
        comment: "Subtitle for Shield Education Tracker Video Ad Block"
      )

    public static let shareTheNewsTitle =
      NSLocalizedString(
        "shieldEducation.shareTheNewsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Share the news",
        comment: "Action title for actionable Share warnings"
      )

    public static let benchmarkAnyTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkAnyTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congrats on reaching this privacy milestone.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkSpecialTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkSpecialTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You’re pretty special.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkExclusiveTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkExclusiveTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You’re part of an exclusive club.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkProfessionalTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkProfessionalTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You joined the pros.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkPrimeTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkPrimeTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You’ve become a master.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkGrandTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkGrandTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You’ve become a Grand Master.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let benchmarkLegendaryTierTitle =
      NSLocalizedString(
        "shieldEducation.benchmarkLegendaryTierTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Congratulations. You are legendary.",
        comment: "Subtitle for tracker benchmark Share"
      )

    public static let shareDescriptionTitle =
      NSLocalizedString(
        "socialSharing.shareDescriptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Every day I save data by browsing the web with Brave.",
        comment: "Text used for social sharing together with Brave Shield values"
      )

    public static let domainSpecificDataSavedTitle =
      NSLocalizedString(
        "socialSharing.domainSpecificDataSavedTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Every day I save data by browsing the web with Brave.",
        comment: "Title used when in warning pop-over when domain specific data save appears "
      )

    public static let domainSpecificDataSavedSubtitle =
      NSLocalizedString(
        "shieldEducation.domainSpecificDataSavedSubtitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Average data saved\n%@ MB",
        comment:
          "Subtitle for  The parameter substituted for \"%@\" is the amount of data saved in MB. E.g.: Average Data Saved: 17.43 MB"
      )

    public static let dontShowThisTitle =
      NSLocalizedString(
        "shieldEducation.dontShowThisTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Don't show this again",
        comment: "Action title for button at domain specific data saved pop-up"
      )
  }
}

// MARK: PlayList

extension Strings {
  public struct PlayList {
    public static let playListSectionTitle =
      NSLocalizedString(
        "playList.playListSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlists",
        comment: "Title For the Section that videos are listed"
      )

    public static let playListTitle =
      NSLocalizedString(
        "playList.playListSettingTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist",
        comment: "Title Playlist (used for back button, and playlist settings)"
      )

    public static let playListSharedFolderSectionTitle =
      NSLocalizedString(
        "playList.playListSharedFolderSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Shared with you",
        comment: "Title For the Section that videos are listed"
      )

    public static let removeActionButtonTitle =
      NSLocalizedString(
        "playList.removeActionButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Remove",
        comment: "Title for removing offline mode storage"
      )

    public static let addedToPlaylistMessage =
      NSLocalizedString(
        "playList.addedToPlaylistMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Added to %@",
        comment:
          "A message shown when the user adds media to the playlist. '%@' will be replaced by the folder it was added to. Example: 'Added to Play Later'"
      )

    public static let removedFromPlaylistMessage =
      NSLocalizedString(
        "playList.removedFromPlaylistMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Removed from %@",
        comment:
          "A message shown when the user removes media from the playlist. '%@' will be replaced by the folder it was added to. Example: 'Removed from Play Later'"
      )

    public static let openInPlaylistButtonTitle =
      NSLocalizedString(
        "playList.openInPlaylistButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open In Playlist",
        comment: "Title for opening the playlist"
      )

    public static let changeFoldersButtonTitle =
      NSLocalizedString(
        "playList.changeFoldersButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Change Playlist",
        comment: "Title for changing the folder of the current media playlist item"
      )

    public static let undoRemoveButtonTitle =
      NSLocalizedString(
        "playList.undoRemoveButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Undo",
        comment:
          "A button title that when tapped will re-add the media item (undo the removal) that the user recently removed."
      )

    public static let noticeAlertTitle =
      NSLocalizedString(
        "playList.noticeAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Notice",
        comment: "Title for download video error alert"
      )

    public static let okayButtonTitle =
      NSLocalizedString(
        "playList.okayButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Okay",
        comment: "Okay Alert button title"
      )

    public static let reopenButtonTitle =
      NSLocalizedString(
        "playList.reopenButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reopen",
        comment: "Reopen Alert button title"
      )

    public static let sorryAlertTitle =
      NSLocalizedString(
        "playList.sorryAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry",
        comment: "Title for load resources error alert"
      )

    public static let loadResourcesErrorAlertDescription =
      NSLocalizedString(
        "playList.loadResourcesErrorAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "There was a problem loading the resource!",
        comment: "Description for load resources error alert"
      )

    public static let addToPlayListAlertTitle =
      NSLocalizedString(
        "playList.addToPlayListAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Add to Brave Playlist",
        comment: "Alert Title for adding videos to playlist"
      )

    public static let addToPlayListAlertDescription =
      NSLocalizedString(
        "playList.addToPlayListAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Would you like to add this video to your playlist?",
        comment: "Alert Description for adding videos to playlist"
      )

    public static let savingForOfflineLabelTitle =
      NSLocalizedString(
        "playList.savingForOfflineLabelTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Saving for Offline…",
        comment: "Text indicator on the table cell while saving a video for offline"
      )

    public static let savedForOfflineLabelTitle =
      NSLocalizedString(
        "playList.savedForOfflineLabelTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Added to Offline",
        comment:
          "Text indicator on the table cell while saving a video for offline with percentage eg: %25 Saved for Offline"
      )

    public static let noItemLabelTitle =
      NSLocalizedString(
        "playList.noItemLabelTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "No Items Available",
        comment: "Text when there are no items in the playlist"
      )

    public static let noItemLabelDetailLabel =
      NSLocalizedString(
        "playList.noItemLabelDetailLabel",
        tableName: "BraveShared",
        bundle: .module,
        value: "You can add items to your Brave Playlist within the browser",
        comment: "Detail Text when there are no items in the playlist"
      )

    public static let expiredLabelTitle =
      NSLocalizedString(
        "playList.expiredLabelTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Expired",
        comment: "Text indicator on the table cell If a video is expired"
      )

    public static let expiredAlertTitle =
      NSLocalizedString(
        "playList.expiredAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Expired Video",
        comment: "The title for the alert that shows up when an item is expired"
      )

    public static let expiredAlertDescription =
      NSLocalizedString(
        "playList.expiredAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This video was a live stream or the time limit was reached. Please reopen the link to refresh.",
        comment: "The description for the alert that shows up when an item is expired"
      )

    public static let pictureInPictureErrorTitle =
      NSLocalizedString(
        "playList.pictureInPictureErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry, an error occurred while attempting to display picture-in-picture.",
        comment: "The title for the alert that shows up when an item is expired"
      )

    public static let toastAddToPlaylistTitle =
      NSLocalizedString(
        "playList.toastAddToPlaylistTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Add to Brave Playlist",
        comment: "The title for the toast that shows up on a page containing a playlist item"
      )

    public static let toastAddedToPlaylistTitle =
      NSLocalizedString(
        "playList.toastAddedToPlaylistTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Added to Brave Playlist",
        comment:
          "The title for the toast that shows up on a page containing a playlist item that was added to playlist"
      )

    public static let toastAddToPlaylistOpenButton =
      NSLocalizedString(
        "playList.toastAddToPlaylistOpenButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open",
        comment: "The title for the toast button when an item was added to playlist"
      )

    public static let toastExitingItemPlaylistTitle =
      NSLocalizedString(
        "playList.toastExitingItemPlaylistTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "View in Brave Playlist",
        comment:
          "The title for the toast that shows up on a page when an item that has already been added, was updated."
      )

    public static let removePlaylistVideoAlertTitle =
      NSLocalizedString(
        "playlist.removePlaylistVideoAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Remove Media Item from Playlist?",
        comment: "Title for the alert shown when the user tries to remove an item from playlist"
      )

    public static let removePlaylistVideoAlertMessage =
      NSLocalizedString(
        "playlist.removePlaylistVideoAlertMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "This will remove the media item from the list. Are you sure you want to continue?",
        comment: "Message for the alert shown when the user tries to remove a video from playlist"
      )

    public static let removePlaylistOfflineDataAlertTitle =
      NSLocalizedString(
        "playlist.removePlaylistOfflineDataAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Remove Offline Data",
        comment:
          "Title for the alert shown when the user tries to remove offline data of an item from playlist"
      )

    public static let removePlaylistOfflineDataAlertMessage =
      NSLocalizedString(
        "playlist.removePlaylistOfflineDataAlertMessage",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This will delete the media from offline storage. Are you sure you want to continue?",
        comment:
          "Message for the alert shown when the user tries to remove offline data of an item from playlist"
      )

    public static let urlBarButtonOptionTitle =
      NSLocalizedString(
        "playlist.urlBarButtonOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable quick-access button",
        comment: "Title for option to disable URL-Bar button"
      )

    public static let urlBarButtonOptionFooter =
      NSLocalizedString(
        "playlist.urlBarButtonOptionFooter",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Adds a playlist button (it looks like 4 lines with a + symbol) beside the address bar in the Brave browser. This button gives you quick access to open Playlist, or add or remove media.",
        comment: "Footer for option to disable URL-Bar button"
      )

    public static let sharePlaylistActionTitle =
      NSLocalizedString(
        "playlist.sharePlaylistActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Playlist Menu",
        comment: "Title of the ActionSheet/Alert when sharing a playlist item from the Swipe-Action"
      )

    public static let sharePlaylistActionDetailsTitle =
      NSLocalizedString(
        "playlist.sharePlaylistActionDetailsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "You can open the current item in a New Tab, or share it via the System Share Menu",
        comment:
          "Details Title of the ActionSheet/Alert when sharing a playlist item from the Swipe-Action"
      )

    public static let sharePlaylistOpenInNewTabTitle =
      NSLocalizedString(
        "playlist.sharePlaylistOpenInNewTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open In New Tab",
        comment:
          "Button Title of the ActionSheet/Alert Button when sharing a playlist item from the Swipe-Action"
      )

    public static let sharePlaylistOpenInNewPrivateTabTitle =
      NSLocalizedString(
        "playlist.sharePlaylistOpenInNewPrivateTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open In Private Tab",
        comment:
          "Button Title of the ActionSheet/Alert Button when sharing a playlist item from the Swipe-Action"
      )

    public static let sharePlaylistMoveActionMenuTitle =
      NSLocalizedString(
        "playlist.movePlaylistShareActionMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Move...",
        comment:
          "Button Title of the ActionSheet/Alert Button when moving a playlist item from the Swipe-Action to a new folder"
      )

    public static let sharePlaylistShareActionMenuTitle =
      NSLocalizedString(
        "playlist.sharePlaylistShareActionMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Share...",
        comment:
          "Button Title of the ActionSheet/Alert Button when sharing a playlist item from the Swipe-Action"
      )

    public static let menuBadgeOptionTitle =
      NSLocalizedString(
        "playlist.menuBadgeOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Show Menu Notification Badge",
        comment: "Title for playlist menu badge option"
      )

    public static let menuBadgeOptionFooterText =
      NSLocalizedString(
        "playlist.menuBadgeOptionFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "When enabled, a badge will be displayed on the main menu icon, indicating media on the page may be added to Brave Playlist.",
        comment: "Description footer for playlist menu badge option"
      )

    public static let playlistLongPressSettingsOptionTitle =
      NSLocalizedString(
        "playlist.playlistLongPressSettingsOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable Long Press",
        comment: "Title for the Playlist Settings Option for long press gesture"
      )

    public static let playlistLongPressSettingsOptionFooterText =
      NSLocalizedString(
        "playlist.playlistLongPressSettingsOptionFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "When enabled, you can long-press on most video/audio files to add them to your Playlist.",
        comment: "Footer Text for the Playlist Settings Option for long press gesture"
      )

    public static let playlistAutoPlaySettingsOptionTitle =
      NSLocalizedString(
        "playlist.playlistAutoPlaySettingsOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Auto-Play",
        comment: "Title for the Playlist Settings Option for Enable/Disable Auto-Play"
      )

    public static let playlistAutoPlaySettingsOptionFooterText =
      NSLocalizedString(
        "playlist.playlistAutoPlaySettingsOptionFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This option will enable/disable auto-play when Playlist is opened. However, this option will not affect auto-play when loading the next video on the list.",
        comment: "Footer Text for the Playlist Settings Option for Enable/Disable Auto-Play"
      )

    public static let playlistSidebarLocationTitle =
      NSLocalizedString(
        "playlist.playlistSidebarLocationTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sidebar Location",
        comment: "Title for the Playlist Settings Option for Sidebar Location (Left/Right)"
      )

    public static let playlistSidebarLocationFooterText =
      NSLocalizedString(
        "playlist.playlistSidebarLocationFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This setting will change video list location between left-hand side/ right-hand side.",
        comment: "Footer Text for the Playlist Settings Option for Sidebar Location (Left/Right)"
      )

    public static let playlistSidebarLocationOptionLeft =
      NSLocalizedString(
        "playlist.playlistSidebarLocationOptionLeft",
        tableName: "BraveShared",
        bundle: .module,
        value: "Left",
        comment: "Option Text for Sidebar Location Left"
      )

    public static let playlistSidebarLocationOptionRight =
      NSLocalizedString(
        "playlist.playlistSidebarLocationOptionRight",
        tableName: "BraveShared",
        bundle: .module,
        value: "Right",
        comment: "Option Text for Sidebar Location Right"
      )

    public static let playlistAutoSaveSettingsTitle =
      NSLocalizedString(
        "playlist.playlistAutoSaveSettingsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Auto-Save for Offline",
        comment:
          "Title for the Playlist Settings Option for Auto Save Videos for Offline (Off/On/Only Wi-fi)"
      )

    public static let playlistAutoSaveSettingsDescription =
      NSLocalizedString(
        "playlist.playlistAutoSaveSettingsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Adding video and audio files for offline use can use a lot of storage on your device as well as use your cellular data",
        comment:
          "Description for the Playlist Settings Option for Auto Save Videos for Offline (Off/On/Only Wi-fi)"
      )

    public static let playlistAutoSaveSettingsFooterText =
      NSLocalizedString(
        "playlist.playlistAutoSaveSettingsFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This option will automatically keep your playlist items on your device so you can play them without an internet connection.",
        comment:
          "Footer Text for the Playlist Settings Option for Auto Save Videos for Offline (Off/On/Only Wi-fi))"
      )

    public static let playlistStartPlaybackSettingsOptionTitle =
      NSLocalizedString(
        "playlist.playlistStartPlaybackSettingsOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Remember File Playback Position",
        comment:
          "Title for the Playlist Settings Option for Enable/Disable ability to start playing from the point where user last left-off"
      )

    public static let playlistStartPlaybackSettingsFooterText =
      NSLocalizedString(
        "playlist.playlistStartPlaybackSettingsFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value: "For individual files, resume playing from the point of last pause.",
        comment:
          "Footer Text for the Playlist Settings Option for Enable/Disable ability to start playing from the point where user last left-off"
      )

    public static let playlistAutoSaveOptionOn =
      NSLocalizedString(
        "playlist.playlistAutoSaveOptionOn",
        tableName: "BraveShared",
        bundle: .module,
        value: "On",
        comment: "Auto Save turn On Option"
      )

    public static let playlistAutoSaveOptionOff =
      NSLocalizedString(
        "playlist.playlistAutoSaveOptionOff",
        tableName: "BraveShared",
        bundle: .module,
        value: "Off",
        comment: "Auto Download turn Off Option"
      )

    public static let playlistAutoSaveOptionOnlyWifi =
      NSLocalizedString(
        "playlist.playlistAutoSaveOptionOnlyWifi",
        tableName: "BraveShared",
        bundle: .module,
        value: "Only Wi-Fi",
        comment: "Option Text for Auto Save Only Wi-Fi Option"
      )

    public static let playlistOfflineDataToggleOption =
      NSLocalizedString(
        "playlist.playlistOfflineDataToggleOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist Offline Data",
        comment:
          "Text for Playlist Offline Data Toggle while clearing the offline data storage in settings"
      )

    public static let playlistMediaAndOfflineDataToggleOption =
      NSLocalizedString(
        "playlist.playlistMediaAndOfflineDataToggleOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist Media & Offline Data",
        comment:
          "Text for Playlist Media & Offline Data Toggle while clearing the offline data storage in settings"
      )

    public static let playlistResetAlertTitle =
      NSLocalizedString(
        "playList.playlistResetAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset",
        comment:
          "The title for the alert that shows up when removing all videos and their offline mode storage."
      )

    public static let playlistResetPlaylistOptionFooterText =
      NSLocalizedString(
        "playlist.playlistResetPlaylistOptionFooterText",
        tableName: "BraveShared",
        bundle: .module,
        value: "This option will remove all videos from Playlist as well as Offline mode storage.",
        comment: "Footer Text for the Playlist Settings Option for resetting Playlist."
      )

    public static let playlistSaveForOfflineErrorTitle =
      NSLocalizedString(
        "playlist.playlistSaveForOfflineErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry, something went wrong",
        comment: "Title of alert when saving a playlist item for offline mode"
      )

    public static let playlistSaveForOfflineErrorMessage =
      NSLocalizedString(
        "playlist.playlistSaveForOfflineErrorMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry, this item could not be saved for offline mode at this time.",
        comment: "Error message when saving a playlist item for offline fails"
      )

    public static let playlistWebCompatibilityTitle =
      NSLocalizedString(
        "playlist.playlistWebCompatibilityTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Web Compatibility",
        comment: "Title for Playlist setting"
      )

    public static let playlistWebCompatibilityDescription =
      NSLocalizedString(
        "playlist.playlistWebCompatibilityDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Disables the WebKit MediaSource API",
        comment: "Description for Playlist setting"
      )

    public static let playlistLiveMediaStream =
      NSLocalizedString(
        "playlist.playlistLiveMediaStream",
        tableName: "BraveShared",
        bundle: .module,
        value: "Live Stream",
        comment: "When a video or audio is live and has no duration"
      )

    public static let playlistDiskSpaceWarningTitle =
      NSLocalizedString(
        "playlist.playlistDiskSpaceWarningTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Storage Almost Full",
        comment: "When the user's disk space is almost full"
      )

    public static let playlistDiskSpaceWarningMessage =
      NSLocalizedString(
        "playlist.playlistDiskSpaceWarningMessage",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Adding video and audio files for offline use can use a lot of storage on your device. Please remove some files to free up storage space.",
        comment: "When the user's disk space is almost full"
      )

    public static let playlistPopoverChangeFoldersButtonTitle =
      NSLocalizedString(
        "playlist.playlistPopoverChangeFoldersButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Change",
        comment:
          "Title of a button displayed when first adding some media to Playlist. Its shown next to the name of a folder that the item is recently added to to imply that you are changing the folder it was saved to"
      )

    public static let playlistCarplayTitle =
      NSLocalizedString(
        "playlist.carplayTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Playlist",
        comment: "The title of the playlist when in Carplay mode"
      )

    public static let playlistCarplaySettingsSectionTitle =
      NSLocalizedString(
        "playlist.carplaySettingsSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Settings",
        comment: "The title of the section containing settings/options in CarPlay"
      )

    public static let playlistCarplayOptionsScreenTitle =
      NSLocalizedString(
        "playlist.carplayOptionsScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playback Options",
        comment: "The title of the screen in CarPlay that contains all the audio/playback options"
      )

    public static let playlistCarplayRestartPlaybackOptionTitle =
      NSLocalizedString(
        "playlist.carplayRestartPlaybackOptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Restart Playback",
        comment: "The title of the checkbox that allows the user to restart audio/video playback"
      )

    public static let playlistCarplayRestartPlaybackOptionDetailsTitle =
      NSLocalizedString(
        "playlist.carplayRestartPlaybackOptionDetailsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Decide whether or not selecting an already playing item will restart playback, or continue playing where last left off",
        comment:
          "The description of the checkbox that allows the user to restart audio/video playback"
      )

    public static let playlistCarplayRestartPlaybackButtonStateEnabled =
      NSLocalizedString(
        "playlist.carplayRestartPlaybackButtonStateEnabled",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enabled. Tap to disable playback restarting.",
        comment:
          "The already enabled button title with instructions telling the user they can tap it to disable playback."
      )

    public static let playlistCarplayRestartPlaybackButtonStateDisabled =
      NSLocalizedString(
        "playlist.carplayRestartPlaybackButtonStateDisabled",
        tableName: "BraveShared",
        bundle: .module,
        value: "Disabled. Tap to enable playback restarting.",
        comment:
          "The already disabled button title with instructions telling the user they can tap it to enable playback."
      )

    public static let playlistSaveForOfflineButtonTitle =
      NSLocalizedString(
        "playlist.saveForOfflineButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Save for Offline",
        comment:
          "The title of the button indicating that the user can save a video for offline playback. (playing without internet)"
      )

    public static let playlistDeleteForOfflineButtonTitle =
      NSLocalizedString(
        "playlist.deleteForOfflineButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Delete Offline Cache",
        comment:
          "The title of the button indicating that the user delete the offline data. (deletes the data that allows them to play offline)"
      )

    public static let playlistAlreadyShowingTitle =
      NSLocalizedString(
        "playlist.playlistAlreadyShowingTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry",
        comment: "Playlist alert title when playlist is already showing on a different window"
      )

    public static let playlistAlreadyShowingBody =
      NSLocalizedString(
        "playlist.playlistAlreadyShowingBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist is already active on another window",
        comment: "Playlist alert message when playlist is already showing on a different window"
      )

    public static let folderItemCountSingular =
      NSLocalizedString(
        "playlist.foldersCountSingular",
        tableName: "BraveShared",
        bundle: .module,
        value: "%d item",
        comment: "Title for when there's a single item inside of a folder"
      )

    public static let folderItemCountPlural =
      NSLocalizedString(
        "playlist.folderItemCountPlural",
        tableName: "BraveShared",
        bundle: .module,
        value: "%d items",
        comment: "Number of items in the playlsit folder. Example: '10 items'"
      )
  }

  public struct PlaylistFolders {
    public static let playlistUntitledFolderTitle =
      NSLocalizedString(
        "playlistFolders.untitledFolderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Untitled Playlist",
        comment: "The title of a folder when the user enters no name (untitled)"
      )

    public static let playlistEditFolderScreenTitle =
      NSLocalizedString(
        "playlistFolders.editFolderScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Edit Folder",
        comment: "The title of the screen where the user edits folder names"
      )

    public static let playlistNewFolderScreenTitle =
      NSLocalizedString(
        "playlistFolders.newFolderScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "New Playlist",
        comment: "The title of the screen to create a new folder"
      )

    public static let playlistNewFolderButtonTitle =
      NSLocalizedString(
        "playlistFolders.newFolderButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "New Playlist",
        comment: "The title of the button to create a new folder"
      )

    public static let playlistCreateNewFolderButtonTitle =
      NSLocalizedString(
        "playlistFolders.createNewFolderButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Create",
        comment: "The title of the button to create a new folder"
      )

    public static let playlistFolderSubtitleItemSingleCount =
      NSLocalizedString(
        "playlistFolders.subtitleItemSingleCount",
        tableName: "BraveShared",
        bundle: .module,
        value: "1 Item",
        comment:
          "The sub-title of the folder. Example: This folder contains ONE item. This folder contains 1 Item."
      )

    public static let playlistFolderSubtitleItemCount =
      NSLocalizedString(
        "playlistFolders.subtitleItemCount",
        tableName: "BraveShared",
        bundle: .module,
        value: "%lld Items",
        comment:
          "The sub-title of the folder. Example: This folder contains 10 Items. This folder contains 3 Items."
      )

    public static let playlistFolderErrorSavingMessage =
      NSLocalizedString(
        "playlistFolders.errorSavingMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry we were unable to save the changes you made to this folder",
        comment: "The error shown to the user when we cannot save their changes made on a folder"
      )

    public static let playlistFolderEditMenuTitle =
      NSLocalizedString(
        "playlistFolders.editMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Edit Playlist",
        comment: "The title of the menu option that allows the user to edit the name of a folder"
      )

    public static let playlistFolderEditButtonTitle =
      NSLocalizedString(
        "playlistFolders.editButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Edit",
        comment: "The title of the button that allows the user to edit the name of a folder"
      )

    public static let playlistFolderNewFolderSectionTitle =
      NSLocalizedString(
        "playlistFolders.newFolderSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Add videos to this playlist",
        comment:
          "The title of the section where the user can select videos to add to the new folder being created"
      )

    public static let playlistFolderNewFolderSectionSubtitle =
      NSLocalizedString(
        "playlistFolders.newFolderSectionSubtitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Tap to select videos",
        comment:
          "The sub-title of the section where the user can select videos to add to the new folder being created"
      )

    public static let playlistFolderMoveFolderCurrentSectionTitle =
      NSLocalizedString(
        "playlistFolders.moveFolderCurrentSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Current Playlist",
        comment: "The title of the section indicating the currently selected folder"
      )

    public static let playlistFolderSelectAFolderTitle =
      NSLocalizedString(
        "playlistFolders.selectAFolderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Select a playlist to move %lld items to",
        comment:
          "The title of the section indicating that the user should select a folder to move `%zu` items to. %lld should not be translated. It will be replaced by a number so the sentence becomes: 'Select a folder to move 10 items to.'"
      )

    public static let playlistFolderSelectASingleFolderTitle =
      NSLocalizedString(
        "playlistFolders.selectASingleFolderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Select a playlist to move 1 item to",
        comment:
          "The title of the section indicating that the user should select a folder to move 1 item to."
      )

    public static let playlistFolderMoveFolderScreenTitle =
      NSLocalizedString(
        "playlistFolders.moveFolderScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Move",
        comment:
          "The title of the screen where the user will move an item from one folder to another folder."
      )

    public static let playlistFolderMoveFolderButtonTitle =
      NSLocalizedString(
        "playlistFolders.moveFolderButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Move",
        comment:
          "The title of the button where the user will move an item from one folder to another folder."
      )

    public static let playlistFolderMoveItemDescription =
      NSLocalizedString(
        "playlistFolders.moveItemDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ and 1 more item",
        comment:
          "%@ Should NOT be localized. It is a placeholder. Example: Brave Folder and 1 more item."
      )

    public static let playlistFolderMoveMultipleItemDescription =
      NSLocalizedString(
        "playlistFolders.moveMultipleItemDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ and %lld more items",
        comment:
          "%@ and %lld Should NOT be localized. They are placeholders. Example: Brave Folder and 3 more items. Music and 2 more items."
      )

    public static let playlistFolderMoveItemWithNoNameTitle =
      NSLocalizedString(
        "playlistFolders.moveItemWithNoNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "1 item",
        comment: "Sometimes an item can have no name. So this is a generic title to use"
      )

    public static let playlistFolderMoveItemWithMultipleNoNameTitle =
      NSLocalizedString(
        "playlistFolders.moveItemWithMultipleNoNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "%lld items",
        comment:
          "%lld is a placeholder and should not be localized. Sometimes items can have no name. So this is a generic title to use"
      )

    public static let playlistChangeFoldersTitle =
      NSLocalizedString(
        "playlistFolders.changeFoldersTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Move To",
        comment:
          "The title of the screen that allows you to change the folder where a playlist item was saved to"
      )

    public static let playlistCreateFolderTextFieldLabel =
      NSLocalizedString(
        "playlistFolders.createFolderTextFieldLabel",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist Name",
        comment:
          "A label shown above the text field that allows the user to type a name of the playlist they are going to create"
      )

    public static let playlistCreateFolderTextFieldPlaceholder =
      NSLocalizedString(
        "playlistFolders.createFolderTextFieldPlaceholder",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enter your playlist name",
        comment:
          "A label shown above the text field that allows the user to type a name of the playlist they are going to create"
      )
  }

  public struct PlaylistFolderSharing {
    public static let addButtonTitle =
      NSLocalizedString(
        "playlistFolderSharing.addButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Save",
        comment: "Title for the button that adds the playlist to the database."
      )

    public static let addButtonAccessibilityTitle =
      NSLocalizedString(
        "playlistFolderSharing.addButtonAccessibilityTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Add to Brave Playlist Button",
        comment: "Accessibility Title for the button that adds the playlist to the database."
      )

    public static let menuButtonAccessibilityTitle =
      NSLocalizedString(
        "playlistFolderSharing.menuButtonAccessibilityTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Playlist Menu Button",
        comment: "Accessibility Title for the playlist menu."
      )

    public static let syncNowMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.syncNowMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Now",
        comment: "Menu Title for syncing playlist"
      )

    public static let editMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.editMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Edit",
        comment: "Menu Title for editing a folder"
      )

    public static let renameMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.renameMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Rename",
        comment: "Menu Title for renaming a folder"
      )

    public static let saveOfflineDataMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.saveOfflineDataMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Save Offline Data",
        comment: "Menu Title for saving offline data/cache"
      )

    public static let deleteOfflineDataMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.deleteOfflineDataMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Remove Offline Data",
        comment: "Menu Title for removing offline data/cache"
      )

    public static let deletePlaylistMenuTitle =
      NSLocalizedString(
        "playlistFolderSharing.deletePlaylistMenuTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Delete Playlist",
        comment: "Menu Title for deleting the playlist"
      )

    public static let offlineManagementViewTitle =
      NSLocalizedString(
        "playlistFolderSharing.offlineManagementViewTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Managing your Playlist data",
        comment: "Title of the playlist offline data management view"
      )

    public static let offlineManagementViewDescription =
      NSLocalizedString(
        "playlistFolderSharing.offlineManagementViewDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Auto-save for offline use is on, meaning new additions to playlists, including shared playlists are saved to your device for viewing offline and could use your cellular data.\n\nAuto-save for offline use can be managed in Playlist settings.",
        comment: "Description of the playlist offline data management view"
      )

    public static let offlineManagementViewAddButtonTitle =
      NSLocalizedString(
        "playlistFolderSharing.offlineManagementViewAddButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Add playlist now",
        comment: "Button that adds the playlist to the local database"
      )

    public static let offlineManagementViewSettingsButtonTitle =
      NSLocalizedString(
        "playlistFolderSharing.offlineManagementViewSettingsButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Settings",
        comment: "Button that takes the user to the settings menu"
      )

    public static let playlistSharedFolderSectionTitle =
      NSLocalizedString(
        "playList.playlistSharedFolderSectionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Shared with you",
        comment: "Title for the Section that videos are listed"
      )

    public static let playlistSharedFolderAlreadyExistsTitle =
      NSLocalizedString(
        "playList.playlistSharedFolderAlreadyExistsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sorry",
        comment: "Title for the error message that shows when a playlist folder already exists"
      )

    public static let playlistSharedFolderAlreadyExistsBody =
      NSLocalizedString(
        "playList.playlistSharedFolderAlreadyExistsBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "This folder already exists in your Brave Playlist",
        comment: "Body for the error message that shows when a playlist folder already exists"
      )

    public static let sharedFolderSyncAutomaticallyTitle =
      NSLocalizedString(
        "playList.sharedFolderSyncAutomaticallyTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Playlist Folders Automatically",
        comment:
          "Title of the settings option to sync folders with the server automatically, every 4 hours."
      )

    public static let sharedFolderSyncAutomaticallyDescription =
      NSLocalizedString(
        "playList.sharedFolderSyncAutomaticallyDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Syncs all your playlist folders automatically",
        comment:
          "Description of the settings option to sync folders with the server automatically, every 4 hours."
      )
  }

}

// MARK: - SSL Certificate Viewer

extension Strings {
  public struct CertificateViewer {
    public static let certificateIsValidTitle =
      NSLocalizedString(
        "certificateViewer.certificateIsValidTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "This certificate is valid",
        comment: "The description for when an SSL certificate is valid"
      )

    public static let subjectNameTitle =
      NSLocalizedString(
        "certificateViewer.subjectNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Subject Name",
        comment: "Section Title for Subject Name in the SSL Certificate"
      )

    public static let issuerNameTitle =
      NSLocalizedString(
        "certificateViewer.issuerNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Issuer Name",
        comment: "Section Title for Issuer Name in the SSL Certificate"
      )

    public static let commonInfoTitle =
      NSLocalizedString(
        "certificateViewer.commonInfoTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Common Info",
        comment: "Section Title displaying common information"
      )

    public static let serialNumberTitle =
      NSLocalizedString(
        "certificateViewer.serialNumberTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Serial Number",
        comment: "Certificates have a serial number to identify that it's unique"
      )

    public static let versionNumberTitle =
      NSLocalizedString(
        "certificateViewer.versionNumberTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Version",
        comment: "Certificates have a version number"
      )

    public static let signatureAlgorithmTitle =
      NSLocalizedString(
        "certificateViewer.signatureAlgorithmTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Signature Algorithm",
        comment:
          "Title for the section where we display information about the algorithm used to sign the certificate"
      )

    public static let signatureAlgorithmSignatureDescription =
      NSLocalizedString(
        "certificateViewer.signatureAlgorithmSignatureDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ Signature with %@",
        comment:
          "Do NOT translate the %@. They are place holders. Example: 'ECDSA Signature' with 'SHA-256'"
      )

    public static let signatureAlgorithmEncryptionDescription =
      NSLocalizedString(
        "certificateViewer.signatureAlgorithmEncryptionDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ with %@ Encryption",
        comment:
          "Do NOT translate the %@. They are place holders. Example: 'SHA256' with 'RSA' Encryption"
      )

    public static let validityDatesTitle =
      NSLocalizedString(
        "certificateViewer.validityDatesTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Validity Dates",
        comment: "Title of section that determines if the dates on a certificate is valid."
      )

    public static let notValidBeforeTitle =
      NSLocalizedString(
        "certificateViewer.notValidBeforeTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Not Valid Before",
        comment: "Certificate is 'not valid before' January 1st, 2022 for example."
      )

    public static let notValidAfterTitle =
      NSLocalizedString(
        "certificateViewer.notValidAfterTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Not Valid After",
        comment: "Certificate is 'not valid after' January 31st, 2022 for example."
      )

    public static let publicKeyInfoTitle =
      NSLocalizedString(
        "certificateViewer.publicKeyInfoTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Public Key Info",
        comment: "Information about a Public Key section title."
      )

    public static let signatureTitle =
      NSLocalizedString(
        "certificateViewer.signatureTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Signature",
        comment:
          "Title of the view that states whether or not something was signed with an encryption algorithm or hash."
      )

    public static let fingerPrintsTitle =
      NSLocalizedString(
        "certificateViewer.fingerPrintsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Fingerprints",
        comment:
          "Fingerprints/Hashes are algorithms used to determine if the certificate is legitimate"
      )

    public static let countryOrRegionTitle =
      NSLocalizedString(
        "certificateViewer.countryOrRegionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Country or Region",
        comment:
          "Title of the section for the certificate's issuing country or region. Example: Canada, or USA"
      )

    public static let stateOrProvinceTitle =
      NSLocalizedString(
        "certificateViewer.stateOrProvinceTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "State/Province",
        comment:
          "Title of the section for the certificate's issuing state or province. Example: Ontario Province or California State"
      )

    public static let localityTitle =
      NSLocalizedString(
        "certificateViewer.localityTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Locality",
        comment:
          "Title of the section for the certificate's issuing city. Example: Toronto, New York, or San Francisco"
      )

    public static let organizationTitle =
      NSLocalizedString(
        "certificateViewer.organizationTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Organization",
        comment: "Title of the section for Name of the Company. Example: Brave Inc."
      )

    public static let organizationalUnitTitle =
      NSLocalizedString(
        "certificateViewer.organizationalUnitTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Organizational Unit",
        comment: "Title of the section for Department of the Company. Example: Human Resources."
      )

    public static let commonNameTitle =
      NSLocalizedString(
        "certificateViewer.commonNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Common Name",
        comment:
          "Title of the section for Commonly used Name for the certificate. Example: Alias, Commonly used name, DigiCert High Assurance TLS."
      )

    public static let streetAddressTitle =
      NSLocalizedString(
        "certificateViewer.streetAddressTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Street Address",
        comment: "Title of the section for the address of where the certificate came from."
      )

    public static let domainComponentTitle =
      NSLocalizedString(
        "certificateViewer.domainComponentTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Domain Component",
        comment:
          "Title of the section for the Domain Component such as: DNS or brave.com or a website's domain."
      )

    public static let userIDTitle =
      NSLocalizedString(
        "certificateViewer.userIDTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "User ID",
        comment: "Title of the section for the User's ID (Identifier)."
      )

    public static let noneTitle =
      NSLocalizedString(
        "certificateViewer.noneTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "None",
        comment: "Title indicating 'None' or no information or empty."
      )

    public static let parametersTitle =
      NSLocalizedString(
        "certificateViewer.parametersTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Parameters",
        comment: "Title indicating 'Parameters' or Input passed to a function/algorithm."
      )

    public static let encryptionTitle =
      NSLocalizedString(
        "certificateViewer.encryptionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Encryption",
        comment: "Title of the section indication which 'Encryption' algorithm was used."
      )

    public static let bytesUnitTitle =
      NSLocalizedString(
        "certificateViewer.bytesUnitTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "bytes",
        comment: "A measurement unit used in computing to indicate or how memory is used."
      )

    public static let bitsUnitTitle =
      NSLocalizedString(
        "certificateViewer.bitsUnitTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "bits",
        comment: "A measurement unit used in computing to indicate or how memory is used."
      )

    public static let encryptTitle =
      NSLocalizedString(
        "certificateViewer.encryptTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Encrypt",
        comment: "Title indicating whether or not a private key can be used to encrypt data"
      )

    public static let verifyTitle =
      NSLocalizedString(
        "certificateViewer.verifyTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Verify",
        comment:
          "Title indicating whether or not a private key can be used to verify data is legitimate"
      )

    public static let wrapTitle =
      NSLocalizedString(
        "certificateViewer.wrapTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Wrap",
        comment:
          "Title indicating whether or not a private key can be used to wrap or enclose some data"
      )

    public static let deriveTitle =
      NSLocalizedString(
        "certificateViewer.deriveTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Derive",
        comment:
          "Title indicating whether or not a private key can be used to derive some data from another piece of data. IE: Use one key to generate another"
      )

    public static let anyTitle =
      NSLocalizedString(
        "certificateViewer.anyTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Any",
        comment:
          "Title indicating whether or not a private key can be used for 'Anything' (encrypting, deriving, wrapping, verifying, signing, etc)"
      )

    public static let algorithmTitle =
      NSLocalizedString(
        "certificateViewer.algorithmTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Algorithm",
        comment: "Title indicating whether the section for the algorithm used"
      )

    public static let publicKeyTitle =
      NSLocalizedString(
        "certificateViewer.publicKeyTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Public Key",
        comment: "Title indicating whether the key used is public (not private)"
      )

    public static let exponentTitle =
      NSLocalizedString(
        "certificateViewer.exponentTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Exponent",
        comment: "Title indicating whether the mathematical exponent used. x³, x², etc."
      )

    public static let keySizeTitle =
      NSLocalizedString(
        "certificateViewer.keySizeTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Key Size",
        comment:
          "Title indicating the size of the private or public key used in bytes. Example: KeySize - 1024 Bytes. Key Size - 2048 Bytes."
      )

    public static let keyUsageTitle =
      NSLocalizedString(
        "certificateViewer.keyUsageTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Key Usage",
        comment: "Title indicating what the private or public key can be used for."
      )
  }
}

// MARK: - StoreKit Receipt Viewer

extension Strings {
  public struct ReceiptViewer {
    public static let vpnMonthlySubscriptionName =
      NSLocalizedString(
        "storekitReceiptViewer.vpnMonthlySubscriptionName",
        bundle: .module,
        value: "Brave VPN Monthly",
        comment: "The title of the product subscription the user purchased (Monthly subscription)"
      )

    public static let vpnYearlySubscriptionName =
      NSLocalizedString(
        "storekitReceiptViewer.vpnYearlySubscriptionName",
        bundle: .module,
        value: "Brave VPN Yearly",
        comment: "The title of the product subscription the user purchased (Yearly subscription)"
      )

    public static let leoMonthlySubscriptionName =
      NSLocalizedString(
        "storekitReceiptViewer.leoMonthlySubscriptionName",
        bundle: .module,
        value: "Brave Leo Monthly",
        comment: "The title of the product subscription the user purchased (Monthly subscription)"
      )

    public static let leoYearlySubscriptionName =
      NSLocalizedString(
        "storekitReceiptViewer.leoYearlySubscriptionName",
        bundle: .module,
        value: "Brave Leo Yearly",
        comment: "The title of the product subscription the user purchased (Yearly subscription)"
      )

    public static let receiptViewerTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptViewerTitle",
        bundle: .module,
        value: "App Store Receipt",
        comment: "The title of the screen that shows the App Store Receipt"
      )

    public static let noReceiptFoundTitle =
      NSLocalizedString(
        "storekitReceiptViewer.noReceiptFoundTitle",
        bundle: .module,
        value: "Sorry, no App Store receipts were found",
        comment: "The error message when the App Store Receipt was not found in the Application Bundle"
      )

    public static let receiptLoadingErrorTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptLoadingErrorTitle",
        bundle: .module,
        value: "Sorry, the App Store receipt could not be loaded",
        comment: "The error message when the App Store Receipt was found in the Application Bundle, but could not be loaded"
      )

    public static let applicationVersionTitle =
      NSLocalizedString(
        "storekitReceiptViewer.applicationVersionTitle",
        bundle: .module,
        value: "Application Version",
        comment: "The title of the label that shows the Application Version"
      )

    public static let receiptDateTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptDateTitle",
        bundle: .module,
        value: "Receipt Date",
        comment: "The title of the label that shows the date the receipt was created"
      )

    public static let receiptOrderIDTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptOrderIDTitle",
        bundle: .module,
        value: "Order ID",
        comment: "The title of the label that shows the Order ID of the user's purchase"
      )

    public static let receiptTransactionIDTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptTransactionIDTitle",
        bundle: .module,
        value: "Transaction ID",
        comment: "The title of the label that shows the Transaction ID of the user's purchase"
      )

    public static let receiptOriginalPurchaseDateTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptOriginalPurchaseDateTitle",
        bundle: .module,
        value: "Original Purchase Date",
        comment: "The title of the label that shows the date when the product subscription was first purchased"
      )

    public static let receiptPurchaseDate =
      NSLocalizedString(
        "storekitReceiptViewer.receiptPurchaseDate",
        bundle: .module,
        value: "Purchase Date",
        comment: "The title of the label that shows the date when the product subscription was recently purchased or renewed"
      )

    public static let receiptExpirationDate =
      NSLocalizedString(
        "storekitReceiptViewer.receiptExpirationDate",
        bundle: .module,
        value: "Expiration Date",
        comment: "The title of the label that shows the date when the product subscription expired"
      )

    public static let receiptCancellationDate =
      NSLocalizedString(
        "storekitReceiptViewer.receiptCancellationDate",
        bundle: .module,
        value: "Cancellation Date",
        comment: "The title of the label that shows the date when the product subscription was cancelled"
      )

    public static let receiptInvalidDate =
      NSLocalizedString(
        "storekitReceiptViewer.receiptInvalidDate",
        bundle: .module,
        value: "N/A",
        comment: "The title of the label that shows any invalid date - Not Available or N/A for short is displayed"
      )

    public static let receiptViewerLoadingTitle =
      NSLocalizedString(
        "storekitReceiptViewer.receiptViewerLoadingTitle",
        bundle: .module,
        value: "Loading Please Wait...",
        comment: "The title of the label that shows when the Receipt Viewer is still loading resources and subscription information"
      )

    public static let shareReceiptTitle =
      NSLocalizedString(
        "storekitReceiptViewer.shareReceiptTitle",
        bundle: .module,
        value: "Share Receipt",
        comment: "The title of the button that shows a system action sheet, to share the receipt"
      )
  }
}

// MARK: - Shortcuts

extension Strings {
  public struct Shortcuts {
    public static let activityTypeNewTabTitle =
      NSLocalizedString(
        "shortcuts.activityTypeNewTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open a New Browser Tab",
        comment: ""
      )

    public static let activityTypeNewPrivateTabTitle =
      NSLocalizedString(
        "shortcuts.activityTypeNewPrivateTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open a New Private Browser Tab",
        comment: ""
      )

    public static let activityTypeOpenBookmarksTitle =
      NSLocalizedString(
        "shortcuts.activityTypeOpenBookmarksTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave Browser Bookmarks",
        comment: ""
      )

    public static let activityTypeOpenHistoryListTitle =
      NSLocalizedString(
        "shortcuts.activityTypeOpenHistoryListTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave Browser History",
        comment: ""
      )

    public static let activityTypeClearHistoryTitle =
      NSLocalizedString(
        "shortcuts.activityTypeClearHistoryTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear Brave Browsing History",
        comment: ""
      )

    public static let activityTypeEnableVPNTitle =
      NSLocalizedString(
        "shortcuts.activityTypeEnableVPNTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave Browser and Enable VPN",
        comment: ""
      )

    public static let activityTypeOpenBraveNewsTitle =
      NSLocalizedString(
        "shortcuts.activityTypeOpenBraveNewsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave News",
        comment: ""
      )

    public static let activityTypeOpenPlaylistTitle =
      NSLocalizedString(
        "shortcuts.activityTypeOpenPlaylistTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Playlist",
        comment: ""
      )

    public static let activityTypeOpenSyncedTabsTitle =
      NSLocalizedString(
        "shortcuts.activityTypeOpenSyncedTabsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Tabs from Other Devices",
        comment: ""
      )

    public static let activityTypeTabDescription =
      NSLocalizedString(
        "shortcuts.activityTypeTabDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Start Searching the Web Securely with Brave",
        comment: ""
      )

    public static let activityTypeOpenBookmarksDescription =
      NSLocalizedString(
        "shortcuts.activityTypeOpenBookmarksDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser Bookmarks Screen",
        comment: ""
      )

    public static let activityTypeOpenHistoryListDescription =
      NSLocalizedString(
        "shortcuts.activityTypeOpenHistoryListDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser History Screen",
        comment: ""
      )

    public static let activityTypeClearHistoryDescription =
      NSLocalizedString(
        "shortcuts.activityTypeClearHistoryDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser in a New Tab and Delete All Private Browser History Data",
        comment: ""
      )

    public static let activityTypeEnableVPNDescription =
      NSLocalizedString(
        "shortcuts.activityTypeEnableVPNDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser in a New Tab and Enable VPN",
        comment: ""
      )

    public static let activityTypeBraveNewsDescription =
      NSLocalizedString(
        "shortcuts.activityTypeBraveNewsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave News and Check Today's Top Stories",
        comment: ""
      )

    public static let activityTypeOpenPlaylistDescription =
      NSLocalizedString(
        "shortcuts.activityTypeOpenPlaylistDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Start Playing your Videos in Playlist",
        comment: ""
      )

    public static let activityTypeOpenSyncedTabsDescription =
      NSLocalizedString(
        "shortcuts.activityTypeOpenSyncedTabsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave Tabs Open in Other Devices",
        comment: ""
      )

    public static let activityTypeNewTabSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeNewTabSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open New Tab",
        comment: ""
      )

    public static let activityTypeNewPrivateTabSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeNewPrivateTabSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open New Private Tab",
        comment: ""
      )

    public static let activityTypeOpenBookmarksSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeOpenBookmarksSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Bookmarks",
        comment: ""
      )

    public static let activityTypeOpenHistoryListSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeOpenHistoryListSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser History",
        comment: ""
      )

    public static let activityTypeClearHistorySuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeClearHistorySuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear Browser History",
        comment: ""
      )

    public static let activityTypeEnableVPNSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeEnableVPNSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable VPN",
        comment: ""
      )

    public static let activityTypeOpenBraveNewsSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeOpenBraveTodaySuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave News",
        comment: ""
      )

    public static let activityTypeOpenPlaylistSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeOpenPlaylistSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Playlist",
        comment: ""
      )

    public static let activityTypeOpenSyncedTabsSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.activityTypeOpenSyncedTabsSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Tabs from Other Devices",
        comment: ""
      )

    public static let customIntentOpenWebsiteSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.customIntentOpenWebsiteSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Website",
        comment: ""
      )

    public static let customIntentOpenHistorySuggestedPhrase =
      NSLocalizedString(
        "shortcuts.customIntentOpenHistorySuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open History Website",
        comment: ""
      )

    public static let customIntentOpenBookmarkSuggestedPhrase =
      NSLocalizedString(
        "shortcuts.customIntentOpenBookmarkSuggestedPhrase",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Bookmark Website",
        comment: ""
      )

    public static let shortcutSettingsTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Siri Shortcuts",
        comment: ""
      )

    public static let shortcutSettingsOpenNewTabTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenNewTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open New Tab",
        comment: ""
      )

    public static let shortcutSettingsOpenNewTabDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenNewTabDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open a new tab via Siri - Voice Assistant",
        comment: ""
      )

    public static let shortcutSettingsOpenNewPrivateTabTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenNewPrivateTabTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open New Private Tab",
        comment: ""
      )

    public static let shortcutSettingsOpenNewPrivateTabDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenNewPrivateTabDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open a new private tab via Siri - Voice Assistant",
        comment: ""
      )

    public static let shortcutSettingsOpenBookmarksTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenBookmarksTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Bookmarks",
        comment: ""
      )

    public static let shortcutSettingsOpenBookmarksDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenBookmarksDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open bookmarks screen via Siri - Voice Assistant",
        comment: ""
      )

    public static let shortcutSettingsOpenHistoryListTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenHistoryListTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Browser History",
        comment: ""
      )

    public static let shortcutSettingsOpenHistoryListDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenHistoryListDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open browser history screen via Siri - Voice Assistant",
        comment: ""
      )

    public static let shortcutSettingsClearBrowserHistoryTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsClearBrowserHistoryTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear Browser History",
        comment: ""
      )

    public static let shortcutSettingsClearBrowserHistoryDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsClearBrowserHistoryDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open a new tab & clear browser history via Siri - Voice Assistant",
        comment: "Description of Clear Browser History Siri Shortcut in Settings Screen"
      )

    public static let shortcutSettingsEnableVPNTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsEnableVPNTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable VPN",
        comment: ""
      )

    public static let shortcutSettingsEnableVPNDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsEnableVPNDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to enable Brave VPN via Siri - Voice Assistant",
        comment: ""
      )

    public static let shortcutSettingsOpenBraveNewsTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenBraveNewsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Brave News",
        comment: ""
      )

    public static let shortcutSettingsOpenBraveNewsDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenBraveNewsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open a new tab & show Brave News Feed via Siri - Voice Assistant",
        comment: "Description of Open Brave News Siri Shortcut in Settings Screen"
      )

    public static let shortcutSettingsOpenPlaylistTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenPlaylistTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Playlist",
        comment: ""
      )

    public static let shortcutSettingsOpenPlaylistDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenPlaylistDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open Playlist via Siri - Voice Assistant",
        comment: "Description of Open Playlist Siri Shortcut in Settings Screen"
      )

    public static let shortcutSettingsOpenSyncedTabsTitle =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenSyncedTabsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Tabs from Other Devices",
        comment: ""
      )

    public static let shortcutSettingsOpenSyncedTabsDescription =
      NSLocalizedString(
        "shortcuts.shortcutSettingsOpenSyncedTabsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use Shortcuts to open Tabs from Other Devices via Siri  - Voice Assistant",
        comment: "Description of Open Tabs from Other Devices Shortcut in Settings Screen"
      )

    public static let shortcutOpenApplicationSettingsTitle =
      NSLocalizedString(
        "shortcuts.shortcutOpenApplicationSettingsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open Settings",
        comment: "Button title that open application settings"
      )

    public static let shortcutOpenApplicationSettingsDescription =
      NSLocalizedString(
        "shortcuts.shortcutOpenApplicationSettingsDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This option will open Brave Settings. In order to change various Siri options, please select 'Siri & Search' menu item and customize your choices.",
        comment: "Description for opening Brave Settings for altering Siri shortcut."
      )
  }
}

// MARK: - VPN
extension Strings {
  public struct VPN {
    public static let vpnName =
      NSLocalizedString(
        "vpn.buyVPNTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Firewall + VPN",
        comment: "Title for screen to buy the VPN."
      )

    public static let poweredBy =
      NSLocalizedString(
        "vpn.poweredBy",
        tableName: "BraveShared",
        bundle: .module,
        value: "Powered by",
        comment: "It is used in context: 'Powered by BRAND_NAME'"
      )

    public static let updateActionCellTitle =
      NSLocalizedString(
        "vpn.updateActionCellTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Update",
        comment: "Cell title indicates update payment method"
      )

    public static let settingsVPNEnabled =
      NSLocalizedString(
        "vpn.settingsVPNEnabled",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enabled",
        comment: "Whether the VPN is enabled or not"
      )

    public static let errorCantGetPricesTitle =
      NSLocalizedString(
        "vpn.errorCantGetPricesTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "App Store Error",
        comment: "Title for an alert when the VPN can't get prices from the App Store"
      )

    public static let errorCantGetPricesBody =
      NSLocalizedString(
        "vpn.errorCantGetPricesBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Could not connect to the App Store, please try again in few minutes.",
        comment: "Message for an alert when the VPN can't get prices from the App Store"
      )

    public static let settingsVPNDisabled =
      NSLocalizedString(
        "vpn.settingsVPNDisabled",
        tableName: "BraveShared",
        bundle: .module,
        value: "Disabled",
        comment: "Whether the VPN is enabled or not"
      )

    public static let popupCheckboxBlockAds =
      NSLocalizedString(
        "vpn.popupCheckboxBlockAds",
        tableName: "BraveShared",
        bundle: .module,
        value: "Blocks unwanted network connections",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let popupCheckboxBlockAdsAlternate =
      NSLocalizedString(
        "vpn.popupCheckboxBlockAdsAlternate",
        tableName: "BraveShared",
        bundle: .module,
        value: "Block ads & trackers across all apps ",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let popupCheckboxFast =
      NSLocalizedString(
        "vpn.popupCheckboxFast",
        tableName: "BraveShared",
        bundle: .module,
        value: "Supports speeds of up to 100 Mbps",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let popupCheckboxFastAlternate =
      NSLocalizedString(
        "vpn.popupCheckboxFastAlternate",
        tableName: "BraveShared",
        bundle: .module,
        value: "Fast and unlimited up to 100 Mbps",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let popupCheckmarkSecureConnections =
      NSLocalizedString(
        "vpn.popupCheckmarkSecureConnections",
        tableName: "BraveShared",
        bundle: .module,
        value: "Secures all connections",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let popupCheckmark247Support =
      NSLocalizedString(
        "vpn.popupCheckmark247Support",
        tableName: "BraveShared",
        bundle: .module,
        value: "24/7 support",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )
    
    public static let infoCheckPrivacy =
      NSLocalizedString(
        "vpn.infoCheckPrivacy",
        tableName: "BraveShared",
        bundle: .module,
        value: "Extra privacy & security online",
        comment: "Information text shown to users for using Brave VPN"
      )

    public static let infoCheckLocation =
      NSLocalizedString(
        "vpn.infoCheckLocation",
        tableName: "BraveShared",
        bundle: .module,
        value: "Hide your IP & change your location",
        comment: "Information Text shown to users for using Brave VPN"
      )

    public static let infoCheckServers =
      NSLocalizedString(
        "vpn.infoCheckServers",
        tableName: "BraveShared",
        bundle: .module,
        value: "Hundreds of servers around the world",
        comment: "Information Text shown to users for using Brave VPN"
      )
    
    public static let infoCheckConnectionSpeed =
      NSLocalizedString(
        "vpn.infoCheckConnectionSpeed",
        tableName: "BraveShared",
        bundle: .module,
        value: "Lightning-fast connection speeds",
        comment: "Information shown to users for using Brave VPN"
      )
    
    public static let infoCheckLimitDevice =
      NSLocalizedString(
        "vpn.infoCheckLimitDevice",
        tableName: "BraveShared",
        bundle: .module,
        value: "Protect up to 10 devices with one plan",
        comment: "Information shown to users for using Brave VPN"
      )
    
    public static let autoRenewSoonExpirePopOverTitle =
      NSLocalizedString(
        "vpn.autoRenewSoonExpireTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Oh no! Your Brave VPN subscription is about to expire.",
        comment: "Pop up title for VPN subscription is about expire"
      )

    public static let autoRenewDiscountPopOverTitle =
      NSLocalizedString(
        "vpn.autoRenewDiscountPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Auto-renew your Brave VPN Subscription now and get 20% off for 3 months!",
        comment: "Pop up title for renewing VPN subscription with discount"
      )

    public static let autoRenewFreeMonthPopOverTitle =
      NSLocalizedString(
        "vpn.autoRenewFreeMonthPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Auto-renew your Brave VPN Subscription now and get 1 month free!",
        comment: "Pop up title for renewing VPN subscription with month free"
      )

    public static let updateBillingSoonExpirePopOverTitle =
      NSLocalizedString(
        "vpn.updateBillingSoonExpirePopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "There's a billing issue with your account, which means your Brave VPN subscription is about to expire.",
        comment: "Pop up title for billing issue for subcription VPN about to expire"
      )

    public static let updateBillingExpiredPopOverTitle =
      NSLocalizedString(
        "vpn.updateBillingExpiredPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Update your payment info to stay protected with Brave VPN.",
        comment: "Pop up title for billing issue for subcription VPN already expired"
      )

    public static let autoRenewSoonExpirePopOverDescription =
      NSLocalizedString(
        "vpn.autoRenewSoonExpirePopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "That means you'll lose Brave's extra protections for every app on your phone.",
        comment: "Pop up description for VPN subscription is about expire"
      )

    public static let updateBillingSoonExpirePopOverDescription =
      NSLocalizedString(
        "vpn.updateBillingSoonExpirePopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Want to keep protecting every app on your phone? Just update your payment details.",
        comment: "Pop up description for billing issue of subcription VPN about to expire"
      )

    public static let updateBillingExpiredPopOverDescription =
      NSLocalizedString(
        "vpn.updateBillingExpiredPopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Don’t worry. We’ll keep VPN active for a few days while you are updating your payment info.",
        comment: "Pop up description for billing issue of subcription VPN already expired"
      )

    public static let subscribeVPNDiscountPopOverTitle =
      NSLocalizedString(
        "vpn.subscribeVPNDiscountPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Give Brave VPN another try and get 20% off for 3 months!",
        comment: "Pop up title for subscribing VPN with discount"
      )

    public static let subscribeVPNProtectionPopOverTitle =
      NSLocalizedString(
        "vpn.subscribeVPNProtectionPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Did you know that Brave VPN protects you outside of Brave Browser?",
        comment: "Pop up title for subscribing VPN explaning VPN protects user outside the Brave"
      )

    public static let subscribeVPNAllDevicesPopOverTitle =
      NSLocalizedString(
        "vpn.subscribeVPNAllDevicesPopOverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Now, use Brave VPN on all your devices for the same price!",
        comment: "Pop up title the subscription for VPN can be used for all platforms"
      )

    public static let subscribeVPNProtectionPopOverDescription =
      NSLocalizedString(
        "vpn.subscribeVPNProtectionPopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Brave VPN has always blocked trackers on every app, even outside the Brave browser. Now you can see who tried to track you, with the Brave Privacy Hub.",
        comment:
          "Pop up description for subscribing VPN explaning VPN protects user outside the Brave"
      )

    public static let subscribeVPNAllDevicesPopOverDescription =
      NSLocalizedString(
        "vpn.subscribeVPNAllDevicesPopOverDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "That’s right. Your Brave VPN subscription is now good on up to 5 devices. So you can subscribe on iOS and use it on your Mac, Windows and Android devices for free.",
        comment: "Pop up description the subscription for VPN can be used for all platforms"
      )

    public static let subscribeVPNPopOverSubDescription =
      NSLocalizedString(
        "vpn.subscribeVPNPopOverSubDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Ready to safeguard every app on your phone? Come back to Brave VPN and get 20% off for the next 3 months.",
        comment: "Pop up sub description the subscription for VPN can be used for all platforms"
      )

    public static let sessionExpiredTitle =
      NSLocalizedString(
        "vpn.sessionExpiredTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Session expired",
        comment: "Alert title to show when the VPN session has expired"
      )

    public static let sessionExpiredDescription =
      NSLocalizedString(
        "vpn.sessionExpiredDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Please login to your Brave Account to refresh your VPN session.",
        comment: "Alert description to show when the VPN session has expired"
      )

    public static let sessionExpiredLoginButton =
      NSLocalizedString(
        "vpn.sessionExpiredLoginButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Login",
        comment: "Login button to fix the vpn session expiration issue"
      )

    public static let sessionExpiredDismissButton =
      NSLocalizedString(
        "vpn.sessionExpiredDismissButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Dismiss",
        comment: "Dismiss button to close the alert showing that the vpn session has expired"
      )

    public static let vpnRegionSelectorButtonSubTitle =
      NSLocalizedString(
        "vpn.vpnRegionSelectorButtonSubTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Current Setting: %@",
        comment:
          "Button subtitle for VPN region selection in menu. %@ will be replaced with country name or automatic ex: Current Setting: Automatic"
      )

    public static let vpnUpdatePaymentMethodDescriptionText =
      NSLocalizedString(
        "vpn.vpnUpdatePaymentMethodDescriptionText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Please update your payment method",
        comment: "Text description of VPN subscription needs a method of payment update"
      )

    public static let settingsVPNExpired =
      NSLocalizedString(
        "vpn.settingsVPNExpired",
        tableName: "BraveShared",
        bundle: .module,
        value: "Expired",
        comment: "Whether the VPN plan has expired"
      )

    public static let vpnRegionSelectorButtonTitle =
      NSLocalizedString(
        "vpn.vpnRegionSelectorButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN Region",
        comment: "Button title for VPN region selection in menu"
      )

    public static let regionPickerAutomaticModeCellText =
      NSLocalizedString(
        "vpn.regionPickerAutomaticModeCellText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Automatic",
        comment: "Name of automatic vpn region selector"
      )

    public static let autoRenewActionButtonTitle =
      NSLocalizedString(
        "vpn.autoRenewActionButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable Auto-Renew",
        comment: "Action button title that enables auto renew for subcription"
      )

    public static let autoReneSoonExpirePopOverSubDescription =
      NSLocalizedString(
        "vpn.autoReneSoonExpirePopOverSubDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Want to stay protected? Just renew before your subscription ends. As a thanks for renewing, we'll even take 20% off for the next 3 months.",
        comment: "Pop up extra description for billing issue of subcription VPN about to expire"
      )

    public static let updatePaymentActionButtonTitle =
      NSLocalizedString(
        "vpn.updatePaymentActionButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Update Payment",
        comment: "Action button title that updates method payment"
      )

    public static let subscribeVPNActionButtonTitle =
      NSLocalizedString(
        "vpn.subscribeVPNActionButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Subscribe Now",
        comment: "Action button title that subscribe action for VPN purchase"
      )

    public static let vpnRegionListServerScreenTitle =
      NSLocalizedString(
        "vpn.vpnRegionListServerScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Server Region",
        comment: "The title for screen for showing list of server region"
      )
  }
}

extension Strings {
  public struct History {
    public static let historyClearAlertTitle =
      NSLocalizedString(
        "history.historyClearAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear Browsing History",
        comment: "Title for Clear All History Alert Title"
      )
    public static let historyClearAlertDescription =
      NSLocalizedString(
        "history.historyClearAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "This will clear all browsing history.",
        comment: "Description for Clear All History Alert Description"
      )
    public static let historyClearActionTitle =
      NSLocalizedString(
        "history.historyClearActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear History",
        comment: "Title for History Clear All Action"
      )

    public static let historyEmptyStateTitle =
      NSLocalizedString(
        "history.historyEmptyStateTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "History will show up here.",
        comment: "Title which is displayed when History screen is empty."
      )

    public static let historyPrivateModeOnlyStateTitle =
      NSLocalizedString(
        "history.historyPrivateModeOnlyStateTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "History is not available in Private Browsing Only mode.",
        comment:
          "Title which is displayed on History screen as a overlay when Private Browsing Only enabled"
      )
    public static let historySearchBarTitle =
      NSLocalizedString(
        "history.historySearchBarTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Search History",
        comment: "Title displayed for placeholder inside Search Bar in History"
      )
  }
}

extension Strings {
  public struct Privacy {
    public static let browserLock =
      NSLocalizedString(
        "BrowserLock",
        tableName: "BraveShared",
        bundle: .module,
        value: "Browser Lock",
        comment: "Title for setting to enable the browser lock privacy feature"
      )
    public static let browserLockDescription =
      NSLocalizedString(
        "BrowserLockDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Unlock Brave with Touch ID, Face ID or system passcode.",
        comment: "Description for setting to enable the browser lock privacy feature"
      )
    public static let tabTraySetPasscodeAlertDescription =
      NSLocalizedString(
        "privacy.tab.tray.passcode.alert",
        tableName: "BraveShared",
        bundle: .module,
        value: "To switch private browsing mode, you must first set a passcode on your device.",
        comment: "The message displayed in alert when a user needs to set a passcode"
      )
  }
}

extension Strings {
  public struct TabsSettings {
    public static let privateTabsSettingsTitle =
      NSLocalizedString(
        "tabs.settings.privateTabsSettingsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Private Tabs",
        comment: ""
      )
    public static let privateBrowsingLockTitleFaceID =
      NSLocalizedString(
        "tabs.settings.privateBrowsingLockTitleFaceID",
        tableName: "BraveShared",
        bundle: .module,
        value: "Require Face ID",
        comment: ""
      )
    public static let privateBrowsingLockTitleTouchID =
      NSLocalizedString(
        "tabs.settings.privateBrowsingLockTitleTouchID",
        tableName: "BraveShared",
        bundle: .module,
        value: "Require Touch ID",
        comment: ""
      )
    public static let privateBrowsingLockTitlePinCode =
      NSLocalizedString(
        "tabs.settings.privateBrowsingLockTitlePinCode",
        tableName: "BraveShared",
        bundle: .module,
        value: "Require Pin Code",
        comment: ""
      )
    public static let persistentPrivateBrowsingTitle =
      NSLocalizedString(
        "tabs.settings.persistentPrivateBrowsingTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Keep Private Tabs",
        comment: ""
      )
    public static let persistentPrivateBrowsingDescription =
      NSLocalizedString(
        "tabs.settings.persistentPrivateBrowsingDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Keep private browsing tabs open when you close the app, ensuring private browsing sessions continue seamlessly.",
        comment: ""
      )
  }
}

extension Strings {
  public struct Login {
    public static let loginListEmptyScreenTitle =
      NSLocalizedString(
        "login.loginListEmptyScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "No logins found",
        comment:
          "The message displayed on the password list screen when there is not password found"
      )
    public static let loginListNavigationTitle =
      NSLocalizedString(
        "login.loginListNavigationTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Logins & Passwords",
        comment: "Title for navigation bar of the login list screen"
      )
    public static let loginListSearchBarPlaceHolderTitle =
      NSLocalizedString(
        "login.loginListSearchBarPlaceHolderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Filter",
        comment: "The text for placeholder inside search bar in login list"
      )
    public static let loginListSavedLoginsHeaderTitle =
      NSLocalizedString(
        "login.loginListSavedLoginsHeaderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Saved Logins",
        comment: "The header title displayed over the login list"
      )
    public static let loginListNeverSavedHeaderTitle =
      NSLocalizedString(
        "login.loginListNeverSavedHeaderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Never Saved",
        comment: "The header title displayed over the never saved login list entry"
      )
    public static let loginInfoDetailsHeaderTitle =
      NSLocalizedString(
        "login.loginInfoDetailsHeaderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Login Details",
        comment: "The header title displayed over the details of login entry"
      )
    public static let loginInfoDetailsWebsiteFieldTitle =
      NSLocalizedString(
        "login.loginInfoDetailsWebsiteFieldTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Website",
        comment: "Title for the website field in password detail page"
      )
    public static let loginInfoDetailsUsernameFieldTitle =
      NSLocalizedString(
        "login.loginInfoDetailsUsernameFieldTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Username",
        comment: "Title for the username field in password detail page"
      )
    public static let loginInfoDetailsPasswordFieldTitle =
      NSLocalizedString(
        "login.loginInfoDetailsPasswordFieldTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Password",
        comment: "Title for the password field in password detail page"
      )
    public static let loginEntryDeleteAlertMessage =
      NSLocalizedString(
        "login.loginEntryDeleteAlertMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Saved Login will be removed permanently.",
        comment: "The message displayed in alert when a login entry deleted"
      )
    public static let loginInfoCreatedHeaderTitle =
      NSLocalizedString(
        "login.loginInfoCreatedHeaderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Created %@",
        comment: "The message displayed in alert when a login entry deleted"
      )
    public static let loginInfoSetPasscodeAlertTitle =
      NSLocalizedString(
        "login.loginInfoSetPasscodeAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Set a Passcode",
        comment: "The title displayed in alert when a user needs to set passcode"
      )
    public static let loginInfoSetPasscodeAlertDescription =
      NSLocalizedString(
        "login.loginInfoSetPasscodeAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "To see passwords, you must first set a passcode on your device.",
        comment: "The message displayed in alert when a user needs to set a passcode"
      )
  }
}

extension Strings {
  public struct OpenTabs {
    public static let sendWebpageScreenTitle =
      NSLocalizedString(
        "openTabs.sendWebpageScreenTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Send Web Page",
        comment: "The title displayed on send-webpage screen"
      )
    public static let sendDeviceButtonTitle =
      NSLocalizedString(
        "opentabs.sendDeviceButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Send To Your Device",
        comment: "Title action button for sending webpage to selected other device"
      )
    public static let openTabsItemLastSyncedTodayTitle =
      NSLocalizedString(
        "opentabs.openTabsItemLastSyncedTodayTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Today %@",
        comment:
          "The description indicating when is the open tab synced. The parameter substituted for \"%@\" is the actual string representation of the exact time. E.g.: Today 12:00 PM"
      )
    public static let openTabsItemLastSyncedYesterdayTitle =
      NSLocalizedString(
        "opentabs.openTabsItemLastSyncedYesterdayTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Yesterday %@",
        comment:
          "The description indicating when is the open tab synced. The parameter substituted for \"%@\" is the actual string representation of the exact time. E.g.: Yesterday 12:00 PM"
      )
    public static let openTabsItemLastSyncedLastWeekTitle =
      NSLocalizedString(
        "opentabs.openTabsItemLastSyncedLastWeekTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Last Week %@",
        comment:
          "The description indicating when is the open tab synced. The parameter substituted for \"%@\" is the actual string representation of the exact day of the week and exact time. E.g.: Last Week Monday 12:00 PM"
      )
    public static let activePeriodDeviceTodayTitle =
      NSLocalizedString(
        "opentabs.activePeriodDeviceTodayTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Active Today",
        comment: "The description indicating the device is active last time today"
      )
    public static let activePeriodDeviceYesterdayTitle =
      NSLocalizedString(
        "opentabs.activePeriodDeviceYesterdayTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Active Yesterday",
        comment: "The description indicating the device is active last time yesterday"
      )
    public static let activePeriodDeviceThisWeekTitle =
      NSLocalizedString(
        "opentabs.activePeriodDeviceThisWeekTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Active Last Week",
        comment: "The description indicating the device is active last time this week"
      )
    public static let activePeriodDeviceThisMonthTitle =
      NSLocalizedString(
        "opentabs.activePeriodDeviceThisMonthTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Active Last Month",
        comment: "The description indicating the device is active last time this month"
      )
    public static let activePeriodDeviceDaysAgoTitle =
      NSLocalizedString(
        "opentabs.activePeriodDeviceDaysAgoTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Active %ld Days Ago",
        comment:
          "The description indicating the device is active # of days ago. The parameter substituted for \"%ld\" is the number of days. E.g.: Active 123 Days Ago"
      )
    public static let tabTrayOpenTabSearchBarTitle =
      NSLocalizedString(
        "opentabs.tabTrayOpenSearchBarTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Search Tabs from Other Devices",
        comment: "The placeholder written in seach bar while tab-tray is showing open tabs."
      )
    public static let sendWebsiteShareActionTitle =
      NSLocalizedString(
        "opentabs.sendWebsiteShareActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Send To Your Devices",
        comment: "Share action title for sending e url to another device"
      )
    public static let noSyncSessionPlaceHolderViewTitle =
      NSLocalizedString(
        "opentabs.noSyncSessionPlaceHolderViewTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "This space shows Brave tabs open in other devices",
        comment: "The title of the view showing no synced sessions"
      )
    public static let noSyncChainPlaceHolderViewDescription =
      NSLocalizedString(
        "opentabs.noSyncChainPlaceHolderViewDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "To get started, set up a sync chain and toggle \"Open Tabs\" in Sync Settings.",
        comment: "The description of the view explaining user should join a sync chain."
      )
    public static let enableOpenTabsPlaceHolderViewDescription =
      NSLocalizedString(
        "opentabs.enableOpenTabsPlaceHolderViewDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable tab syncing to see tabs from your other devices.",
        comment: "The description of the view describing tab syncing should be enabled."
      )
    public static let noSyncSessionPlaceHolderViewDescription =
      NSLocalizedString(
        "opentabs.noSyncSessionPlaceHolderViewDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "There are no active sessions to show from your other devices.",
        comment: "The description of the view describing no other sessions are active in sync."
      )
    public static let syncChainStartButtonTitle =
      NSLocalizedString(
        "opentabs.startASyncChainButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Start a sync chain",
        comment: "The button which navigates to screen starting a new sync chain"
      )
    public static let tabSyncEnableButtonTitle =
      NSLocalizedString(
        "opentabs.tabSyncEnableButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable tab syncing",
        comment:
          "The button which navigates to screen where Tab Sync functionality toggle can be adjusted"
      )
    public static let showSettingsSyncButtonTitle =
      NSLocalizedString(
        "opentabs.showSettingsSyncButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Show Sync Settings",
        comment: "The button which navigates to screen where user can view sync settings"
      )
    public static let noSyncSessionPlaceHolderViewAdditionalDescription =
      NSLocalizedString(
        "opentabs.noSyncSessionPlaceHolderViewAdditionalDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Manage what Brave syncs in Settings.",
        comment: "The additional description of the view describing tab syncing should be enabled."
      )
    public static let noDevicesSyncChainPlaceholderViewTitle =
      NSLocalizedString(
        "opentabs.noDevicesSyncChainPlaceholderViewTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "No Devices in this sync chain",
        comment: "The title of the view showing no devices inside sync chain"
      )
    public static let noDevicesSyncChainPlaceholderViewDescription =
      NSLocalizedString(
        "opentabs.noDevicesSyncChainPlaceholderViewDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "To see your devices and enabled sync types, join a sync chain",
        comment: "The description of the view showing no devices inside sync chain"
      )
    public static let sendingWebpageProgressTitle =
      NSLocalizedString(
        "openTabs.sendingWebpageProgressTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sending…",
        comment: "The title displayed on screen showing webpage is being sent"
      )
    public static let sendingWebpageCompletedTitle =
      NSLocalizedString(
        "openTabs.sendingWebpageCompletedTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sent!",
        comment: "The title displayed on screen showing sending page is already completed"
      )
    public static let openTabsListTableHeaderTitle =
      NSLocalizedString(
        "openTabs.openTabsListTableHeaderTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Devices",
        comment: "The title displayed on table header for open tabs list from other devices"
      )
    public static let openSessionOpenAllActionTitle =
      NSLocalizedString(
        "opentabs.openSessionOpenAllActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open All",
        comment: "The title for the action to open all the tabs in a synced open session"
      )
    public static let openSessionHideAllActionTitle =
      NSLocalizedString(
        "opentabs.openSessionHideAllActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Hide for now",
        comment: "The title for the action to hide open session with all tabs"
      )
  }
}

extension Strings {
  public struct BraveNews {
    public static let braveNews = NSLocalizedString(
      "today.braveToday",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave News",
      comment: "The name of the feature"
    )
    public static let braveNewsTitle = NSLocalizedString(
      "today.braveToday.title",
      tableName: "BraveShared",
      bundle: .module,
      value: "News",
      comment: "The name of the feature in settings"
    )
    public static let sourcesAndSettings = NSLocalizedString(
      "today.sourcesAndSettings",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sources & Settings",
      comment: ""
    )
    public static let turnOnBraveNews = NSLocalizedString(
      "today.turnOnBraveToday",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn on Brave News",
      comment: ""
    )
    public static let learnMoreTitle = NSLocalizedString(
      "today.learnMoreTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Learn more about your data",
      comment: ""
    )
    public static let introCardTitle = NSLocalizedString(
      "today.introCardTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn on Brave News, and never miss a story",
      comment: "Shown above a button that turns on the \"Brave News\" product in the app."
    )
    public static let introCardBody = NSLocalizedString(
      "today.introCardBody",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Follow your favorite sources, in a single feed. Just open a tab in Brave, scroll down, and… voila!\nBrave News is ad-supported with private, anonymized ads.",
      comment:
        "Shown above a button that turns on the \"Brave News\" product in the app. by sources & feeds they mean websites & blogs."
    )
    public static let refresh = NSLocalizedString(
      "today.refresh",
      tableName: "BraveShared",
      bundle: .module,
      value: "Refresh",
      comment: ""
    )
    public static let emptyFeedTitle = NSLocalizedString(
      "today.emptyFeedTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "No articles to show",
      comment: ""
    )
    public static let emptyFeedBody = NSLocalizedString(
      "today.emptyFeedBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Try turning on some news sources",
      comment: ""
    )
    public static let deals = NSLocalizedString(
      "today.deals",
      tableName: "BraveShared",
      bundle: .module,
      value: "Deals",
      comment: ""
    )
    public static let errorNoInternetTitle = NSLocalizedString(
      "today.noInternet",
      tableName: "BraveShared",
      bundle: .module,
      value: "No Internet",
      comment: ""
    )
    public static let errorNoInternetBody = NSLocalizedString(
      "today.noInternetBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Try checking your connection or reconnecting to Wi-Fi.",
      comment: ""
    )
    public static let errorGeneralTitle = NSLocalizedString(
      "today.errorGeneralTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Oops…",
      comment: ""
    )
    public static let errorGeneralBody = NSLocalizedString(
      "today.errorGeneralBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave News is experiencing some issues. Try again.",
      comment: ""
    )
    public static let disablePublisherContent = NSLocalizedString(
      "today.disablePublisherContent",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hide content from %@",
      comment: "'%@' will turn into the name of a publisher (verbatim), for example: Brave Blog"
    )
    public static let enablePublisherContent = NSLocalizedString(
      "today.enablePublisherContent",
      tableName: "BraveShared",
      bundle: .module,
      value: "Allow content from %@",
      comment: "'%@' will turn into the name of a publisher (verbatim), for example: Brave Blog"
    )
    public static let disabledAlertTitle = NSLocalizedString(
      "today.disabledAlertTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Disabled",
      comment: ""
    )
    public static let disabledAlertBody = NSLocalizedString(
      "today.disabledAlertBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave News will stop showing content from %@",
      comment: "'%@' will turn into the name of a publisher (verbatim), for example: Brave Blog"
    )
    public static let isEnabledToggleLabel = NSLocalizedString(
      "today.isEnabledToggleLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show Brave News",
      comment: ""
    )
    public static let contentAvailableButtonTitle = NSLocalizedString(
      "today.contentAvailableButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "New Content Available",
      comment: ""
    )
    public static let moreBraveOffers = NSLocalizedString(
      "today.moreBraveOffers",
      tableName: "BraveShared",
      bundle: .module,
      value: "More Brave Offers",
      comment: "'Brave Offers' is a product name"
    )
    public static let promoted = NSLocalizedString(
      "today.promoted",
      tableName: "BraveShared",
      bundle: .module,
      value: "Promoted",
      comment: "A button title that is placed on promoted cards"
    )
    public static let addSourceShareTitle = NSLocalizedString(
      "today.addSourceShareTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add Source to Brave News",
      comment: "The action title displayed in the iOS share menu"
    )
    public static let addSourceFailureTitle = NSLocalizedString(
      "today.addSourceFailureTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Failed to Add Source",
      comment: "The title in the alert when a source fails to add"
    )
    public static let addSourceNetworkFailureMessage = NSLocalizedString(
      "today.addSourceNetworkFailureMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sorry, we couldn’t find that feed address.",
      comment: ""
    )
    public static let addSourceInvalidDataMessage = NSLocalizedString(
      "today.addSourceInvalidDataMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sorry, we couldn’t recognize that feed address.",
      comment: ""
    )
    public static let addSourceNoFeedsFoundMessage = NSLocalizedString(
      "today.addSourceNoFeedsFoundMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sorry, that feed address doesn’t have any content.",
      comment: ""
    )
    public static let addSourceAddButtonTitle = NSLocalizedString(
      "today.addSourceAddButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add",
      comment: "To add a list of 1 or more rss feeds"
    )
    public static let insecureSourcesHeader = NSLocalizedString(
      "today.insecureSourcesHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Insecure Sources - Add at your own risk",
      comment: "The header above the list of insecure sources"
    )
    public static let importOPML = NSLocalizedString(
      "today.importOPML",
      tableName: "BraveShared",
      bundle: .module,
      value: "Import OPML",
      comment: "\"OPML\" is a file extension that contains a list of rss feeds."
    )
    public static let sourcesHeaderTitle = NSLocalizedString(
      "today.sourcesHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sources",
      comment: "A header title shown over a list of online news sources (i.e. CNN, NYT, etc.)"
    )
    public static let channelsHeaderTitle = NSLocalizedString(
      "today.channelsHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Channels",
      comment: "A header title shown over a list of channels (i.e. Cars, Business, Fashion, etc.)"
    )
    public static let userSourcesHeaderTitle = NSLocalizedString(
      "today.userSourcesHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "User Sources",
      comment:
        "A header title shown over a list of news sources the user has subscribed to manually via RSS."
    )
    public static let followingTitle = NSLocalizedString(
      "today.followingTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Following",
      comment:
        "A title at the top of the screen that shows a list of the news content the user is following"
    )
    public static let suggestedTitle = NSLocalizedString(
      "today.suggestedTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Suggested",
      comment:
        "A title at the top of the screen that shows a list of the news content the user is being suggested"
    )
    public static let channelsTitle = NSLocalizedString(
      "today.channelsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Channels",
      comment: "A title at the top of the screen that shows a list of topics the user can follow"
    )
    public static let popularSourcesTitle = NSLocalizedString(
      "today.popularSourcesTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Popular Sources",
      comment:
        "A title at the top of the screen that shows a list of sources that are ranked the highest"
    )
    public static let searchPlaceholder = NSLocalizedString(
      "today.searchPlaceholder",
      tableName: "BraveShared",
      bundle: .module,
      value: "Search for site, topic or RSS feed",
      comment: "Placeholder text that appears in the search bar when empty. Site as in website."
    )
    public static let popularSourcesButtonTitle = NSLocalizedString(
      "today.popularSourcesDestinationTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Popular",
      comment:
        "A button title that when tapped shows users a list of news sources that are ranked the highest"
    )
    public static let popularSourcesButtonSubtitle = NSLocalizedString(
      "today.popularSourcesButtonSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Ranked list of sources",
      comment: "Shown below 'Popular Sources' explaining what sources will be shown."
    )
    public static let suggestedSourcesButtonTitle = NSLocalizedString(
      "today.suggestedSourcesButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Suggested",
      comment:
        "A button title that when tapped shows users a list of news sources that are suggested based on the ones they currently follow"
    )
    public static let suggestedSourcesButtonSubtitle = NSLocalizedString(
      "today.suggestedSourcesButtonSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Some sources you might like",
      comment: "Shown below 'Suggested' explaining what sources will be shown."
    )
    public static let channelsButtonTitle = NSLocalizedString(
      "today.channelsButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Channels",
      comment:
        "A button title that when tapped shows users a list of channels/topics that they can follow"
    )
    public static let channelsButtonSubtitle = NSLocalizedString(
      "today.channelsButtonSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Follow topics that interest you",
      comment: "Shown below 'Channels' explaining what will be shown when tapping on it."
    )
    public static let followingButtonTitle = NSLocalizedString(
      "today.followingButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Following",
      comment:
        "A button title that when tapped shows users a list of sources & channels they are currently following"
    )
    public static let followingButtonSubtitle = NSLocalizedString(
      "today.followingButtonSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Manage sources for your feed",
      comment:
        "Shown below 'Following' explaining that they can manage their list of followed sources by tapping on it"
    )
    public static let availableRSSFeedsHeaderTitle = NSLocalizedString(
      "today.availableRSSFeedsHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Available RSS Feeds",
      comment:
        "A header title shown above a button that will allow the user to fetch RSS feeds for a website they have typed in"
    )
    public static let getFeedsFromSiteButtonTitle = NSLocalizedString(
      "today.getFeedsFromSiteButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Get feeds from",
      comment:
        "Shown with a website the user has typed in after the text. For example: 'Get feeds from https://brave.com'."
    )
    public static let noResultsFound = NSLocalizedString(
      "today.noResultsFound",
      tableName: "BraveShared",
      bundle: .module,
      value: "No Results Found",
      comment: "Shown on the screen when a user searches for sources/channels but finds no results."
    )
    public static let followToggleTitle = NSLocalizedString(
      "today.followToggleTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Follow",
      comment:
        "A toggle title shown next to a source or channel that the user can follow. When tapped switches to 'Unfollow'"
    )
    public static let unfollowToggleTitle = NSLocalizedString(
      "today.unfollowToggleTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Unfollow",
      comment:
        "A toggle title shown next to a source or channel that the user can unfollow. When tapped switches to 'Follow'"
    )
    public static let similarToSourceSubtitle = NSLocalizedString(
      "today.similarToSourceSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Similar to %@",
      comment:
        "Shown on a row displaying a source. '%@' will be replaced with another sources name. For example: 'Similar to 9to5Mac'"
    )
    public static let rateBraveCardRateActionTitle = NSLocalizedString(
      "today.rateBraveCardRateActionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rate Brave",
      comment:
        "Button title / Title for long press action that will perform an action which open AppStore Rate screen"
    )
    public static let rateBraveCardHideActionTitle = NSLocalizedString(
      "today.rateBraveCardHideActionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hide",
      comment: "Title for long press action that will hide Brave Rate Card"
    )
    public static let rateBraveCardActionSheetTitle = NSLocalizedString(
      "today.rateBraveCardActionSheetTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rate Brave in App Store",
      comment: "Title for long press action sheet list which has item for rate / hide"
    )
    public static let rateBraveCardTitle = NSLocalizedString(
      "today.rateBraveCardTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Liking Brave?",
      comment: "Title shown on the Rate Brave Card "
    )
    public static let rateBraveCardSubtitle = NSLocalizedString(
      "today.rateBraveCardSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Tell us what you think!",
      comment: "Subtitle shown on the Rate Brave Card"
    )
  }
}

// MARK: - Rewards Internals
extension Strings {
  public struct RewardsInternals {
    public static let title = NSLocalizedString(
      "RewardsInternalsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Internals",
      comment: "'Rewards' as in 'Brave Rewards'"
    )
    public static let walletInfoHeader = NSLocalizedString(
      "RewardsInternalsWalletInfoHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Profile Info",
      comment: ""
    )
    public static let keyInfoSeed = NSLocalizedString(
      "RewardsInternalsKeyInfoSeed",
      tableName: "BraveShared",
      bundle: .module,
      value: "Key Info Seed",
      comment: ""
    )
    public static let valid = NSLocalizedString(
      "RewardsInternalsValid",
      tableName: "BraveShared",
      bundle: .module,
      value: "Valid",
      comment: ""
    )
    public static let invalid = NSLocalizedString(
      "RewardsInternalsInvalid",
      tableName: "BraveShared",
      bundle: .module,
      value: "Invalid",
      comment: ""
    )
    public static let walletPaymentID = NSLocalizedString(
      "RewardsInternalsWalletPaymentID",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Payment ID",
      comment: ""
    )
    public static let walletCreationDate = NSLocalizedString(
      "RewardsInternalsWalletCreationDate",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Profile Creation Date",
      comment: ""
    )
    public static let deviceInfoHeader = NSLocalizedString(
      "RewardsInternalsDeviceInfoHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Device Info",
      comment: ""
    )
    public static let status = NSLocalizedString(
      "RewardsInternalsStatus",
      tableName: "BraveShared",
      bundle: .module,
      value: "Status",
      comment: ""
    )
    public static let supported = NSLocalizedString(
      "RewardsInternalsSupported",
      tableName: "BraveShared",
      bundle: .module,
      value: "Supported",
      comment: ""
    )
    public static let notSupported = NSLocalizedString(
      "RewardsInternalsNotSupported",
      tableName: "BraveShared",
      bundle: .module,
      value: "Not supported",
      comment: ""
    )
    public static let enrollmentState = NSLocalizedString(
      "RewardsInternalsEnrollmentState",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enrollment State",
      comment: ""
    )
    public static let enrolled = NSLocalizedString(
      "RewardsInternalsEnrolled",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enrolled",
      comment: ""
    )
    public static let notEnrolled = NSLocalizedString(
      "RewardsInternalsNotEnrolled",
      tableName: "BraveShared",
      bundle: .module,
      value: "Not enrolled",
      comment: ""
    )
    public static let balanceInfoHeader = NSLocalizedString(
      "RewardsInternalsBalanceInfoHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Balance Info",
      comment: ""
    )
    public static let totalBalance = NSLocalizedString(
      "RewardsInternalsTotalBalance",
      tableName: "BraveShared",
      bundle: .module,
      value: "Total Balance",
      comment: ""
    )
    public static let anonymous = NSLocalizedString(
      "RewardsInternalsAnonymous",
      tableName: "BraveShared",
      bundle: .module,
      value: "Anonymous",
      comment: ""
    )
    public static let logsTitle = NSLocalizedString(
      "RewardsInternalsLogsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Logs",
      comment: ""
    )
    public static let logsCount = NSLocalizedString(
      "RewardsInternalsLogsCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "%d logs shown",
      comment: "%d will be the number of logs currently being shown, i.e. '10 logs shown'"
    )
    public static let clearLogsTitle = NSLocalizedString(
      "RewardsInternalsClearLogsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Clear Logs",
      comment: ""
    )
    public static let clearLogsConfirmation = NSLocalizedString(
      "RewardsInternalsClearLogsConfirmation",
      tableName: "BraveShared",
      bundle: .module,
      value: "Are you sure you wish to clear all Rewards logs?",
      comment: ""
    )
    public static let amount = NSLocalizedString(
      "RewardsInternalsAmount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Amount",
      comment: ""
    )
    public static let type = NSLocalizedString(
      "RewardsInternalsType",
      tableName: "BraveShared",
      bundle: .module,
      value: "Type",
      comment: ""
    )
    public static let expiresAt = NSLocalizedString(
      "RewardsInternalsExpiresAt",
      tableName: "BraveShared",
      bundle: .module,
      value: "Expires At",
      comment: ""
    )
    public static let version = NSLocalizedString(
      "RewardsInternalsVersion",
      tableName: "BraveShared",
      bundle: .module,
      value: "Version",
      comment: ""
    )
    public static let contributionsTitle = NSLocalizedString(
      "RewardsInternalsContributionsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Contributions",
      comment: ""
    )
    public static let rewardsTypeAutoContribute = NSLocalizedString(
      "RewardsInternalsRewardsTypeAutoContribute",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto-Contribute",
      comment: ""
    )
    public static let rewardsTypeOneTimeTip = NSLocalizedString(
      "RewardsInternalsRewardsTypeOneTimeTip",
      tableName: "BraveShared",
      bundle: .module,
      value: "One time tip",
      comment: ""
    )
    public static let rewardsTypeRecurringTip = NSLocalizedString(
      "RewardsInternalsRewardsTypeRecurringTip",
      tableName: "BraveShared",
      bundle: .module,
      value: "Recurring tip",
      comment: ""
    )
    public static let contributionsStepACOff = NSLocalizedString(
      "RewardsInternalsContributionsStepACOff",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto-Contribute Off",
      comment: ""
    )
    public static let contributionsStepRewardsOff = NSLocalizedString(
      "RewardsInternalsContributionsStepRewardsOff",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Off",
      comment: ""
    )
    public static let contributionsStepACTableEmpty = NSLocalizedString(
      "RewardsInternalsContributionsStepACTableEmpty",
      tableName: "BraveShared",
      bundle: .module,
      value: "AC table empty",
      comment: "'AC' refers to Auto-Contribute"
    )
    public static let contributionsStepNotEnoughFunds = NSLocalizedString(
      "RewardsInternalsContributionsStepNotEnoughFunds",
      tableName: "BraveShared",
      bundle: .module,
      value: "Not enough funds",
      comment: ""
    )
    public static let contributionsStepFailed = NSLocalizedString(
      "RewardsInternalsContributionsStepFailed",
      tableName: "BraveShared",
      bundle: .module,
      value: "Failed",
      comment: ""
    )
    public static let contributionsStepCompleted = NSLocalizedString(
      "RewardsInternalsContributionsStepCompleted",
      tableName: "BraveShared",
      bundle: .module,
      value: "Completed",
      comment: ""
    )
    public static let contributionsStepStart = NSLocalizedString(
      "RewardsInternalsContributionsStepStart",
      tableName: "BraveShared",
      bundle: .module,
      value: "Start",
      comment: ""
    )
    public static let contributionsStepPrepare = NSLocalizedString(
      "RewardsInternalsContributionsStepPrepare",
      tableName: "BraveShared",
      bundle: .module,
      value: "Prepare",
      comment: ""
    )
    public static let contributionsStepReserve = NSLocalizedString(
      "RewardsInternalsContributionsStepReserve",
      tableName: "BraveShared",
      bundle: .module,
      value: "Reserve",
      comment: ""
    )
    public static let contributionsStepExternalTransaction = NSLocalizedString(
      "RewardsInternalsContributionsStepExternalTransaction",
      tableName: "BraveShared",
      bundle: .module,
      value: "External Transaction",
      comment: ""
    )
    public static let contributionsStepCreds = NSLocalizedString(
      "RewardsInternalsContributionsStepCreds",
      tableName: "BraveShared",
      bundle: .module,
      value: "Credentials",
      comment: ""
    )
    public static let contributionProcessorBraveTokens = NSLocalizedString(
      "RewardsInternalsContributionProcessorBraveTokens",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Tokens",
      comment: ""
    )
    public static let contributionProcessorUserFunds = NSLocalizedString(
      "RewardsInternalsContributionProcessorUserFunds",
      tableName: "BraveShared",
      bundle: .module,
      value: "User Funds",
      comment: ""
    )
    public static let contributionProcessorUphold = NSLocalizedString(
      "RewardsInternalsContributionProcessorUphold",
      tableName: "BraveShared",
      bundle: .module,
      value: "Uphold",
      comment: ""
    )
    public static let contributionProcessorNone = NSLocalizedString(
      "RewardsInternalsContributionProcessorNone",
      tableName: "BraveShared",
      bundle: .module,
      value: "None",
      comment: ""
    )
    public static let createdAt = NSLocalizedString(
      "RewardsInternalsCreatedAt",
      tableName: "BraveShared",
      bundle: .module,
      value: "Created at",
      comment: ""
    )
    public static let step = NSLocalizedString(
      "RewardsInternalsStep",
      tableName: "BraveShared",
      bundle: .module,
      value: "Step",
      comment: "i.e. 'Step: Started'"
    )
    public static let retryCount = NSLocalizedString(
      "RewardsInternalsRetryCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Retry Count",
      comment: ""
    )
    public static let processor = NSLocalizedString(
      "RewardsInternalsProcessor",
      tableName: "BraveShared",
      bundle: .module,
      value: "Processor",
      comment: ""
    )
    public static let publishers = NSLocalizedString(
      "RewardsInternalsPublishers",
      tableName: "BraveShared",
      bundle: .module,
      value: "Publishers",
      comment: ""
    )
    public static let publisher = NSLocalizedString(
      "RewardsInternalsPublisher",
      tableName: "BraveShared",
      bundle: .module,
      value: "Publisher",
      comment: ""
    )
    public static let totalAmount = NSLocalizedString(
      "RewardsInternalsTotalAmount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Total amount",
      comment: ""
    )
    public static let contributionAmount = NSLocalizedString(
      "RewardsInternalsContributionAmount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Contribution amount",
      comment: ""
    )
    public static let shareInternalsTitle = NSLocalizedString(
      "RewardsInternalsShareInternalsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Share Rewards Internals",
      comment: "'Rewards' as in 'Brave Rewards'"
    )
    public static let share = NSLocalizedString(
      "RewardsInternalsShare",
      tableName: "BraveShared",
      bundle: .module,
      value: "Share",
      comment: ""
    )
    public static let sharableBasicTitle = NSLocalizedString(
      "RewardsInternalsSharableBasicTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Basic Info",
      comment: ""
    )
    public static let sharableBasicDescription = NSLocalizedString(
      "RewardsInternalsSharableBasicDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards profile, device & balance info (always shared)",
      comment: ""
    )
    public static let sharableLogsDescription = NSLocalizedString(
      "RewardsInternalsSharableLogsDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards specific logging",
      comment: ""
    )
    public static let sharableContributionsDescription = NSLocalizedString(
      "RewardsInternalsSharableContributionsDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Any contributions made to publishers through tipping or auto-contribute",
      comment: ""
    )
    public static let sharableDatabaseTitle = NSLocalizedString(
      "RewardsInternalsSharableDatabaseTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Rewards Database",
      comment: ""
    )
    public static let sharableDatabaseDescription = NSLocalizedString(
      "RewardsInternalsSharableDatabaseDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "The internal data store",
      comment: ""
    )
    public static let sharingWarningTitle = NSLocalizedString(
      "RewardsInternalsSharingWarningTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Warning",
      comment: ""
    )
    public static let sharingWarningMessage = NSLocalizedString(
      "RewardsInternalsSharingWarningMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "Data on these pages may be sensitive. Be careful who you share them with.",
      comment: ""
    )
  }
}

// MARK: - Rewards
extension Strings {
  public struct Rewards {
    public static let enabledBody = NSLocalizedString(
      "rewards.enabledBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "You are helping support Brave and the BAT community.",
      comment: "Displayed when Brave Rewards is enabled"
    )
    public static let disabledBody = NSLocalizedString(
      "rewards.disabledBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn on to help support Brave and the BAT community.",
      comment: "Displayed when Brave Rewards is disabled"
    )
    public static let supportingPublisher = NSLocalizedString(
      "rewards.supportingPublisher",
      tableName: "BraveShared",
      bundle: .module,
      value: "This content creator is verified with Brave Creators",
      comment: "Displayed under verified publishers"
    )
    public static let unverifiedPublisher = NSLocalizedString(
      "rewards.unverifiedPublisher",
      tableName: "BraveShared",
      bundle: .module,
      value: "This creator has not verified and will not be included in creator support",
      comment: "Displayed under unverified publishers"
    )
    public static let enabledStatusBody = NSLocalizedString(
      "rewards.enabledStatusBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Thank you for helping support Brave and the BAT community!",
      comment:
        "Displayed in the status container when rewards is enabled but you're not currently supporting any publishers (0 AC count)"
    )
    public static let disabledStatusBody = NSLocalizedString(
      "rewards.disabledStatusBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Using Brave Rewards helps support Brave and the BAT community.",
      comment: "Displayed in the status container when rewards is disabled"
    )
    public static let totalSupportedCount = NSLocalizedString(
      "rewards.totalSupportedCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Number of content creators you are helping support this month.",
      comment: "Displayed next to a number representing the total number of publishers supported"
    )
    public static let settingsToggleTitle = NSLocalizedString(
      "rewards.settingsToggleTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enable Brave Rewards",
      comment: ""
    )
    public static let settingsToggleMessage = NSLocalizedString(
      "rewards.settingsToggleMessage",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Help support Brave and the BAT community by enabling Brave Rewards and seeing Brave Ads.",
      comment: ""
    )
    public static let onProviderText = NSLocalizedString(
      "OnProviderText",
      tableName: "BraveShared",
      bundle: .module,
      value: "on %@",
      comment: "This is a suffix statement. example: SomeChannel on Twitter"
    )
    public static let braveTalkRewardsOptInTitle =
      NSLocalizedString(
        "rewards.braveTalkRewardsOptInTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "To start a free call, turn on Brave Rewards",
        comment: "Title for Brave Talk rewards opt-in screen"
      )

    public static let braveTalkRewardsOptInBody =
      NSLocalizedString(
        "rewards.braveTalkRewardsOptInBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "With Brave Rewards, you can view privacy-preserving ads from the Brave Ads network. No trackers. No slowdowns. And your data stays totally safe.",
        comment: "Body for Brave Talk rewards opt-in screen"
      )

    public static let braveTalkRewardsOptInButtonTitle =
      NSLocalizedString(
        "rewards.braveTalkRewardsOptInButtonTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Turn on Rewards",
        comment: "Title for Brave Talk rewards opt-in screen button"
      )

    public static let braveTalkRewardsOptInDisclaimer =
      NSLocalizedString(
        "rewards.optInDisclaimer",
        tableName: "BraveShared",
        bundle: .module,
        value: "By clicking, you agree to the %@ and %@. Disable any time in Settings.",
        comment:
          "The placeholders say 'Terms of Service' and 'Privacy Policy'. So full sentence goes like: 'By clicking, you agree to the Terms of Service and Privacy Policy...'"
      )

    public static let braveTalkRewardsOptInSuccessTitle =
      NSLocalizedString(
        "rewards.braveTalkRewardsOptInSuccessTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "You can now start a free call",
        comment: "Title for successful Brave Talk rewards opt-in"
      )

    public static let braveTalkRewardsOptInSuccessBody =
      NSLocalizedString(
        "rewards.braveTalkRewardsOptInSuccessBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Click anywhere on the screen to continue to Brave Talk.",
        comment: "Body for successful Brave Talk rewards opt-in"
      )
  }
}

// MARK: - Talk
extension Strings {
  public struct BraveTalk {
    public static let braveTalkTitle = NSLocalizedString(
      "bravetalk.braveTalkTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Talk",
      comment: "The name of the feature"
    )
  }
}

// MARK: - Ads
extension Strings {
  public struct Ads {
    public static let myFirstAdTitle = NSLocalizedString(
      "MyFirstAdTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "This is your first Brave ad",
      comment: ""
    )
    public static let myFirstAdBody = NSLocalizedString(
      "MyFirstAdBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Tap here to learn more.",
      comment: ""
    )
    public static let open = NSLocalizedString(
      "BraveRewardsOpen",
      tableName: "BraveShared",
      bundle: .module,
      value: "Open",
      comment: ""
    )
    public static let adNotificationTitle = NSLocalizedString(
      "BraveRewardsAdNotificationTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Rewards",
      comment: ""
    )

    public static let braveAdsDataToggleOption =
      NSLocalizedString(
        "ads.braveAdsDataToggleOption",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Ads Data",
        comment:
          "Text for the toggle switch to clear 'Brave Ads' data in settings"
      )
  }
}

// MARK: - Recent Searches
extension Strings {
  public static let recentSearchFavorites = NSLocalizedString(
    "RecentSearchFavorites",
    tableName: "BraveShared",
    bundle: .module,
    value: "Favorites",
    comment: "Recent Search Favorites Section Title"
  )
  public static let recentSearchPasteAndGo = NSLocalizedString(
    "RecentSearchPasteAndGo",
    tableName: "BraveShared",
    bundle: .module,
    value: "Paste & Go",
    comment: "Recent Search Paste & Go Button Title"
  )
  public static let recentSearchSectionTitle = NSLocalizedString(
    "RecentSearchSectionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Recent Searches",
    comment: "Recent Search Section Title"
  )
  public static let recentSearchSectionDescription = NSLocalizedString(
    "RecentSearchSectionDescription",
    tableName: "BraveShared",
    bundle: .module,
    value:
      "Recent Searches allow you to privately access past searches. Would you like to enable Recent Searches?",
    comment: "Recent Search Section Description"
  )
  public static let recentSearchClear = NSLocalizedString(
    "RecentSearchClear",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear",
    comment: "Recent Search Clear Button"
  )
  public static let recentSearchShow = NSLocalizedString(
    "RecentSearchShow",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show",
    comment: "Recent Search Show Button"
  )
  public static let recentSearchHide = NSLocalizedString(
    "RecentSearchHide",
    tableName: "BraveShared",
    bundle: .module,
    value: "Hide",
    comment: "Recent Search Hide Button"
  )
  public static let recentShowMore = NSLocalizedString(
    "RecentSearchShowMore",
    tableName: "BraveShared",
    bundle: .module,
    value: "Show more",
    comment: "Recent Search Show More button"
  )
  public static let recentSearchScanned = NSLocalizedString(
    "RecentSearchScanned",
    tableName: "BraveShared",
    bundle: .module,
    value: "Scanned",
    comment: "Recent Search Scanned text when a user scans a qr code"
  )
  public static let recentSearchQuickSearchOnWebsite = NSLocalizedString(
    "RecentSearchQuickSearchOnWebsite",
    tableName: "BraveShared",
    bundle: .module,
    value: "on",
    comment: "Recent Search 'on' text when a user searches 'on' a website"
  )
  public static let recentSearchSuggestionsTitle = NSLocalizedString(
    "RecentSearchSuggestionsTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Search Suggestions",
    comment: "Recent Search suggestions title when prompting to turn on suggestions"
  )
  public static let recentSearchEnableSuggestions = NSLocalizedString(
    "RecentSearchEnableSuggestions",
    tableName: "BraveShared",
    bundle: .module,
    value: "Enable",
    comment: "Recent Search button title to enable suggestions"
  )
  public static let recentSearchDisableSuggestions = NSLocalizedString(
    "RecentSearchDisableSuggestions",
    tableName: "BraveShared",
    bundle: .module,
    value: "Disable",
    comment: "Recent Search button title to disable suggestions"
  )
  public static let recentSearchClearDataToggleOption = NSLocalizedString(
    "RecentSearchClearDataToggleOption",
    tableName: "BraveShared",
    bundle: .module,
    value: "Recent Search Data",
    comment: "Recent Search setting title to clear recent searches"
  )
  public static let recentSearchScannerTitle = NSLocalizedString(
    "RecentSearchScannerTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Scan QR Code",
    comment: "Scanning a QR Code for searching"
  )
  public static let recentSearchScannerDescriptionTitle = NSLocalizedString(
    "RecentSearchScannerDescriptionTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Instructions",
    comment: "Scanning a QR Code for title"
  )
  public static let recentSearchScannerDescriptionBody = NSLocalizedString(
    "RecentSearchScannerDescriptionBody",
    tableName: "BraveShared",
    bundle: .module,
    value: "To search by QR Code, align the QR Code in the center of the frame.",
    comment: "Scanning a QR Code for searching body"
  )
  public static let recentSearchClearAlertButton = NSLocalizedString(
    "RecentSearchClearAlertButton",
    tableName: "BraveShared",
    bundle: .module,
    value: "Clear Recent",
    comment: "The button title that shows when you clear all recent searches"
  )
}

// MARK: - Widgets
extension Strings {
  public struct Widgets {
    public static let widgetTitle = NSLocalizedString(
      "widgets.widgetTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Widgets",
      comment: "Title of the settings for widgets part"
    )

    public static let favoritesWidgetTitle = NSLocalizedString(
      "widgets.favoritesWidgetTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Favorites",
      comment: "Title for favorites widget on 'add widget' screen."
    )
  }
}

// MARK: - Night Mode

extension Strings {
  public struct NightMode {
    public static let sectionTitle = NSLocalizedString(
      "nightMode.modeTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Mode",
      comment:
        "It refers to a night mode, to a type of mode a user can change referring to website appearance."
    )
    public static let settingsTitle = NSLocalizedString(
      "nightMode.settingsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Night Mode",
      comment: "A table cell title for Night Mode - defining element for the toggle"
    )
    public static let settingsDescription = NSLocalizedString(
      "nightMode.settingsDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn on/off Night Mode",
      comment:
        "A table cell subtitle for Night Mode - explanatory element for the toggle preference"
    )
    public static let sectionDescription = NSLocalizedString(
      "nightMode.sectionDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Night mode will effect website appearance and general system appearance at the same time.",
      comment:
        "A table cell subtitle for Night Mode - explanatory element for the toggle preference"
    )
  }
}

// MARK: - ManageWebsiteData
extension Strings {
  public static let manageWebsiteDataTitle = NSLocalizedString(
    "websiteData.manageWebsiteDataTitle",
    tableName: "BraveShared",
    bundle: .module,
    value: "Manage Website Data",
    comment:
      "A button or screen title describing that the user is there to manually manage website data that is persisted to their device. I.e. to manage data specific to a web page the user has visited"
  )
  public static let loadingWebsiteData = NSLocalizedString(
    "websiteData.loadingWebsiteData",
    tableName: "BraveShared",
    bundle: .module,
    value: "Loading website data…",
    comment: "A message displayed to users while the system fetches all website data to display."
  )
  public static let dataRecordCookies = NSLocalizedString(
    "websiteData.dataRecordCookies",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cookies",
    comment:
      "The word used to describe small bits of state stored locally in a web browser (e.g. Browser cookies)"
  )
  public static let dataRecordCache = NSLocalizedString(
    "websiteData.dataRecordCache",
    tableName: "BraveShared",
    bundle: .module,
    value: "Cache",
    comment:
      "Temporary data that is stored on the users device to speed up future requests and interactions."
  )
  public static let dataRecordLocalStorage = NSLocalizedString(
    "websiteData.dataRecordLocalStorage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Local storage",
    comment:
      "A kind of browser storage particularely for saving data on the users device for a specific webpage for the given session or longer time periods."
  )
  public static let dataRecordDatabases = NSLocalizedString(
    "websiteData.dataRecordDatabases",
    tableName: "BraveShared",
    bundle: .module,
    value: "Databases",
    comment: "Some data stored on disk that is a kind of database (such as WebSQL or IndexedDB.)"
  )
  public static let removeDataRecord = NSLocalizedString(
    "websiteData.removeDataRecord",
    tableName: "BraveShared",
    bundle: .module,
    value: "Remove",
    comment:
      "Shown when a user has attempted to delete a single webpage data record such as cookies, caches, or local storage that has been persisted on their device. Tapping it will delete that records and remove it from the list"
  )
  public static let removeSelectedDataRecord = NSLocalizedString(
    "websiteData.removeSelectedDataRecord",
    tableName: "BraveShared",
    bundle: .module,
    value: "Remove %ld items",
    comment:
      "Shown on a button when a user has selected multiple webpage data records (such as cookies, caches, or local storage) that has been persisted on their device. Tapping it will delete those records and remove them from the list"
  )
  public static let removeAllDataRecords = NSLocalizedString(
    "websiteData.removeAllDataRecords",
    tableName: "BraveShared",
    bundle: .module,
    value: "Remove All",
    comment:
      "Shown on a button to delete all displayed webpage records (such as cookies, caches, or local storage) that has been persisted on their device. Tapping it will delete those records and remove them from the list"
  )
  public static let noSavedWebsiteData = NSLocalizedString(
    "websiteData.noSavedWebsiteData",
    tableName: "BraveShared",
    bundle: .module,
    value: "No Saved Website Data",
    comment:
      "Shown when the user has no website data (such as cookies, caches, or local storage) persisted to their device."
  )

  public static let blockCookieConsentNotices = NSLocalizedString(
    "BlockCookieConsentNotices",
    tableName: "BraveShared",
    bundle: .module,
    value: "Block Cookie Consent Notices",
    comment: "A title for a setting that enables cookie consent notices"
  )

  public static let braveShieldsSaveContactInfo = NSLocalizedString(
    "BraveShieldsSaveContactInfo",
    tableName: "BraveShared",
    bundle: .module,
    value: "Store contact information for future broken site reports",
    comment: "Shields panel toggle label that would save webcompat report contact info data when enabled."
  )

  public static let braveShieldsSaveContactInfoDescription = NSLocalizedString(
    "BraveShieldsSaveContactInfoDescription",
    tableName: "BraveShared",
    bundle: .module,
    value: "If you provide contact info it will be stored for future reports",
    comment: "Description for shields panel toggle label that would save webcompat report contact info data when enabled."
  )
}

// MARK: - Privacy hub
extension Strings {
  public struct PrivacyHub {
    public static let privacyReportsTitle = NSLocalizedString(
      "privacyHub.privacyReportsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Privacy Hub",
      comment:
        "Title of main privacy hub screen. This screen shows various stats caught by Brave's ad blockers."
    )

    public static let notificationCalloutBody = NSLocalizedString(
      "privacyHub.notificationCalloutBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Get weekly privacy updates on tracker & ad blocking.",
      comment: "Text of a callout to encourage user to enable Apple notification system."
    )

    public static let notificationCalloutButtonText = NSLocalizedString(
      "privacyHub.notificationCalloutButtonText",
      tableName: "BraveShared",
      bundle: .module,
      value: "Turn on notifications",
      comment: "Text of a button to encourage user to enable Apple notification system."
    )

    public static let noDataCalloutBody = NSLocalizedString(
      "privacyHub.noDataCalloutBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Visit some websites to see data here.",
      comment:
        "Text of a callout that tell user they need to browser some websites first in order to see privacy stats data"
    )

    public static let trackingDisabledCalloutBody = NSLocalizedString(
      "privacyHub.trackingDisabledCalloutBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enable '%@' to see data here",
      comment:
        "Text of a callout that tells the user they shields tracking data is disabled. The %@ placeholder will name the setting that needs to be enabled"
    )

    public static let lastWeekHeader = NSLocalizedString(
      "privacyHub.lastWeekHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Last week",
      comment: "Header text, under it we display blocked items from last week"
    )

    public static let mostFrequentTrackerAndAdTitle = NSLocalizedString(
      "privacyHub.mostFrequentTrackerAndAdTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Most Frequent Tracker & Ad",
      comment:
        "Title under which we display a tracker which was most frequently detected by our ad blocking mechanism."
    )

    public static let mostFrequentTrackerAndAdBody = NSLocalizedString(
      "privacyHub.mostFrequentTrackerAndAdBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "**%@** was blocked by Brave Shields on **%lld** sites",
      comment:
        "Do NOT localize asterisk('*') characters, they are used to make the text bold in the app. It says which tracker was blocked on how many websites, example usage: 'Google Analytics was blocked by Brave Shields on 42 sites'"
    )

    public static let noDataToShow = NSLocalizedString(
      "privacyHub.noDataToShow",
      tableName: "BraveShared",
      bundle: .module,
      value: "No data to show yet.",
      comment:
        "This text is diplayed when there is no data to display to the user. The data is about blocked trackers or sites with trackers on them."
    )

    public static let riskiestWebsiteTitle = NSLocalizedString(
      "privacyHub.riskiestWebsiteTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Site with the most trackers",
      comment: "Title of a website that contained the most trackers per visit."
    )

    public static let riskiestWebsiteBody = NSLocalizedString(
      "privacyHub.riskiestWebsiteBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "**%@** had an average of **%lld** trackers & ads blocked per visit",
      comment:
        "Do NOT localize asterisk('*') characters, they are used to make the text bold in the app. It says which website had the most tracker per visit, example usage: 'example.com had an average of 10 trackers & ads blocked per visit '"
    )

    public static let vpnAlertsHeader = NSLocalizedString(
      "privacyHub.vpnAlertsHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Firewall + VPN Alerts",
      comment:
        "Section title, this section displays vpn alerts: items which the vpn managed to block on users behalf."
    )

    public static let allVPNAlertsButtonText = NSLocalizedString(
      "privacyHub.allVPNAlertsButtonText",
      tableName: "BraveShared",
      bundle: .module,
      value: "All alerts",
      comment:
        "Text for a button to display a list of all alerts caught by the Brave VPN. VPN alert is a notificaion of what item has been blocked by the vpn, similar to a regular adblocker"
    )

    public static let allTimeListsHeader = NSLocalizedString(
      "privacyHub.allTimeListsHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "All time",
      comment:
        "Header text, under it we show items blocked by our ad blocker. 'All  time' sentence context is like 'All time items blocked by our ad blocker."
    )

    public static let allTimeTrackerTitle = NSLocalizedString(
      "privacyHub.allTimeTrackerTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Tracker & Ad",
      comment: "Title under which we display most name of the most blocked tracker or ad."
    )

    public static let allTimeWebsiteTitle = NSLocalizedString(
      "privacyHub.allTimeWebsiteTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Website",
      comment:
        "Title under which we display a website on which there's the highest number of trackers or ads."
    )

    public static let allTimeSitesCount = NSLocalizedString(
      "privacyHub.allTimeSitesCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "%lld sites",
      comment:
        "Displays a number of websites on which we blocked trackers, example usage: '23 sites'"
    )

    public static let allTimeTrackersCount = NSLocalizedString(
      "privacyHub.allTimeTrackersCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "%lld trackers & ads",
      comment:
        "Displays a number of trackers we blocked on a particular website, example usage: '23 trackers & ads'"
    )

    public static let allTimeListsButtonText = NSLocalizedString(
      "privacyHub.allTimeListsButtonText",
      tableName: "BraveShared",
      bundle: .module,
      value: "All time lists",
      comment:
        "Button text that takes user to a list of all trackers and ads we blocked. 'All time lists' refer to list of blocked trackers or websites which have the most trackers"
    )

    public static let allTimeListsTrackersView = NSLocalizedString(
      "privacyHub.allTimeListsTrackersView",
      tableName: "BraveShared",
      bundle: .module,
      value: "Trackers & ads",
      comment: "Title of a section to show total count of trackers blocked by Brave"
    )

    public static let allTimeListsWebsitesView = NSLocalizedString(
      "privacyHub.allTimeListsWebsitesView",
      tableName: "BraveShared",
      bundle: .module,
      value: "Websites",
      comment: "Title of a section to show websites containing highest amount of trackers"
    )

    public static let blockedBy = NSLocalizedString(
      "privacyHub.blockedBy",
      tableName: "BraveShared",
      bundle: .module,
      value: "Blocked by",
      comment:
        "Text which explain by what type of ad blocker a given resource was blocked. Context is like: 'Blocked by Brave Shields', 'Blocked by BraveVPN"
    )

    public static let allTimeListTrackersHeaderTitle = NSLocalizedString(
      "privacyHub.allTimeListTrackersHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Most frequent trackers & ads on sites you Visit",
      comment: "Header title for a list of most frequent ads and trackers detected."
    )

    public static let allTimeListWebsitesHeaderTitle = NSLocalizedString(
      "privacyHub.allTimeListWebsitesHeaderTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Websites with the most trackers & ads",
      comment: "Header title for a list of websites with ads and trackers."
    )

    public static let vpvnAlertsTotalCount = NSLocalizedString(
      "privacyHub.vpvnAlertsTotalCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Total count",
      comment: "It shows a total count of items blocked by our VPN shields"
    )

    public static let shieldsLabel = NSLocalizedString(
      "privacyHub.shieldsLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Shields",
      comment:
        "This label says shields, as a source of by what a resource was blocked. Think of it in context of 'Blocked by Shields'"
    )

    public static let vpnLabel = NSLocalizedString(
      "privacyHub.vpnLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Firewall + VPN",
      comment:
        "This label says about Brave VPN, as a source of by what the resource was blocked by. Think of it in context of 'Blocked by VPN'"
    )

    public static let blockedLabel = NSLocalizedString(
      "privacyHub.blockedLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Blocked",
      comment:
        "It says that a ad or tracker was blocked. Think of it in context of 'A tracker X was blocked'"
    )

    public static let vpnAlertRegularTrackerTypeSingular = NSLocalizedString(
      "privacyHub.vpnAlertRegularTrackerTypeSingular",
      tableName: "BraveShared",
      bundle: .module,
      value: "Tracker or Ad",
      comment: "Type of tracker blocked by the VPN, it's a regular tracker or an ad."
    )

    public static let vpnAlertLocationTrackerTypeSingular = NSLocalizedString(
      "privacyHub.vpnAlertLocationTrackerTypeSingular",
      tableName: "BraveShared",
      bundle: .module,
      value: "Location Ping",
      comment: "Type of tracker blocked by the VPN, it's a tracker that asks you for your location."
    )

    public static let vpnAlertEmailTrackerTypeSingular = NSLocalizedString(
      "privacyHub.vpnAlertEmailTrackerTypeSingular",
      tableName: "BraveShared",
      bundle: .module,
      value: "Email Tracker",
      comment: "Type of tracker blocked by the VPN, it's a tracker contained in an email."
    )

    public static let vpnAlertRegularTrackerTypePlural = NSLocalizedString(
      "privacyHub.vpnAlertRegularTrackerTypePlural",
      tableName: "BraveShared",
      bundle: .module,
      value: "Trackers & Ads",
      comment: "Type of tracker blocked by the VPN, it's a regular tracker or an ad."
    )

    public static let vpnAlertLocationTrackerTypePlural = NSLocalizedString(
      "privacyHub.vpnAlertLocationTrackerTypePlural",
      tableName: "BraveShared",
      bundle: .module,
      value: "Location Pings",
      comment: "Type of tracker blocked by the VPN, it's a tracker that asks you for your location."
    )

    public static let vpnAlertEmailTrackerTypePlural = NSLocalizedString(
      "privacyHub.vpnAlertEmailTrackerTypePlural",
      tableName: "BraveShared",
      bundle: .module,
      value: "Email Trackers",
      comment: "Type of tracker blocked by the VPN, it's a tracker contained in an email."
    )

    public static let notificationTitle = NSLocalizedString(
      "privacyHub.notificationTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Weekly Privacy Report",
      comment:
        "Title of a notification we show to the user, on tapping it, the Privacy Hub screen will open."
    )

    public static let notificationMessage = NSLocalizedString(
      "privacyHub.notificationMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "A recap of how Brave protected you online this week.",
      comment:
        "Message of a notification we show to the user, on tapping it, the Privacy Hub screen will open."
    )

    public static let settingsEnableShieldsTitle = NSLocalizedString(
      "privacyHub.settingsEnableShieldsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show Shields Data",
      comment: "Title of a setting that lets Brave monitor blocked network requests"
    )

    public static let settingsEnableShieldsFooter = NSLocalizedString(
      "privacyHub.settingsEnableShieldsFooter",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Privacy Hub shows a count of what Shields blocked. Setting will not affect Shields counter on new tab page. Shields data is not counted in private windows.",
      comment: "This text explains a setting that lets Brave monitor blocked network requests"
    )

    public static let settingsEnableVPNAlertsTitle = NSLocalizedString(
      "privacyHub.settingsEnableVPNAlertsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show VPN Alerts",
      comment:
        "Title of a setting that lets Brave monitor blocked network requests captured by Brave VPN"
    )

    public static let settingsEnableVPNAlertsFooter = NSLocalizedString(
      "privacyHub.settingsEnableVPNAlertsFooter",
      tableName: "BraveShared",
      bundle: .module,
      value: "Setting only applies if you've purchased VPN subscription.",
      comment:
        "This text explains a setting that lets Brave monitor blocked network requests captured by Brave VPN"
    )

    public static let settingsSlearDataTitle = NSLocalizedString(
      "privacyHub.settingsSlearDataTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Clear Shields Data",
      comment:
        "Button that lets user clear all blocked requests and vpn alerts data that Brave captured for them."
    )

    public static let settingsSlearDataFooter = NSLocalizedString(
      "privacyHub.settingsSlearDataFooter",
      tableName: "BraveShared",
      bundle: .module,
      value: "Resets the count of everything Shields has blocked.",
      comment: "This text explains what the button to clear datain the Privacy Hub is for."
    )

    public static let clearAllDataPrompt = NSLocalizedString(
      "privacyHub.clearAllDataPrompt",
      tableName: "BraveShared",
      bundle: .module,
      value: "Clear all Shields data?",
      comment:
        "A prompt message we show to the user if they want to clear all data gathered by the Privacy Reports Feature"
    )

    public static let clearAllDataAccessibility = NSLocalizedString(
      "privacyHub.clearAllDataAccessibility",
      tableName: "BraveShared",
      bundle: .module,
      value: "Clear Privacy Hub data",
      comment: "Accessibility label for the 'clear all data' button."
    )

    public static let privacyReportsDisclaimer = NSLocalizedString(
      "privacyHub.privacyReportsDisclaimer",
      tableName: "BraveShared",
      bundle: .module,
      value: "Privacy Hub data is stored locally and never sent anywhere.",
      comment:
        "Text of a disclaimer that explains how the data for generating privacy reprots is stored."
    )

    public static let onboardingButtonTitle = NSLocalizedString(
      "privacyHub.onboardingButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Open Privacy Hub",
      comment: "Text of a button that opens up a Privacy Reports screen."
    )

    public static let hidePrivacyHubWidgetActionTitle = NSLocalizedString(
      "privacyHub.hidePrivacyHubWidgetActionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hide Privacy Stats",
      comment: "The title of the button action which hides the privacy hub"
    )

    public static let hidePrivacyHubWidgetAlertDescription = NSLocalizedString(
      "privacyHub.hidePrivacyHubWidgetAlertDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Tap Hide to remove the Privacy Stats widget. You can always re-enable in Settings, under New Tab Page",
      comment: "The description in alert of the hide action which hides the privacy hub"
    )

    public static let openPrivacyHubWidgetActionTitle = NSLocalizedString(
      "privacyHub.openPrivacyHubWidgetActionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Open Privacy Hub",
      comment: "The title of the button action which open the privacy hub"
    )

    public static let hidePrivacyHubWidgetActionButtonTitle = NSLocalizedString(
      "privacyHub.hidePrivacyHubWidgetActionButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hide",
      comment: "Hide Privacy Hub Alert Hide Button"
    )
  }
}

// P3A
extension Strings {
  public struct P3A {
    public static let settingTitle = NSLocalizedString(
      "p3a.settingTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Allow Privacy-Preserving Product Analytics (P3A)",
      comment:
        "The title for the setting that will allow a user to toggle sending privacy preserving analytics to Brave."
    )

    public static let settingSubtitle = NSLocalizedString(
      "p3a.settingSubtitle",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "These private responses help Brave estimate the overall usage of certain features and make them better for you.",
      comment: "A subtitle shown on the setting that toggles analytics on Brave."
    )

    public static let continueButton = NSLocalizedString(
      "p3a.continue",
      tableName: "BraveShared",
      bundle: .module,
      value: "Continue",
      comment:
        "A button to proceed with the rest of the user onboarding after they see the P3A introduction. It means to continue browsing or continue with the rest of the onboarding"
    )
  }
}

// Page Zoom
extension Strings {
  public struct PageZoom {
    public static let settingsTitle = NSLocalizedString(
      "pagezoom.settings.title",
      tableName: "BraveShared",
      bundle: .module,
      value: "Page Zoom",
      comment: "Title of the Web-Page Zoom screen"
    )

    public static let resetAll = NSLocalizedString(
      "pagezoom.settings.reset-all",
      tableName: "BraveShared",
      bundle: .module,
      value: "Reset All",
      comment: "Title of the Web-Page Zoom reset all option"
    )

    public static let resetAllDescription = NSLocalizedString(
      "pagezoom.settings.reset-all-description",
      tableName: "BraveShared",
      bundle: .module,
      value: "Reset Website Zoom Levels",
      comment: "Description of the Web-Page Zoom reset all option"
    )

    public static let specificWebsitesZoomLevelsSectionTitle = NSLocalizedString(
      "pagezoom.settings.specificWebsitesZoomLevelsSectionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Specific Page Settings",
      comment: "Title for list of websites shown in settings screen"
    )

    public static let zoomViewText = NSLocalizedString(
      "pagezoom.zoomView.text",
      tableName: "BraveShared",
      bundle: .module,
      value: "Page Zoom",
      comment: "Title for the web-page zoom level view"
    )

    public static let defaultZoomLevelSectionTitle = NSLocalizedString(
      "pagezoom.settings.defaultZoomLevelSectionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Default Web-Page Zoom Level",
      comment: "Title for the section for the global/default zoom level"
    )

    public static let otherWebsiteZoomLevelSectionTitle = NSLocalizedString(
      "pagezoom.settings.otherWebsitesZoomLevelSectionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "All Other Websites",
      comment: "Title for the section for the Web-Page zoom level of other websites"
    )

    public static let emptyZoomSettingsWebsitesTitle = NSLocalizedString(
      "pagezoom.settings.emptyWebsitesTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "No Websites Added",
      comment: "Title for the empty Web-Page zoom view"
    )

    public static let emptyZoomSettingsWebsitesDescription = NSLocalizedString(
      "pagezoom.settings.emptyWebsitesDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "When a website's Zoom-Level has changed, the website will show up on the list here. Currently, there are no websites zoomed in or out.",
      comment: "Description for the empty Web-Page zoom view"
    )
  }
}

extension Strings {
  public struct RecentlyClosed {
    public static let viewRecentlyClosedTab =
      NSLocalizedString(
        "recently.closed.view.tab",
        tableName: "BraveShared",
        bundle: .module,
        value: "View Recently Closed Tabs",
        comment: "Action item title of long press for Opening recently closed tab view"
      )

    public static let recentlyClosedTabsScreenTitle =
      NSLocalizedString(
        "recently.closed.title",
        tableName: "BraveShared",
        bundle: .module,
        value: "Recently Closed Tabs",
        comment: "Action item title of long press for Opening recently closed tab view"
      )

    public static let recentlyClosedClearActionTitle =
      NSLocalizedString(
        "recently.closed.action.clear",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear",
        comment: "Title for Clear Button inside Recently Closed Tabs"
      )

    public static let recentlyClosedClearActionConfirmation =
      NSLocalizedString(
        "recently.closed.confirmation.clear.title",
        tableName: "BraveShared",
        bundle: .module,
        value: "Clear All Recently Closed Tabs?",
        comment: "Confirmation message for Clear Button action inside Recently Closed Tabs"
      )

    public static let recentlyClosedEmptyListTitle =
      NSLocalizedString(
        "recently.closed.empty.list.title",
        tableName: "BraveShared",
        bundle: .module,
        value: "No Recently Closed Tabs.",
        comment: "Title for empty screen Recently Closed Tabs"
      )

    public static let recentlyClosedShakeActionDescription =
      NSLocalizedString(
        "recently.closed.shake.description",
        tableName: "BraveShared",
        bundle: .module,
        value: "Do you want to open the latest closed tab?",
        comment: "Description for alert to ask user for opening last closed tab in list"
      )

    public static let recentlyClosedOpenActionTitle =
      NSLocalizedString(
        "recently.closed.open",
        tableName: "BraveShared",
        bundle: .module,
        value: "Open",
        comment: "Open Tab action title"
      )

    public static let recentlyClosedReOpenLastActionTitle =
      NSLocalizedString(
        "recently.closed.reopen.latest",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reopen Last Closed Tab",
        comment: "Re-open Last Tab action title"
      )
  }
}

// MARK: - Voice Search

extension Strings {
  public struct VoiceSearch {
    public static let screenDisclaimer =
      NSLocalizedString(
        "voice.search.screen.disclaimer",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave does not store or share your voice searches.",
        comment: "Disclaimer for screen to search using voice."
      )

    public static let microphoneAccessRequiredWarningTitle =
      NSLocalizedString(
        "voice.search.microphone.title.required",
        tableName: "BraveShared",
        bundle: .module,
        value: "Microphone Access Required",
        comment: "Title for warning alert microphone access is required."
      )

    public static let microphoneAccessRequiredWarningDescription =
      NSLocalizedString(
        "voice.search.microphone.description.required",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Please allow Microphone Access in iOS System Settings for Brave to use anonymous voice search.",
        comment: "Explanation for warning alert why the microphone access required."
      )
  }
}

extension Strings {
  // Errors
  public static let unsupportedInstrumentMessage = NSLocalizedString(
    "unsupportedInstrumentMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Unsupported payment instruments",
    comment: "Error message if list of Payment Instruments doesn't include BAT"
  )
  public static let userCancelledMessage = NSLocalizedString(
    "userCancelledMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "User cancelled",
    comment: "Error message if the payment workflow is canceled by the user"
  )
  public static let invalidDetailsMessage = NSLocalizedString(
    "invalidDetailsMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Invalid details in payment request",
    comment: "Error message if details don't have the right type or values"
  )
  public static let clientErrorMessage = NSLocalizedString(
    "clientErrorMessage",
    tableName: "BraveShared",
    bundle: .module,
    value: "Client error",
    comment: "Client is in an invalid state which caused the error"
  )
}

extension Strings {
  public static let urlRedirectsSettings =
    NSLocalizedString(
      "urlRedirectsSettings",
      tableName: "BraveShared",
      bundle: .module,
      value: "Website Redirects",
      comment: "Setting title for option to automatically redirect a url to another url"
    )

  public static let redditRedirectFooter =
    NSLocalizedString(
      "redditRedirectFooter",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Automatically redirects Reddit links to the older interface (at old.reddit.com). To force the new Reddit interface, use new.reddit.com instead.",
      comment: "Setting title for option to automatically redirect a url to another url"
    )

  public static let nprRedirectFooter =
    NSLocalizedString(
      "nprRedirectFooter",
      tableName: "BraveShared",
      bundle: .module,
      value: "Automatically redirects NPR links to their text-based version (at text.npr.org).",
      comment: "Setting title for option to automatically redirect a url to another url"
    )
}

// MARK: Hotkey Titles

extension Strings {
  public struct Hotkey {
    public static let reloadPageTitle = NSLocalizedString(
      "ReloadPageTitle",
      bundle: .module,
      value: "Reload Page",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let backTitle = NSLocalizedString(
      "BackTitle",
      bundle: .module,
      value: "Back",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let forwardTitle = NSLocalizedString(
      "ForwardTitle",
      bundle: .module,
      value: "Forward",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let zoomInTitle = NSLocalizedString(
      "ZoomInTitle",
      bundle: .module,
      value: "Zoom In",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let zoomOutTitle = NSLocalizedString(
      "ZoomOutTitle",
      bundle: .module,
      value: "Zoom Out",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let selectLocationBarTitle = NSLocalizedString(
      "SelectLocationBarTitle",
      bundle: .module,
      value: "Select Location Bar",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let newTabTitle = NSLocalizedString(
      "NewTabTitle",
      bundle: .module,
      value: "New Tab",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let newPrivateTabTitle = NSLocalizedString(
      "NewPrivateTabTitle",
      bundle: .module,
      value: "New Private Tab",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let recentlyClosedTabTitle = NSLocalizedString(
      "RecentlyClosedTabTitle",
      bundle: .module,
      value: "Re-Open Closed Tab",
      comment:
        "Label to display in the Discoverability overlay for keyboard shortcuts used for opening recently closed tab."
    )
    public static let closeTabTitle = NSLocalizedString(
      "CloseTabTitle",
      bundle: .module,
      value: "Close Tab",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showNextTabTitle = NSLocalizedString(
      "ShowNextTabTitle",
      bundle: .module,
      value: "Show Next Tab",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showPreviousTabTitle = NSLocalizedString(
      "ShowPreviousTabTitle",
      bundle: .module,
      value: "Show Previous Tab",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showBookmarksTitle = NSLocalizedString(
      "showBookmarksTitle",
      bundle: .module,
      value: "Show Bookmarks",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showShieldsTitle = NSLocalizedString(
      "showShieldsTitle",
      bundle: .module,
      value: "Open Brave Shields",
      comment:
        "Label to display in the Discoverability overlay for keyboard shortcuts which is for Showing Brave Shields"
    )
    public static let showHistoryTitle = NSLocalizedString(
      "showHistoryTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show History",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showDownloadsTitle = NSLocalizedString(
      "showDownloadsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show Downloads",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let addBookmarkTitle = NSLocalizedString(
      "addBookmarkTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add Bookmark",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let addFavouritesTitle = NSLocalizedString(
      "addFavouritesTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add to Favourites",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let findInPageTitle = NSLocalizedString(
      "findInPageTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Find in Page",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let findNextTitle = NSLocalizedString(
      "findNextTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Find Next",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let findPreviousTitle = NSLocalizedString(
      "findPreviousTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Find Previous",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let shareWithTitle = NSLocalizedString(
      "shareWithTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Share with...",
      comment: "Label to display in the Discoverability overlay for keyboard shortcuts"
    )
    public static let showTabTrayFromTabKeyCodeTitle = NSLocalizedString(
      "ShowTabTrayFromTabKeyCodeTitle",
      bundle: .module,
      value: "Show All Tabs",
      comment:
        "Hardware shortcut to open the tab tray from a tab. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let closeTabFromTabTrayKeyCodeTitle = NSLocalizedString(
      "CloseTabFromTabTrayKeyCodeTitle",
      bundle: .module,
      value: "Close Selected Tab",
      comment:
        "Hardware shortcut to close the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let closeAllTabsFromTabTrayKeyCodeTitle = NSLocalizedString(
      "CloseAllTabsFromTabTrayKeyCodeTitle",
      bundle: .module,
      value: "Close All Tabs",
      comment:
        "Hardware shortcut to close all tabs from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let openSelectedTabFromTabTrayKeyCodeTitle = NSLocalizedString(
      "OpenSelectedTabFromTabTrayKeyCodeTitle",
      bundle: .module,
      value: "Open Selected Tab",
      comment:
        "Hardware shortcut open the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let openNewTabFromTabTrayKeyCodeTitle = NSLocalizedString(
      "OpenNewTabFromTabTrayKeyCodeTitle",
      bundle: .module,
      value: "Open New Tab",
      comment:
        "Hardware shortcut to open a new tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let switchToPBMKeyCodeTitle = NSLocalizedString(
      "SwitchToPBMKeyCodeTitle",
      bundle: .module,
      value: "Private Browsing Mode",
      comment:
        "Hardware shortcut switch to the private browsing tab or tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
    public static let switchToNonPBMKeyCodeTitle = NSLocalizedString(
      "SwitchToNonPBMKeyCodeTitle",
      bundle: .module,
      value: "Normal Browsing Mode",
      comment:
        "Hardware shortcut for non-private tab or tab. Shown in the Discoverability overlay when the hardware Command Key is held down."
    )
  }
}

// MARK: - Paywall

extension Strings {
  public struct Paywall {
    public static let alreadyPurchasedTitle = NSLocalizedString(
      "paywall.alreadyPurchasedTitle",
      bundle: .module,
      value: "Already purchased on brave.com?",
      comment: "Title label text for refreshing credentials from brave site."
    )
    public static let refreshCredentialsButtonTitle = NSLocalizedString(
      "paywall.refreshCredentialsButtonTitle",
      bundle: .module,
      value: "Refresh your credentials",
      comment: "Button action text for refreshing credentials from brave site."
    )
  }
}

extension Strings {
  public struct ShortcutButton {
    public static let hideButtonTitle = NSLocalizedString(
      "shortcutsButton.hideButtonTitle",
      bundle: .module,
      value: "Hide Shortcut Button",
      comment: "An option in a list of shortcut actions that hides the button"
    )
    public static let shortcutButtonTitle = NSLocalizedString(
      "shortcutsButton.shortcutButtonTitle",
      bundle: .module,
      value: "Shortcut Button",
      comment: "A title shown when letting the user choose a button's purpose from a list of destinations"
    )
  }
}
