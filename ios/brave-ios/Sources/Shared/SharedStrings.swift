// swift-format-ignore-file

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public static let OKString = NSLocalizedString(
    "OKString",
    bundle: .module,
    value: "OK",
    comment: "OK button"
  )
  public static let CancelString = NSLocalizedString(
    "CancelString",
    bundle: .module,
    value: "Cancel",
    comment: "Cancel button"
  )
}

// Settings.
extension Strings {
  public static let settingsGeneralSectionTitle = NSLocalizedString(
    "SettingsGeneralSectionTitle",
    bundle: .module,
    value: "General",
    comment: "General settings section title"
  )
  public static let settingsSearchDoneButton = NSLocalizedString(
    "SettingsSearchDoneButton",
    bundle: .module,
    value: "Done",
    comment: "Button displayed at the top of the search settings."
  )
  public static let bookmarksLastVisitedFolderTitle = NSLocalizedString(
    "BookmarksLastVisitedFolderTitle",
    bundle: .module,
    value: "Show Last Visited Bookmarks",
    comment: "General settings bookmarks last visited folder toggle title"
  )
}

// Logins Helper.
extension Strings {
  public static let loginsHelperSaveLoginButtonTitle = NSLocalizedString(
    "LoginsHelperSaveLoginButtonTitle",
    bundle: .module,
    value: "Save Login",
    comment: "Button to save the user's password"
  )
  public static let loginsHelperDontSaveButtonTitle = NSLocalizedString(
    "LoginsHelperDontSaveButtonTitle",
    bundle: .module,
    value: "Don’t Save",
    comment: "Button to not save the user's password"
  )
  public static let loginsHelperUpdateButtonTitle = NSLocalizedString(
    "LoginsHelperUpdateButtonTitle",
    bundle: .module,
    value: "Update",
    comment: "Button to update the user's password"
  )
  public static let loginsHelperDontUpdateButtonTitle = NSLocalizedString(
    "LoginsHelperDontUpdateButtonTitle",
    bundle: .module,
    value: "Don’t Update",
    comment: "Button to not update the user's password"
  )
}

// Brave Logins
extension Strings {
  public static let saveLoginUsernamePrompt = NSLocalizedString(
    "SaveLoginUsernamePrompt",
    bundle: .module,
    value: "Save login %@ for %@?",
    comment:
      "Prompt for saving a login. The first parameter is the username being saved. The second parameter is the hostname of the site."
  )
  public static let saveLoginPrompt = NSLocalizedString(
    "SaveLoginPrompt",
    bundle: .module,
    value: "Save password for %@?",
    comment:
      "Prompt for saving a password with no username. The parameter is the hostname of the site."
  )
  public static let updateLoginUsernamePrompt = NSLocalizedString(
    "UpdateLoginUsernamePrompt",
    bundle: .module,
    value: "Update login %@ for %@?",
    comment:
      "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site."
  )
  public static let updateLoginPrompt = NSLocalizedString(
    "UpdateLoginPrompt",
    bundle: .module,
    value: "Update login %@ for %@?",
    comment:
      "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site."
  )
}

// Tabs Delete All Undo Toast
extension Strings {
  public static let tabsDeleteAllUndoTitle = NSLocalizedString(
    "TabsDeleteAllUndoTitle",
    bundle: .module,
    value: "%d tab(s) closed",
    comment: "The label indicating that all the tabs were closed"
  )
  public static let tabsDeleteAllUndoAction = NSLocalizedString(
    "TabsDeleteAllUndoAction",
    bundle: .module,
    value: "Undo",
    comment: "The button to undo the delete all tabs"
  )
}

// Clipboard Toast
extension Strings {
  public static let goButtonTittle = NSLocalizedString(
    "GoButtonTittle",
    bundle: .module,
    value: "Go",
    comment: "The button to open a new tab with the copied link"
  )
}

// errors
extension Strings {
  public static let unableToAddPassErrorTitle = NSLocalizedString(
    "UnableToAddPassErrorTitle",
    bundle: .module,
    value: "Failed to Add Pass",
    comment:
      "Title of the 'Add Pass Failed' alert. See https://support.apple.com/HT204003 for context on Wallet."
  )
  public static let unableToAddPassErrorMessage = NSLocalizedString(
    "UnableToAddPassErrorMessage",
    bundle: .module,
    value: "An error occurred while adding the pass to Wallet. Please try again later.",
    comment:
      "Text of the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet."
  )
  public static let unableToAddPassErrorDismiss = NSLocalizedString(
    "UnableToAddPassErrorDismiss",
    bundle: .module,
    value: "OK",
    comment:
      "Button to dismiss the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet."
  )
  public static let unableToOpenURLError = NSLocalizedString(
    "UnableToOpenURLError",
    bundle: .module,
    value: "Brave cannot open the page because it has an invalid address.",
    comment:
      "The message displayed to a user when they try to open a URL that cannot be handled by Brave, or any external app."
  )
  public static let unableToOpenURLErrorTitle = NSLocalizedString(
    "UnableToOpenURLErrorTitle",
    bundle: .module,
    value: "Cannot Open Page",
    comment: "Title of the message shown when the user attempts to navigate to an invalid link."
  )
}

// Download Helper
extension Strings {
  public static let downloadsButtonTitle = NSLocalizedString(
    "DownloadsButtonTitle",
    bundle: .module,
    value: "Downloads",
    comment: "The button to open a new tab with the Downloads home panel"
  )
  public static let cancelDownloadDialogTitle = NSLocalizedString(
    "CancelDownloadDialogTitle",
    bundle: .module,
    value: "Cancel Download",
    comment: "Alert dialog title when the user taps the cancel download icon."
  )
  public static let cancelDownloadDialogMessage = NSLocalizedString(
    "CancelDownloadDialogMessage",
    bundle: .module,
    value: "Are you sure you want to cancel this download?",
    comment: "Alert dialog body when the user taps the cancel download icon."
  )
  public static let cancelDownloadDialogResume = NSLocalizedString(
    "CancelDownloadDialogResume",
    bundle: .module,
    value: "Resume",
    comment: "Button declining the cancellation of the download."
  )
  public static let cancelDownloadDialogCancel = NSLocalizedString(
    "CancelDownloadDialogCancel",
    bundle: .module,
    value: "Cancel",
    comment: "Button confirming the cancellation of the download."
  )
  public static let downloadCancelledToastLabelText = NSLocalizedString(
    "DownloadCancelledToastLabelText",
    bundle: .module,
    value: "Download Cancelled",
    comment:
      "The label text in the Download Cancelled toast for showing confirmation that the download was cancelled."
  )
  public static let downloadFailedToastLabelText = NSLocalizedString(
    "DownloadFailedToastLabelText",
    bundle: .module,
    value: "Download Failed",
    comment:
      "The label text in the Download Failed toast for showing confirmation that the download has failed."
  )
  public static let downloadMultipleFilesToastDescriptionText = NSLocalizedString(
    "DownloadMultipleFilesToastDescriptionText",
    bundle: .module,
    value: "1 of %d files",
    comment:
      "The description text in the Download progress toast for showing the number of files when multiple files are downloading."
  )
  public static let downloadProgressToastDescriptionText = NSLocalizedString(
    "DownloadProgressToastDescriptionText",
    bundle: .module,
    value: "%1$@/%2$@",
    comment:
      "The description text in the Download progress toast for showing the downloaded file size (1$) out of the total expected file size (2$)."
  )
  public static let downloadMultipleFilesAndProgressToastDescriptionText = NSLocalizedString(
    "DownloadMultipleFilesAndProgressToastDescriptionText",
    bundle: .module,
    value: "%1$@ %2$@",
    comment:
      "The description text in the Download progress toast for showing the number of files (1$) and download progress (2$). This string only consists of two placeholders for purposes of displaying two other strings side-by-side where 1$ is Downloads.Toast.MultipleFiles.DescriptionText and 2$ is Downloads.Toast.Progress.DescriptionText. This string should only consist of the two placeholders side-by-side separated by a single space and 1$ should come before 2$ everywhere except for right-to-left locales."
  )
}

// Context menu ButtonToast instances.
extension Strings {
  public static let contextMenuButtonToastNewTabOpenedLabelText = NSLocalizedString(
    "ContextMenuButtonToastNewTabOpenedLabelText",
    bundle: .module,
    value: "New Tab opened",
    comment: "The label text in the Button Toast for switching to a fresh New Tab."
  )
  public static let contextMenuButtonToastNewTabOpenedButtonText = NSLocalizedString(
    "ContextMenuButtonToastNewTabOpenedButtonText",
    bundle: .module,
    value: "Switch",
    comment: "The button text in the Button Toast for switching to a fresh New Tab."
  )
}

// Reader Mode.
extension Strings {
  public static let readerModeAvailableVoiceOverAnnouncement = NSLocalizedString(
    "ReaderModeAvailableVoiceOverAnnouncement",
    bundle: .module,
    value: "Reader Mode available",
    comment: "Accessibility message e.g. spoken by VoiceOver when Reader Mode becomes available."
  )
  public static let readerModeResetFontSizeAccessibilityLabel = NSLocalizedString(
    "ReaderModeResetFontSizeAccessibilityLabel",
    bundle: .module,
    value: "Reset text size",
    comment: "Accessibility label for button resetting font size in display settings of reader mode"
  )
}

// QR Code scanner.
extension Strings {
  public static let scanQRCodeViewTitle = NSLocalizedString(
    "ScanQRCodeViewTitle",
    bundle: .module,
    value: "Scan QR Code",
    comment: "Title for the QR code scanner view."
  )
  public static let scanQRCodeInstructionsLabel = NSLocalizedString(
    "ScanQRCodeInstructionsLabel",
    bundle: .module,
    value: "Align QR code within frame to scan",
    comment: "Text for the instructions label, displayed in the QR scanner view"
  )
  public static let scanQRCodeInvalidDataErrorMessage = NSLocalizedString(
    "ScanQRCodeInvalidDataErrorMessage",
    bundle: .module,
    value: "The data is invalid",
    comment: "Text of the prompt that is shown to the user when the data is invalid"
  )
  public static let scanQRCodePermissionErrorMessage = NSLocalizedString(
    "ScanQRCodePermissionErrorMessage",
    bundle: .module,
    value:
      "Please allow Brave to access your device’s camera in ‘Settings’ -> ‘Privacy’ -> ‘Camera’.",
    comment: "Text of the prompt user to setup the camera authorization."
  )
  public static let scanQRCodeErrorOKButton = NSLocalizedString(
    "ScanQRCodeErrorOKButton",
    bundle: .module,
    value: "OK",
    comment: "OK button to dismiss the error prompt."
  )
}

// App menu.
extension Strings {
  public static let appMenuViewDesktopSiteTitleString = NSLocalizedString(
    "AppMenuViewDesktopSiteTitleString",
    bundle: .module,
    value: "Request Desktop Site",
    comment:
      "Label for the button, displayed in the menu, used to request the desktop version of the current website."
  )
  public static let appMenuViewMobileSiteTitleString = NSLocalizedString(
    "AppMenuViewMobileSiteTitleString",
    bundle: .module,
    value: "Request Mobile Site",
    comment:
      "Label for the button, displayed in the menu, used to request the mobile version of the current website."
  )
  public static let appMenuCopyURLConfirmMessage = NSLocalizedString(
    "AppMenuCopyURLConfirmMessage",
    bundle: .module,
    value: "URL Copied To Clipboard",
    comment: "Toast displayed to user after copy url pressed."
  )
}

// Snackbar shown when tapping app store link
extension Strings {
  public static let externalLinkAppStoreConfirmationTitle = NSLocalizedString(
    "ExternalLinkAppStoreConfirmationTitle",
    bundle: .module,
    value: "Open this link in the App Store app?",
    comment: "Question shown to user when tapping a link that opens the App Store app"
  )
}

// Search result ad clicked InfoBar title and learn more / opt out choices link.
extension Strings {
  public static let searchResultAdClickedInfoBarTitle = NSLocalizedString(
    "SearchResultAdClickedInfoBarTitle",
    bundle: .module,
    value: "Thanks for supporting Brave Search by clicking a private ad. Unlike Big Tech, we measure ad performance anonymously to preserve your privacy.",
    comment: "The text label of creative search result ad infobar message."
  )

  public static let searchResultAdClickedLearnMoreOptOutChoicesLabel = NSLocalizedString(
    "SearchResultAdClickedLearnMoreOptOutChoicesLabel",
    bundle: .module,
    value: "Learn more / opt out choices",
    comment: "The text label of creative search result ad learn more / opt out choices link."
  )
}
