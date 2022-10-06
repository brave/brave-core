/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
@_exported import Strings

extension Strings {
  public static let OKString = NSLocalizedString("OKString", bundle: .strings, value: "OK", comment: "OK button")
  public static let CancelString = NSLocalizedString("CancelString", bundle: .strings, value: "Cancel", comment: "Cancel button")
}

// Settings.
extension Strings {
  public static let settingsGeneralSectionTitle = NSLocalizedString("SettingsGeneralSectionTitle", bundle: .strings, value: "General", comment: "General settings section title")
  public static let settingsSearchDoneButton = NSLocalizedString("SettingsSearchDoneButton", bundle: .strings, value: "Done", comment: "Button displayed at the top of the search settings.")
  public static let bookmarksLastVisitedFolderTitle = NSLocalizedString("BookmarksLastVisitedFolderTitle", bundle: .strings, value: "Show Last Visited Bookmarks", comment: "General settings bookmarks last visited folder toggle title")
}

// Logins Helper.
extension Strings {
  public static let loginsHelperSaveLoginButtonTitle = NSLocalizedString("LoginsHelperSaveLoginButtonTitle", bundle: .strings, value: "Save Login", comment: "Button to save the user's password")
  public static let loginsHelperDontSaveButtonTitle = NSLocalizedString("LoginsHelperDontSaveButtonTitle", bundle: .strings, value: "Don’t Save", comment: "Button to not save the user's password")
  public static let loginsHelperUpdateButtonTitle = NSLocalizedString("LoginsHelperUpdateButtonTitle", bundle: .strings, value: "Update", comment: "Button to update the user's password")
  public static let loginsHelperDontUpdateButtonTitle = NSLocalizedString("LoginsHelperDontUpdateButtonTitle", bundle: .strings, value: "Don’t Update", comment: "Button to not update the user's password")
}

// Brave Logins
extension Strings {
  public static let saveLoginUsernamePrompt = NSLocalizedString("SaveLoginUsernamePrompt", bundle: .strings, value: "Save login %@ for %@?", comment: "Prompt for saving a login. The first parameter is the username being saved. The second parameter is the hostname of the site.")
  public static let saveLoginPrompt = NSLocalizedString("SaveLoginPrompt", bundle: .strings, value: "Save password for %@?", comment: "Prompt for saving a password with no username. The parameter is the hostname of the site.")
  public static let updateLoginUsernamePrompt = NSLocalizedString("UpdateLoginUsernamePrompt", bundle: .strings, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
  public static let updateLoginPrompt = NSLocalizedString("UpdateLoginPrompt", bundle: .strings, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
}

// Hotkey Titles
extension Strings {
  public static let reloadPageTitle = NSLocalizedString("ReloadPageTitle", bundle: .strings, value: "Reload Page", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let backTitle = NSLocalizedString("BackTitle", bundle: .strings, value: "Back", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let forwardTitle = NSLocalizedString("ForwardTitle", bundle: .strings, value: "Forward", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let selectLocationBarTitle = NSLocalizedString("SelectLocationBarTitle", bundle: .strings, value: "Select Location Bar", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let newTabTitle = NSLocalizedString("NewTabTitle", bundle: .strings, value: "New Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let newPrivateTabTitle = NSLocalizedString("NewPrivateTabTitle", bundle: .strings, value: "New Private Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let closeTabTitle = NSLocalizedString("CloseTabTitle", bundle: .strings, value: "Close Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let showNextTabTitle = NSLocalizedString("ShowNextTabTitle", bundle: .strings, value: "Show Next Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let showPreviousTabTitle = NSLocalizedString("ShowPreviousTabTitle", bundle: .strings, value: "Show Previous Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let showBookmarksTitle = NSLocalizedString("showBookmarksTitle", bundle: .strings, value: "Show Bookmarks", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
  public static let showShieldsTitle = NSLocalizedString("showShieldsTitle", bundle: .strings, value: "Open Brave Shields", comment: "Label to display in the Discoverability overlay for keyboard shortcuts which is for Showing Brave Shields")
}

// Tabs Delete All Undo Toast
extension Strings {
  public static let tabsDeleteAllUndoTitle = NSLocalizedString("TabsDeleteAllUndoTitle", bundle: .strings, value: "%d tab(s) closed", comment: "The label indicating that all the tabs were closed")
  public static let tabsDeleteAllUndoAction = NSLocalizedString("TabsDeleteAllUndoAction", bundle: .strings, value: "Undo", comment: "The button to undo the delete all tabs")
}

// Clipboard Toast
extension Strings {
  public static let goButtonTittle = NSLocalizedString("GoButtonTittle", bundle: .strings, value: "Go", comment: "The button to open a new tab with the copied link")
}

// errors
extension Strings {
  public static let unableToAddPassErrorTitle = NSLocalizedString("UnableToAddPassErrorTitle", bundle: .strings, value: "Failed to Add Pass", comment: "Title of the 'Add Pass Failed' alert. See https://support.apple.com/HT204003 for context on Wallet.")
  public static let unableToAddPassErrorMessage = NSLocalizedString("UnableToAddPassErrorMessage", bundle: .strings, value: "An error occurred while adding the pass to Wallet. Please try again later.", comment: "Text of the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
  public static let unableToAddPassErrorDismiss = NSLocalizedString("UnableToAddPassErrorDismiss", bundle: .strings, value: "OK", comment: "Button to dismiss the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
  public static let unableToOpenURLError = NSLocalizedString("UnableToOpenURLError", bundle: .strings, value: "Brave cannot open the page because it has an invalid address.", comment: "The message displayed to a user when they try to open a URL that cannot be handled by Brave, or any external app.")
  public static let unableToOpenURLErrorTitle = NSLocalizedString("UnableToOpenURLErrorTitle", bundle: .strings, value: "Cannot Open Page", comment: "Title of the message shown when the user attempts to navigate to an invalid link.")
}

// Download Helper
extension Strings {
  public static let downloadsButtonTitle = NSLocalizedString("DownloadsButtonTitle", bundle: .strings, value: "Downloads", comment: "The button to open a new tab with the Downloads home panel")
  public static let cancelDownloadDialogTitle = NSLocalizedString("CancelDownloadDialogTitle", bundle: .strings, value: "Cancel Download", comment: "Alert dialog title when the user taps the cancel download icon.")
  public static let cancelDownloadDialogMessage = NSLocalizedString("CancelDownloadDialogMessage", bundle: .strings, value: "Are you sure you want to cancel this download?", comment: "Alert dialog body when the user taps the cancel download icon.")
  public static let cancelDownloadDialogResume = NSLocalizedString("CancelDownloadDialogResume", bundle: .strings, value: "Resume", comment: "Button declining the cancellation of the download.")
  public static let cancelDownloadDialogCancel = NSLocalizedString("CancelDownloadDialogCancel", bundle: .strings, value: "Cancel", comment: "Button confirming the cancellation of the download.")
  public static let downloadCancelledToastLabelText = NSLocalizedString("DownloadCancelledToastLabelText", bundle: .strings, value: "Download Cancelled", comment: "The label text in the Download Cancelled toast for showing confirmation that the download was cancelled.")
  public static let downloadFailedToastLabelText = NSLocalizedString("DownloadFailedToastLabelText", bundle: .strings, value: "Download Failed", comment: "The label text in the Download Failed toast for showing confirmation that the download has failed.")
  public static let downloadMultipleFilesToastDescriptionText = NSLocalizedString("DownloadMultipleFilesToastDescriptionText", bundle: .strings, value: "1 of %d files", comment: "The description text in the Download progress toast for showing the number of files when multiple files are downloading.")
  public static let downloadProgressToastDescriptionText = NSLocalizedString("DownloadProgressToastDescriptionText", bundle: .strings, value: "%1$@/%2$@", comment: "The description text in the Download progress toast for showing the downloaded file size (1$) out of the total expected file size (2$).")
  public static let downloadMultipleFilesAndProgressToastDescriptionText = NSLocalizedString("DownloadMultipleFilesAndProgressToastDescriptionText", bundle: .strings, value: "%1$@ %2$@", comment: "The description text in the Download progress toast for showing the number of files (1$) and download progress (2$). This string only consists of two placeholders for purposes of displaying two other strings side-by-side where 1$ is Downloads.Toast.MultipleFiles.DescriptionText and 2$ is Downloads.Toast.Progress.DescriptionText. This string should only consist of the two placeholders side-by-side separated by a single space and 1$ should come before 2$ everywhere except for right-to-left locales.")
}

// Context menu ButtonToast instances.
extension Strings {
  public static let contextMenuButtonToastNewTabOpenedLabelText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedLabelText", bundle: .strings, value: "New Tab opened", comment: "The label text in the Button Toast for switching to a fresh New Tab.")
  public static let contextMenuButtonToastNewTabOpenedButtonText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedButtonText", bundle: .strings, value: "Switch", comment: "The button text in the Button Toast for switching to a fresh New Tab.")
}

// Reader Mode.
extension Strings {
  public static let readerModeAvailableVoiceOverAnnouncement = NSLocalizedString("ReaderModeAvailableVoiceOverAnnouncement", bundle: .strings, value: "Reader Mode available", comment: "Accessibility message e.g. spoken by VoiceOver when Reader Mode becomes available.")
  public static let readerModeResetFontSizeAccessibilityLabel = NSLocalizedString("ReaderModeResetFontSizeAccessibilityLabel", bundle: .strings, value: "Reset text size", comment: "Accessibility label for button resetting font size in display settings of reader mode")
}

// QR Code scanner.
extension Strings {
  public static let scanQRCodeViewTitle = NSLocalizedString("ScanQRCodeViewTitle", bundle: .strings, value: "Scan QR Code", comment: "Title for the QR code scanner view.")
  public static let scanQRCodeInstructionsLabel = NSLocalizedString("ScanQRCodeInstructionsLabel", bundle: .strings, value: "Align QR code within frame to scan", comment: "Text for the instructions label, displayed in the QR scanner view")
  public static let scanQRCodeInvalidDataErrorMessage = NSLocalizedString("ScanQRCodeInvalidDataErrorMessage", bundle: .strings, value: "The data is invalid", comment: "Text of the prompt that is shown to the user when the data is invalid")
  public static let scanQRCodePermissionErrorMessage = NSLocalizedString("ScanQRCodePermissionErrorMessage", bundle: .strings, value: "Please allow Brave to access your device’s camera in ‘Settings’ -> ‘Privacy’ -> ‘Camera’.", comment: "Text of the prompt user to setup the camera authorization.")
  public static let scanQRCodeErrorOKButton = NSLocalizedString("ScanQRCodeErrorOKButton", bundle: .strings, value: "OK", comment: "OK button to dismiss the error prompt.")
}

// App menu.
extension Strings {
  public static let appMenuViewDesktopSiteTitleString = NSLocalizedString("AppMenuViewDesktopSiteTitleString", bundle: .strings, value: "Request Desktop Site", comment: "Label for the button, displayed in the menu, used to request the desktop version of the current website.")
  public static let appMenuViewMobileSiteTitleString = NSLocalizedString("AppMenuViewMobileSiteTitleString", bundle: .strings, value: "Request Mobile Site", comment: "Label for the button, displayed in the menu, used to request the mobile version of the current website.")
  public static let appMenuCopyURLConfirmMessage = NSLocalizedString("AppMenuCopyURLConfirmMessage", bundle: .strings, value: "URL Copied To Clipboard", comment: "Toast displayed to user after copy url pressed.")
}

// Snackbar shown when tapping app store link
extension Strings {
  public static let externalLinkAppStoreConfirmationTitle = NSLocalizedString("ExternalLinkAppStoreConfirmationTitle", bundle: .strings, value: "Open this link in the App Store app?", comment: "Question shown to user when tapping a link that opens the App Store app")
}

// Location bar long press menu
extension Strings {
  public static let pasteAndGoTitle = NSLocalizedString("PasteAndGoTitle", bundle: .strings, value: "Paste & Go", comment: "The title for the button that lets you paste and go to a URL")
  public static let pasteTitle = NSLocalizedString("PasteTitle", bundle: .strings, value: "Paste", comment: "The title for the button that lets you paste into the location bar")
  public static let copyAddressTitle = NSLocalizedString("CopyAddressTitle", bundle: .strings, value: "Copy Address", comment: "The title for the button that lets you copy the url from the location bar.")
}

// Keyboard short cuts
extension Strings {
  public static let showTabTrayFromTabKeyCodeTitle = NSLocalizedString("ShowTabTrayFromTabKeyCodeTitle", bundle: .strings, value: "Show All Tabs", comment: "Hardware shortcut to open the tab tray from a tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let closeTabFromTabTrayKeyCodeTitle = NSLocalizedString("CloseTabFromTabTrayKeyCodeTitle", bundle: .strings, value: "Close Selected Tab", comment: "Hardware shortcut to close the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let closeAllTabsFromTabTrayKeyCodeTitle = NSLocalizedString("CloseAllTabsFromTabTrayKeyCodeTitle", bundle: .strings, value: "Close All Tabs", comment: "Hardware shortcut to close all tabs from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let openSelectedTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenSelectedTabFromTabTrayKeyCodeTitle", bundle: .strings, value: "Open Selected Tab", comment: "Hardware shortcut open the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let openNewTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenNewTabFromTabTrayKeyCodeTitle", bundle: .strings, value: "Open New Tab", comment: "Hardware shortcut to open a new tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let switchToPBMKeyCodeTitle = NSLocalizedString("SwitchToPBMKeyCodeTitle", bundle: .strings, value: "Private Browsing Mode", comment: "Hardware shortcut switch to the private browsing tab or tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
  public static let switchToNonPBMKeyCodeTitle = NSLocalizedString("SwitchToNonPBMKeyCodeTitle", bundle: .strings, value: "Normal Browsing Mode", comment: "Hardware shortcut for non-private tab or tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
}
