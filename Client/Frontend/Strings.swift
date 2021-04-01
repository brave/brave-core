/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

public struct Strings {}

/// Return the main application bundle. Even if called from an extension. If for some reason we cannot find the
/// application bundle, the current bundle is returned, which will then result in an English base language string.
private func applicationBundle() -> Bundle {
    let bundle = Bundle.main
    guard bundle.bundleURL.pathExtension == "appex", let applicationBundleURL = (bundle.bundleURL as NSURL).deletingLastPathComponent?.deletingLastPathComponent() else {
        return bundle
    }
    return Bundle(url: applicationBundleURL) ?? bundle
}

extension Strings {
    public static let OKString = NSLocalizedString("OKString", bundle: Bundle.shared, value: "OK", comment: "OK button")
    public static let CancelString = NSLocalizedString("CancelString", bundle: Bundle.shared, value: "Cancel", comment: "Cancel button")
}

// Settings.
extension Strings {
    public static let settingsGeneralSectionTitle = NSLocalizedString("SettingsGeneralSectionTitle", bundle: Bundle.shared, value: "General", comment: "General settings section title")
    public static let settingsSearchDoneButton = NSLocalizedString("SettingsSearchDoneButton", bundle: Bundle.shared, value: "Done", comment: "Button displayed at the top of the search settings.")
    public static let settingsSearchEditButton = NSLocalizedString("SettingsSearchEditButton", bundle: Bundle.shared, value: "Edit", comment: "Button displayed at the top of the search settings.")
    public static let useTouchID = NSLocalizedString("UseTouchID", bundle: Bundle.shared, value: "Use Touch ID", comment: "List section title for when to use Touch ID")
    public static let useFaceID = NSLocalizedString("UseFaceID", bundle: Bundle.shared, value: "Use Face ID", comment: "List section title for when to use Face ID")
    
    public static let bookmarksLastVisitedFolderTitle = NSLocalizedString("BookmarksLastVisitedFolderTitle", bundle: Bundle.shared, value: "Show Last Visited Bookmarks", comment: "General settings bookmarks last visited folder toggle title")
}

// Logins Helper.
extension Strings {
    public static let loginsHelperSaveLoginButtonTitle = NSLocalizedString("LoginsHelperSaveLoginButtonTitle", bundle: Bundle.shared, value: "Save Login", comment: "Button to save the user's password")
    public static let loginsHelperDontSaveButtonTitle = NSLocalizedString("LoginsHelperDontSaveButtonTitle", bundle: Bundle.shared, value: "Don’t Save", comment: "Button to not save the user's password")
    public static let loginsHelperUpdateButtonTitle = NSLocalizedString("LoginsHelperUpdateButtonTitle", bundle: Bundle.shared, value: "Update", comment: "Button to update the user's password")
    public static let loginsHelperDontUpdateButtonTitle = NSLocalizedString("LoginsHelperDontUpdateButtonTitle", bundle: Bundle.shared, value: "Don’t Update", comment: "Button to not update the user's password")
}

// Brave Logins
extension Strings {
    public static let saveLoginUsernamePrompt = NSLocalizedString("SaveLoginUsernamePrompt", bundle: Bundle.shared, value: "Save login %@ for %@?", comment: "Prompt for saving a login. The first parameter is the username being saved. The second parameter is the hostname of the site.")
    public static let saveLoginPrompt = NSLocalizedString("SaveLoginPrompt", bundle: Bundle.shared, value: "Save password for %@?", comment: "Prompt for saving a password with no username. The parameter is the hostname of the site.")
    public static let updateLoginUsernamePrompt = NSLocalizedString("UpdateLoginUsernamePrompt", bundle: Bundle.shared, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
    public static let updateLoginPrompt = NSLocalizedString("UpdateLoginPrompt", bundle: Bundle.shared, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
}

// Hotkey Titles
extension Strings {
    public static let reloadPageTitle = NSLocalizedString("ReloadPageTitle", bundle: Bundle.shared, value: "Reload Page", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let backTitle = NSLocalizedString("BackTitle", bundle: Bundle.shared, value: "Back", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let forwardTitle = NSLocalizedString("ForwardTitle", bundle: Bundle.shared, value: "Forward", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")

    public static let findTitle = NSLocalizedString("FindTitle", bundle: Bundle.shared, value: "Find", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let selectLocationBarTitle = NSLocalizedString("SelectLocationBarTitle", bundle: Bundle.shared, value: "Select Location Bar", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let newTabTitle = NSLocalizedString("NewTabTitle", bundle: Bundle.shared, value: "New Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let newPrivateTabTitle = NSLocalizedString("NewPrivateTabTitle", bundle: Bundle.shared, value: "New Private Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let closeTabTitle = NSLocalizedString("CloseTabTitle", bundle: Bundle.shared, value: "Close Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let showNextTabTitle = NSLocalizedString("ShowNextTabTitle", bundle: Bundle.shared, value: "Show Next Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let showPreviousTabTitle = NSLocalizedString("ShowPreviousTabTitle", bundle: Bundle.shared, value: "Show Previous Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
}

// Home page.
extension Strings {
    public static let setHomePageDialogTitle = NSLocalizedString("SetHomePageDialogTitle", bundle: Bundle.shared, value: "Do you want to use this web page as your home page?", comment: "Alert dialog title when the user opens the home page for the first time.")
    public static let setHomePageDialogMessage = NSLocalizedString("SetHomePageDialogMessage", bundle: Bundle.shared, value: "You can change this at any time in Settings", comment: "Alert dialog body when the user opens the home page for the first time.")
    public static let setHomePageDialogYes = NSLocalizedString("SetHomePageDialogYes", bundle: Bundle.shared, value: "Set Homepage", comment: "Button accepting changes setting the home page for the first time.")
    public static let setHomePageDialogNo = NSLocalizedString("SetHomePageDialogNo", bundle: Bundle.shared, value: "Cancel", comment: "Button cancelling changes setting the home page for the first time.")
}

// New tab choice settings
extension Strings {
    public static let settingsNewTabTopSites = NSLocalizedString("SettingsNewTabTopSites", bundle: Bundle.shared, value: "Show your Top Sites", comment: "Option in settings to show top sites when you open a new tab")
    public static let settingsNewTabBlankPage = NSLocalizedString("SettingsNewTabBlankPage", bundle: Bundle.shared, value: "Show a Blank Page", comment: "Option in settings to show a blank page when you open a new tab")
    public static let settingsNewTabHomePage = NSLocalizedString("SettingsNewTabHomePage", bundle: Bundle.shared, value: "Show your Homepage", comment: "Option in settings to show your homepage when you open a new tab")
}

// Custom account settings - These strings did not make it for the v10 l10n deadline so we have turned them into regular strings. These strings will come back localized in a next version.

extension Strings {
    // Settings.AdvanceAccount.SectionName
    // Label used as an item in Settings. When touched it will open a dialog to setup advance Brave account settings.
    public static let settingsAdvanceAccountSectionName = "Account Settings"

    // Settings.AdvanceAccount.SectionFooter
    // Details for using custom Brave Account service.
    public static let settingsAdvanceAccountSectionFooter = "To use a custom Brave Account and sync servers, specify the root Url of the Brave Account site. This will download the configuration and setup this device to use the new service. After the new service has been set, you will need to create a new Brave Account or login with an existing one."

    // Settings.AdvanceAccount.SectionName
    // Title displayed in header of the setting panel.
    public static let settingsAdvanceAccountTitle = "Advance Account Settings"

    // Settings.AdvanceAccount.UrlPlaceholder
    // Title displayed in header of the setting panel.
    public static let settingsAdvanceAccountUrlPlaceholder = "Custom Account Url"

    // Settings.AdvanceAccount.UpdatedAlertMessage
    // Messaged displayed when sync service has been successfully set.
    public static let settingsAdvanceAccountUrlUpdatedAlertMessage = "Brave account service updated. To begin using custom server, please log out and re-login."

    // Settings.AdvanceAccount.UpdatedAlertOk
    // Ok button on custom sync service updated alert
    public static let settingsAdvanceAccountUrlUpdatedAlertOk = "OK"

    // Settings.AdvanceAccount.ErrorAlertTitle
    // Error alert message title.
    public static let settingsAdvanceAccountUrlErrorAlertTitle = "Error"

    // Settings.AdvanceAccount.ErrorAlertMessage
    // Messaged displayed when sync service has an error setting a custom sync url.
    public static let settingsAdvanceAccountUrlErrorAlertMessage = "There was an error while attempting to parse the url. Please make sure that it is a valid Brave Account root url."

    // Settings.AdvanceAccount.ErrorAlertOk
    // Ok button on custom sync service error alert.
    public static let settingsAdvanceAccountUrlErrorAlertOk = "OK"

    // Settings.AdvanceAccount.UseCustomAccountsServiceTitle
    // Toggle switch to use custom FxA server
    public static let settingsAdvanceAccountUseCustomAccountsServiceTitle = "Use Custom Account Service"

    // Settings.AdvanceAccount.UrlEmptyErrorAlertMessage
    // No custom service set.
    public static let settingsAdvanceAccountEmptyUrlErrorAlertMessage = "Please enter a custom account url before enabling."
}

// Tabs Delete All Undo Toast
extension Strings {
    public static let tabsDeleteAllUndoTitle = NSLocalizedString("TabsDeleteAllUndoTitle", bundle: Bundle.shared, value: "%d tab(s) closed", comment: "The label indicating that all the tabs were closed")
    public static let tabsDeleteAllUndoAction = NSLocalizedString("TabsDeleteAllUndoAction", bundle: Bundle.shared, value: "Undo", comment: "The button to undo the delete all tabs")
}

// Clipboard Toast
extension Strings {
    public static let goToCopiedLink = NSLocalizedString("GoToCopiedLink", bundle: Bundle.shared, value: "Go to copied link?", comment: "Message displayed when the user has a copied link on the clipboard")
    public static let goButtonTittle = NSLocalizedString("GoButtonTittle", bundle: Bundle.shared, value: "Go", comment: "The button to open a new tab with the copied link")
}

// errors
extension Strings {
    public static let unableToAddPassErrorTitle = NSLocalizedString("UnableToAddPassErrorTitle", bundle: Bundle.shared, value: "Failed to Add Pass", comment: "Title of the 'Add Pass Failed' alert. See https://support.apple.com/HT204003 for context on Wallet.")
    public static let unableToAddPassErrorMessage = NSLocalizedString("UnableToAddPassErrorMessage", bundle: Bundle.shared, value: "An error occurred while adding the pass to Wallet. Please try again later.", comment: "Text of the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
    public static let unableToAddPassErrorDismiss = NSLocalizedString("UnableToAddPassErrorDismiss", bundle: Bundle.shared, value: "OK", comment: "Button to dismiss the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
    public static let unableToOpenURLError = NSLocalizedString("UnableToOpenURLError", bundle: Bundle.shared, value: "Brave cannot open the page because it has an invalid address.", comment: "The message displayed to a user when they try to open a URL that cannot be handled by Brave, or any external app.")
    public static let unableToOpenURLErrorTitle = NSLocalizedString("UnableToOpenURLErrorTitle", bundle: Bundle.shared, value: "Cannot Open Page", comment: "Title of the message shown when the user attempts to navigate to an invalid link.")
}

// Download Helper
extension Strings {
    public static let downloadsButtonTitle = NSLocalizedString("DownloadsButtonTitle", bundle: Bundle.shared, value: "Downloads", comment: "The button to open a new tab with the Downloads home panel")
    public static let cancelDownloadDialogTitle = NSLocalizedString("CancelDownloadDialogTitle", bundle: Bundle.shared, value: "Cancel Download", comment: "Alert dialog title when the user taps the cancel download icon.")
    public static let cancelDownloadDialogMessage = NSLocalizedString("CancelDownloadDialogMessage", bundle: Bundle.shared, value: "Are you sure you want to cancel this download?", comment: "Alert dialog body when the user taps the cancel download icon.")
    public static let cancelDownloadDialogResume = NSLocalizedString("CancelDownloadDialogResume", bundle: Bundle.shared, value: "Resume", comment: "Button declining the cancellation of the download.")
    public static let cancelDownloadDialogCancel = NSLocalizedString("CancelDownloadDialogCancel", bundle: Bundle.shared, value: "Cancel", comment: "Button confirming the cancellation of the download.")
    public static let downloadCancelledToastLabelText = NSLocalizedString("DownloadCancelledToastLabelText", bundle: Bundle.shared, value: "Download Cancelled", comment: "The label text in the Download Cancelled toast for showing confirmation that the download was cancelled.")
    public static let downloadFailedToastLabelText = NSLocalizedString("DownloadFailedToastLabelText", bundle: Bundle.shared, value: "Download Failed", comment: "The label text in the Download Failed toast for showing confirmation that the download has failed.")
    public static let downloadMultipleFilesToastDescriptionText = NSLocalizedString("DownloadMultipleFilesToastDescriptionText", bundle: Bundle.shared, value: "1 of %d files", comment: "The description text in the Download progress toast for showing the number of files when multiple files are downloading.")
    public static let downloadProgressToastDescriptionText = NSLocalizedString("DownloadProgressToastDescriptionText", bundle: Bundle.shared, value: "%1$@/%2$@", comment: "The description text in the Download progress toast for showing the downloaded file size (1$) out of the total expected file size (2$).")
    public static let downloadMultipleFilesAndProgressToastDescriptionText = NSLocalizedString("DownloadMultipleFilesAndProgressToastDescriptionText", bundle: Bundle.shared, value: "%1$@ %2$@", comment: "The description text in the Download progress toast for showing the number of files (1$) and download progress (2$). This string only consists of two placeholders for purposes of displaying two other strings side-by-side where 1$ is Downloads.Toast.MultipleFiles.DescriptionText and 2$ is Downloads.Toast.Progress.DescriptionText. This string should only consist of the two placeholders side-by-side separated by a single space and 1$ should come before 2$ everywhere except for right-to-left locales.")
}

// Context menu ButtonToast instances.
extension Strings {
    public static let contextMenuButtonToastNewTabOpenedLabelText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedLabelText", bundle: Bundle.shared, value: "New Tab opened", comment: "The label text in the Button Toast for switching to a fresh New Tab.")
    public static let contextMenuButtonToastNewTabOpenedButtonText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedButtonText", bundle: Bundle.shared, value: "Switch", comment: "The button text in the Button Toast for switching to a fresh New Tab.")
}

// Reader Mode.
extension Strings {
    public static let readerModeAvailableVoiceOverAnnouncement = NSLocalizedString("ReaderModeAvailableVoiceOverAnnouncement", bundle: Bundle.shared, value: "Reader Mode available", comment: "Accessibility message e.g. spoken by VoiceOver when Reader Mode becomes available.")
    public static let readerModeResetFontSizeAccessibilityLabel = NSLocalizedString("ReaderModeResetFontSizeAccessibilityLabel", bundle: Bundle.shared, value: "Reset text size", comment: "Accessibility label for button resetting font size in display settings of reader mode")
}

// QR Code scanner.
extension Strings {
    public static let scanQRCodeViewTitle = NSLocalizedString("ScanQRCodeViewTitle", bundle: Bundle.shared, value: "Scan QR Code", comment: "Title for the QR code scanner view.")
    public static let scanQRCodeInstructionsLabel = NSLocalizedString("ScanQRCodeInstructionsLabel", bundle: Bundle.shared, value: "Align QR code within frame to scan", comment: "Text for the instructions label, displayed in the QR scanner view")
    public static let scanQRCodeInvalidDataErrorMessage = NSLocalizedString("ScanQRCodeInvalidDataErrorMessage", bundle: Bundle.shared, value: "The data is invalid", comment: "Text of the prompt that is shown to the user when the data is invalid")
    public static let scanQRCodePermissionErrorMessage = NSLocalizedString("ScanQRCodePermissionErrorMessage", bundle: Bundle.shared, value: "Please allow Brave to access your device’s camera in ‘Settings’ -> ‘Privacy’ -> ‘Camera’.", comment: "Text of the prompt user to setup the camera authorization.")
    public static let scanQRCodeErrorOKButton = NSLocalizedString("ScanQRCodeErrorOKButton", bundle: Bundle.shared, value: "OK", comment: "OK button to dismiss the error prompt.")
}

// App menu.
extension Strings {
    public static let appMenuViewDesktopSiteTitleString = NSLocalizedString("AppMenuViewDesktopSiteTitleString", bundle: Bundle.shared, value: "Request Desktop Site", comment: "Label for the button, displayed in the menu, used to request the desktop version of the current website.")
    public static let appMenuViewMobileSiteTitleString = NSLocalizedString("AppMenuViewMobileSiteTitleString", bundle: Bundle.shared, value: "Request Mobile Site", comment: "Label for the button, displayed in the menu, used to request the mobile version of the current website.")
    public static let appMenuButtonAccessibilityLabel = NSLocalizedString("AppMenuButtonAccessibilityLabel", bundle: Bundle.shared, value: "Menu", comment: "Accessibility label for the Menu button.")
    public static let appMenuCopyURLConfirmMessage = NSLocalizedString("AppMenuCopyURLConfirmMessage", bundle: Bundle.shared, value: "URL Copied To Clipboard", comment: "Toast displayed to user after copy url pressed.")
}

// Snackbar shown when tapping app store link
extension Strings {
    public static let externalLinkAppStoreConfirmationTitle = NSLocalizedString("ExternalLinkAppStoreConfirmationTitle", bundle: Bundle.shared, value: "Open this link in the App Store app?", comment: "Question shown to user when tapping a link that opens the App Store app")
}

// Location bar long press menu
extension Strings {
    public static let pasteAndGoTitle = NSLocalizedString("PasteAndGoTitle", bundle: Bundle.shared, value: "Paste & Go", comment: "The title for the button that lets you paste and go to a URL")
    public static let pasteTitle = NSLocalizedString("PasteTitle", bundle: Bundle.shared, value: "Paste", comment: "The title for the button that lets you paste into the location bar")
    public static let copyAddressTitle = NSLocalizedString("CopyAddressTitle", bundle: Bundle.shared, value: "Copy Address", comment: "The title for the button that lets you copy the url from the location bar.")
}

// Keyboard short cuts
extension Strings {
    public static let showTabTrayFromTabKeyCodeTitle = NSLocalizedString("ShowTabTrayFromTabKeyCodeTitle", bundle: Bundle.shared, value: "Show All Tabs", comment: "Hardware shortcut to open the tab tray from a tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let closeTabFromTabTrayKeyCodeTitle = NSLocalizedString("CloseTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Close Selected Tab", comment: "Hardware shortcut to close the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let closeAllTabsFromTabTrayKeyCodeTitle = NSLocalizedString("CloseAllTabsFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Close All Tabs", comment: "Hardware shortcut to close all tabs from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let openSelectedTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenSelectedTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Open Selected Tab", comment: "Hardware shortcut open the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let openNewTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenNewTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Open New Tab", comment: "Hardware shortcut to open a new tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let switchToPBMKeyCodeTitle = NSLocalizedString("SwitchToPBMKeyCodeTitle", bundle: Bundle.shared, value: "Private Browsing Mode", comment: "Hardware shortcut switch to the private browsing tab or tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let switchToNonPBMKeyCodeTitle = NSLocalizedString("SwitchToNonPBMKeyCodeTitle", bundle: Bundle.shared, value: "Normal Browsing Mode", comment: "Hardware shortcut for non-private tab or tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
}
