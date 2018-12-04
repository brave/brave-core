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
    public static let ThirdPartySearchFailedTitle = NSLocalizedString("ThirdPartySearchFailedTitle", bundle: Bundle.shared, value: "Failed", comment: "A title explaining that we failed to add a search engine")
}

// Settings.
extension Strings {
    public static let SettingsGeneralSectionTitle = NSLocalizedString("SettingsGeneralSectionTitle", bundle: Bundle.shared, value: "General", comment: "General settings section title")
    public static let SettingsSearchDoneButton = NSLocalizedString("SettingsSearchDoneButton", bundle: Bundle.shared, value: "Done", comment: "Button displayed at the top of the search settings.")
    public static let SettingsSearchEditButton = NSLocalizedString("SettingsSearchEditButton", bundle: Bundle.shared, value: "Edit", comment: "Button displayed at the top of the search settings.")
    public static let UseTouchID = NSLocalizedString("UseTouchID", bundle: Bundle.shared, value: "Use Touch ID", comment: "List section title for when to use Touch ID")
    public static let UseFaceID = NSLocalizedString("UseFaceID", bundle: Bundle.shared, value: "Use Face ID", comment: "List section title for when to use Face ID")
}

// Logins Helper.
extension Strings {
    public static let LoginsHelperSaveLoginButtonTitle = NSLocalizedString("LoginsHelperSaveLoginButtonTitle", bundle: Bundle.shared, value: "Save Login", comment: "Button to save the user's password")
    public static let LoginsHelperDontSaveButtonTitle = NSLocalizedString("LoginsHelperDontSaveButtonTitle", bundle: Bundle.shared, value: "Don’t Save", comment: "Button to not save the user's password")
    public static let LoginsHelperUpdateButtonTitle = NSLocalizedString("LoginsHelperUpdateButtonTitle", bundle: Bundle.shared, value: "Update", comment: "Button to update the user's password")
    public static let LoginsHelperDontUpdateButtonTitle = NSLocalizedString("LoginsHelperDontUpdateButtonTitle", bundle: Bundle.shared, value: "Don’t Update", comment: "Button to not update the user's password")
}

// Brave Logins
extension Strings {
    public static let SaveLoginUsernamePrompt = NSLocalizedString("SaveLoginUsernamePrompt", bundle: Bundle.shared, value: "Save login %@ for %@?", comment: "Prompt for saving a login. The first parameter is the username being saved. The second parameter is the hostname of the site.")
    public static let SaveLoginPrompt = NSLocalizedString("SaveLoginPrompt", bundle: Bundle.shared, value: "Save password for %@?", comment: "Prompt for saving a password with no username. The parameter is the hostname of the site.")
    public static let UpdateLoginUsernamePrompt = NSLocalizedString("UpdateLoginUsernamePrompt", bundle: Bundle.shared, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
    public static let UpdateLoginPrompt = NSLocalizedString("UpdateLoginPrompt", bundle: Bundle.shared, value: "Update login %@ for %@?", comment: "Prompt for updating a login. The first parameter is the username for which the password will be updated for. The second parameter is the hostname of the site.")
}

//Hotkey Titles
extension Strings {
    public static let ReloadPageTitle = NSLocalizedString("ReloadPageTitle", bundle: Bundle.shared, value: "Reload Page", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let BackTitle = NSLocalizedString("BackTitle", bundle: Bundle.shared, value: "Back", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let ForwardTitle = NSLocalizedString("ForwardTitle", bundle: Bundle.shared, value: "Forward", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")

    public static let FindTitle = NSLocalizedString("FindTitle", bundle: Bundle.shared, value: "Find", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let SelectLocationBarTitle = NSLocalizedString("SelectLocationBarTitle", bundle: Bundle.shared, value: "Select Location Bar", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let NewTabTitle = NSLocalizedString("NewTabTitle", bundle: Bundle.shared, value: "New Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let NewPrivateTabTitle = NSLocalizedString("NewPrivateTabTitle", bundle: Bundle.shared, value: "New Private Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let CloseTabTitle = NSLocalizedString("CloseTabTitle", bundle: Bundle.shared, value: "Close Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let ShowNextTabTitle = NSLocalizedString("ShowNextTabTitle", bundle: Bundle.shared, value: "Show Next Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
    public static let ShowPreviousTabTitle = NSLocalizedString("ShowPreviousTabTitle", bundle: Bundle.shared, value: "Show Previous Tab", comment: "Label to display in the Discoverability overlay for keyboard shortcuts")
}

// Home page.
extension Strings {
    public static let SetHomePageDialogTitle = NSLocalizedString("SetHomePageDialogTitle", bundle: Bundle.shared, value: "Do you want to use this web page as your home page?", comment: "Alert dialog title when the user opens the home page for the first time.")
    public static let SetHomePageDialogMessage = NSLocalizedString("SetHomePageDialogMessage", bundle: Bundle.shared, value: "You can change this at any time in Settings", comment: "Alert dialog body when the user opens the home page for the first time.")
    public static let SetHomePageDialogYes = NSLocalizedString("SetHomePageDialogYes", bundle: Bundle.shared, value: "Set Homepage", comment: "Button accepting changes setting the home page for the first time.")
    public static let SetHomePageDialogNo = NSLocalizedString("SetHomePageDialogNo", bundle: Bundle.shared, value: "Cancel", comment: "Button cancelling changes setting the home page for the first time.")
}

// New tab choice settings
extension Strings {
    public static let SettingsNewTabTopSites = NSLocalizedString("SettingsNewTabTopSites", bundle: Bundle.shared, value: "Show your Top Sites", comment: "Option in settings to show top sites when you open a new tab")
    public static let SettingsNewTabBlankPage = NSLocalizedString("SettingsNewTabBlankPage", bundle: Bundle.shared, value: "Show a Blank Page", comment: "Option in settings to show a blank page when you open a new tab")
    public static let SettingsNewTabHomePage = NSLocalizedString("SettingsNewTabHomePage", bundle: Bundle.shared, value: "Show your Homepage", comment: "Option in settings to show your homepage when you open a new tab")
}

// Custom account settings - These strings did not make it for the v10 l10n deadline so we have turned them into regular strings. These strings will come back localized in a next version.

extension Strings {
    // Settings.AdvanceAccount.SectionName
    // Label used as an item in Settings. When touched it will open a dialog to setup advance Brave account settings.
    public static let SettingsAdvanceAccountSectionName = "Account Settings"

    // Settings.AdvanceAccount.SectionFooter
    // Details for using custom Brave Account service.
    public static let SettingsAdvanceAccountSectionFooter = "To use a custom Brave Account and sync servers, specify the root Url of the Brave Account site. This will download the configuration and setup this device to use the new service. After the new service has been set, you will need to create a new Brave Account or login with an existing one."

    // Settings.AdvanceAccount.SectionName
    // Title displayed in header of the setting panel.
    public static let SettingsAdvanceAccountTitle = "Advance Account Settings"

    // Settings.AdvanceAccount.UrlPlaceholder
    // Title displayed in header of the setting panel.
    public static let SettingsAdvanceAccountUrlPlaceholder = "Custom Account Url"

    // Settings.AdvanceAccount.UpdatedAlertMessage
    // Messaged displayed when sync service has been successfully set.
    public static let SettingsAdvanceAccountUrlUpdatedAlertMessage = "Brave account service updated. To begin using custom server, please log out and re-login."

    // Settings.AdvanceAccount.UpdatedAlertOk
    // Ok button on custom sync service updated alert
    public static let SettingsAdvanceAccountUrlUpdatedAlertOk = "OK"

    // Settings.AdvanceAccount.ErrorAlertTitle
    // Error alert message title.
    public static let SettingsAdvanceAccountUrlErrorAlertTitle = "Error"

    // Settings.AdvanceAccount.ErrorAlertMessage
    // Messaged displayed when sync service has an error setting a custom sync url.
    public static let SettingsAdvanceAccountUrlErrorAlertMessage = "There was an error while attempting to parse the url. Please make sure that it is a valid Brave Account root url."

    // Settings.AdvanceAccount.ErrorAlertOk
    // Ok button on custom sync service error alert.
    public static let SettingsAdvanceAccountUrlErrorAlertOk = "OK"

    // Settings.AdvanceAccount.UseCustomAccountsServiceTitle
    // Toggle switch to use custom FxA server
    public static let SettingsAdvanceAccountUseCustomAccountsServiceTitle = "Use Custom Account Service"

    // Settings.AdvanceAccount.UrlEmptyErrorAlertMessage
    // No custom service set.
    public static let SettingsAdvanceAccountEmptyUrlErrorAlertMessage = "Please enter a custom account url before enabling."
}

// Third Party Search Engines
extension Strings {
    public static let ThirdPartySearchEngineAdded = NSLocalizedString("ThirdPartySearchEngineAdded", bundle: Bundle.shared, value: "Added Search engine!", comment: "The success message that appears after a user sucessfully adds a new search engine")
    public static let ThirdPartySearchAddTitle = NSLocalizedString("ThirdPartySearchAddTitle", bundle: Bundle.shared, value: "Add Search Provider?", comment: "The title that asks the user to Add the search provider")
    public static let ThirdPartySearchAddMessage = NSLocalizedString("ThirdPartySearchAddMessage", bundle: Bundle.shared, value: "The new search engine will appear in the quick search bar.", comment: "The message that asks the user to Add the search provider explaining where the search engine will appear")
    public static let ThirdPartySearchCancelButton = NSLocalizedString("ThirdPartySearchCancelButton", bundle: Bundle.shared, value: "Cancel", comment: "The cancel button if you do not want to add a search engine.")
    public static let ThirdPartySearchOkayButton = NSLocalizedString("ThirdPartySearchOkayButton", bundle: Bundle.shared, value: "OK", comment: "The confirmation button")
    public static let ThirdPartySearchFailedMessage = NSLocalizedString("ThirdPartySearchFailedMessage", bundle: Bundle.shared, value: "The search provider could not be added.", comment: "A title explaining that we failed to add a search engine")
    public static let CustomEngineFormErrorMessage = NSLocalizedString("CustomEngineFormErrorMessage", bundle: Bundle.shared, value: "Please fill all fields correctly.", comment: "A message explaining fault in custom search engine form.")
    public static let CustomEngineDuplicateErrorMessage = NSLocalizedString("CustomEngineDuplicateErrorMessage", bundle: Bundle.shared, value: "A search engine with this title or URL has already been added.", comment: "A message explaining fault in custom search engine form.")
}

// Tabs Delete All Undo Toast
extension Strings {
    public static let TabsDeleteAllUndoTitle = NSLocalizedString("TabsDeleteAllUndoTitle", bundle: Bundle.shared, value: "%d tab(s) closed", comment: "The label indicating that all the tabs were closed")
    public static let TabsDeleteAllUndoAction = NSLocalizedString("TabsDeleteAllUndoAction", bundle: Bundle.shared, value: "Undo", comment: "The button to undo the delete all tabs")
}

//Clipboard Toast
extension Strings {
    public static let GoToCopiedLink = NSLocalizedString("GoToCopiedLink", bundle: Bundle.shared, value: "Go to copied link?", comment: "Message displayed when the user has a copied link on the clipboard")
    public static let GoButtonTittle = NSLocalizedString("GoButtonTittle", bundle: Bundle.shared, value: "Go", comment: "The button to open a new tab with the copied link")
}

// errors
extension Strings {
    public static let UnableToAddPassErrorTitle = NSLocalizedString("UnableToAddPassErrorTitle", bundle: Bundle.shared, value: "Failed to Add Pass", comment: "Title of the 'Add Pass Failed' alert. See https://support.apple.com/HT204003 for context on Wallet.")
    public static let UnableToAddPassErrorMessage = NSLocalizedString("UnableToAddPassErrorMessage", bundle: Bundle.shared, value: "An error occurred while adding the pass to Wallet. Please try again later.", comment: "Text of the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
    public static let UnableToAddPassErrorDismiss = NSLocalizedString("UnableToAddPassErrorDismiss", bundle: Bundle.shared, value: "OK", comment: "Button to dismiss the 'Add Pass Failed' alert.  See https://support.apple.com/HT204003 for context on Wallet.")
    public static let UnableToOpenURLError = NSLocalizedString("UnableToOpenURLError", bundle: Bundle.shared, value: "Brave cannot open the page because it has an invalid address.", comment: "The message displayed to a user when they try to open a URL that cannot be handled by Brave, or any external app.")
    public static let UnableToOpenURLErrorTitle = NSLocalizedString("UnableToOpenURLErrorTitle", bundle: Bundle.shared, value: "Cannot Open Page", comment: "Title of the message shown when the user attempts to navigate to an invalid link.")
}

// Download Helper
extension Strings {
    public static let DownloadsButtonTitle = NSLocalizedString("DownloadsButtonTitle", bundle: Bundle.shared, value: "Downloads", comment: "The button to open a new tab with the Downloads home panel")
    public static let CancelDownloadDialogTitle = NSLocalizedString("CancelDownloadDialogTitle", bundle: Bundle.shared, value: "Cancel Download", comment: "Alert dialog title when the user taps the cancel download icon.")
    public static let CancelDownloadDialogMessage = NSLocalizedString("CancelDownloadDialogMessage", bundle: Bundle.shared, value: "Are you sure you want to cancel this download?", comment: "Alert dialog body when the user taps the cancel download icon.")
    public static let CancelDownloadDialogResume = NSLocalizedString("CancelDownloadDialogResume", bundle: Bundle.shared, value: "Resume", comment: "Button declining the cancellation of the download.")
    public static let CancelDownloadDialogCancel = NSLocalizedString("CancelDownloadDialogCancel", bundle: Bundle.shared, value: "Cancel", comment: "Button confirming the cancellation of the download.")
    public static let DownloadCancelledToastLabelText = NSLocalizedString("DownloadCancelledToastLabelText", bundle: Bundle.shared, value: "Download Cancelled", comment: "The label text in the Download Cancelled toast for showing confirmation that the download was cancelled.")
    public static let DownloadFailedToastLabelText = NSLocalizedString("DownloadFailedToastLabelText", bundle: Bundle.shared, value: "Download Failed", comment: "The label text in the Download Failed toast for showing confirmation that the download has failed.")
    public static let DownloadMultipleFilesToastDescriptionText = NSLocalizedString("DownloadMultipleFilesToastDescriptionText", bundle: Bundle.shared, value: "1 of %d files", comment: "The description text in the Download progress toast for showing the number of files when multiple files are downloading.")
    public static let DownloadProgressToastDescriptionText = NSLocalizedString("DownloadProgressToastDescriptionText", bundle: Bundle.shared, value: "%1$@/%2$@", comment: "The description text in the Download progress toast for showing the downloaded file size (1$) out of the total expected file size (2$).")
    public static let DownloadMultipleFilesAndProgressToastDescriptionText = NSLocalizedString("DownloadMultipleFilesAndProgressToastDescriptionText", bundle: Bundle.shared, value: "%1$@ %2$@", comment: "The description text in the Download progress toast for showing the number of files (1$) and download progress (2$). This string only consists of two placeholders for purposes of displaying two other strings side-by-side where 1$ is Downloads.Toast.MultipleFiles.DescriptionText and 2$ is Downloads.Toast.Progress.DescriptionText. This string should only consist of the two placeholders side-by-side separated by a single space and 1$ should come before 2$ everywhere except for right-to-left locales.")
}

// Context menu ButtonToast instances.
extension Strings {
    public static let ContextMenuButtonToastNewTabOpenedLabelText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedLabelText", bundle: Bundle.shared, value: "New Tab opened", comment: "The label text in the Button Toast for switching to a fresh New Tab.")
    public static let ContextMenuButtonToastNewTabOpenedButtonText = NSLocalizedString("ContextMenuButtonToastNewTabOpenedButtonText", bundle: Bundle.shared, value: "Switch", comment: "The button text in the Button Toast for switching to a fresh New Tab.")
}

// Reader Mode.
extension Strings {
    public static let ReaderModeAvailableVoiceOverAnnouncement = NSLocalizedString("ReaderModeAvailableVoiceOverAnnouncement", bundle: Bundle.shared, value: "Reader Mode available", comment: "Accessibility message e.g. spoken by VoiceOver when Reader Mode becomes available.")
    public static let ReaderModeResetFontSizeAccessibilityLabel = NSLocalizedString("ReaderModeResetFontSizeAccessibilityLabel", bundle: Bundle.shared, value: "Reset text size", comment: "Accessibility label for button resetting font size in display settings of reader mode")
}

// QR Code scanner.
extension Strings {
    public static let ScanQRCodeViewTitle = NSLocalizedString("ScanQRCodeViewTitle", bundle: Bundle.shared, value: "Scan QR Code", comment: "Title for the QR code scanner view.")
    public static let ScanQRCodeInstructionsLabel = NSLocalizedString("ScanQRCodeInstructionsLabel", bundle: Bundle.shared, value: "Align QR code within frame to scan", comment: "Text for the instructions label, displayed in the QR scanner view")
    public static let ScanQRCodeInvalidDataErrorMessage = NSLocalizedString("ScanQRCodeInvalidDataErrorMessage", bundle: Bundle.shared, value: "The data is invalid", comment: "Text of the prompt that is shown to the user when the data is invalid")
    public static let ScanQRCodePermissionErrorMessage = NSLocalizedString("ScanQRCodePermissionErrorMessage", bundle: Bundle.shared, value: "Please allow Brave to access your device’s camera in ‘Settings’ -> ‘Privacy’ -> ‘Camera’.", comment: "Text of the prompt user to setup the camera authorization.")
    public static let ScanQRCodeErrorOKButton = NSLocalizedString("ScanQRCodeErrorOKButton", bundle: Bundle.shared, value: "OK", comment: "OK button to dismiss the error prompt.")
}

// App menu.
extension Strings {
    public static let AppMenuViewDesktopSiteTitleString = NSLocalizedString("AppMenuViewDesktopSiteTitleString", bundle: Bundle.shared, value: "Request Desktop Site", comment: "Label for the button, displayed in the menu, used to request the desktop version of the current website.")
    public static let AppMenuViewMobileSiteTitleString = NSLocalizedString("AppMenuViewMobileSiteTitleString", bundle: Bundle.shared, value: "Request Mobile Site", comment: "Label for the button, displayed in the menu, used to request the mobile version of the current website.")
    public static let AppMenuButtonAccessibilityLabel = NSLocalizedString("AppMenuButtonAccessibilityLabel", bundle: Bundle.shared, value: "Menu", comment: "Accessibility label for the Menu button.")
    public static let AppMenuCopyURLConfirmMessage = NSLocalizedString("AppMenuCopyURLConfirmMessage", bundle: Bundle.shared, value: "URL Copied To Clipboard", comment: "Toast displayed to user after copy url pressed.")
}

// Snackbar shown when tapping app store link
extension Strings {
    public static let ExternalLinkAppStoreConfirmationTitle = NSLocalizedString("ExternalLinkAppStoreConfirmationTitle", bundle: Bundle.shared, value: "Open this link in the App Store app?", comment: "Question shown to user when tapping a link that opens the App Store app")
}

// Location bar long press menu
extension Strings {
    public static let PasteAndGoTitle = NSLocalizedString("PasteAndGoTitle", bundle: Bundle.shared, value: "Paste & Go", comment: "The title for the button that lets you paste and go to a URL")
    public static let PasteTitle = NSLocalizedString("PasteTitle", bundle: Bundle.shared, value: "Paste", comment: "The title for the button that lets you paste into the location bar")
    public static let CopyAddressTitle = NSLocalizedString("CopyAddressTitle", bundle: Bundle.shared, value: "Copy Address", comment: "The title for the button that lets you copy the url from the location bar.")
}

// Keyboard short cuts
extension Strings {
    public static let ShowTabTrayFromTabKeyCodeTitle = NSLocalizedString("ShowTabTrayFromTabKeyCodeTitle", bundle: Bundle.shared, value: "Show All Tabs", comment: "Hardware shortcut to open the tab tray from a tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let CloseTabFromTabTrayKeyCodeTitle = NSLocalizedString("CloseTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Close Selected Tab", comment: "Hardware shortcut to close the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let CloseAllTabsFromTabTrayKeyCodeTitle = NSLocalizedString("CloseAllTabsFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Close All Tabs", comment: "Hardware shortcut to close all tabs from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let OpenSelectedTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenSelectedTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Open Selected Tab", comment: "Hardware shortcut open the selected tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let OpenNewTabFromTabTrayKeyCodeTitle = NSLocalizedString("OpenNewTabFromTabTrayKeyCodeTitle", bundle: Bundle.shared, value: "Open New Tab", comment: "Hardware shortcut to open a new tab from the tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let SwitchToPBMKeyCodeTitle = NSLocalizedString("SwitchToPBMKeyCodeTitle", bundle: Bundle.shared, value: "Private Browsing Mode", comment: "Hardware shortcut switch to the private browsing tab or tab tray. Shown in the Discoverability overlay when the hardware Command Key is held down.")
    public static let SwitchToNonPBMKeyCodeTitle = NSLocalizedString("SwitchToNonPBMKeyCodeTitle", bundle: Bundle.shared, value: "Normal Browsing Mode", comment: "Hardware shortcut for non-private tab or tab. Shown in the Discoverability overlay when the hardware Command Key is held down.")
}
