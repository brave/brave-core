// swift-format-ignore-file

// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct Sync {
    public static let QRCode = NSLocalizedString(
      "sync.qrCode",
      tableName: "BraveShared",
      bundle: .module,
      value: "QR Code",
      comment: "QR Code section title"
    )
    public static let codeWords = NSLocalizedString(
      "sync.codeWords",
      tableName: "BraveShared",
      bundle: .module,
      value: "Code Words",
      comment: "Code words section title"
    )
    public static let syncTitle = NSLocalizedString(
      "sync.syncTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sync",
      comment: "Sync settings section title"
    )
    public static let settingsHeader = NSLocalizedString(
      "sync.settingsHeader",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "The device list below includes all devices in your sync chain. Each device can be managed from any other device.",
      comment: "Header title for Sync settings"
    )
    public static let thisDevice = NSLocalizedString(
      "sync.thisDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "This Device",
      comment: "This device cell"
    )
    public static let internalsTitle = NSLocalizedString(
      "sync.internalsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sync Internals",
      comment: "Sync-Internals screen title (Sync Internals or Sync Debugging is fine)."
    )
    public static let welcome = NSLocalizedString(
      "sync.welcome",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "To start, you will need Brave installed on all the devices you plan to sync. To chain them together, start a sync chain that you will use to securely link all of your devices together.",
      comment: "Sync settings welcome"
    )
    public static let newSyncCode = NSLocalizedString(
      "sync.newSyncCode",
      tableName: "BraveShared",
      bundle: .module,
      value: "Start a new Sync Chain",
      comment: "New sync code button title"
    )
    public static let scanSyncCode = NSLocalizedString(
      "sync.scanSyncCode",
      tableName: "BraveShared",
      bundle: .module,
      value: "I have a Sync Code",
      comment: "Scan sync code button title"
    )
    public static let scan = NSLocalizedString(
      "sync.scan",
      tableName: "BraveShared",
      bundle: .module,
      value: "Scan",
      comment: "Scan sync code title"
    )
    public static let chooseDevice = NSLocalizedString(
      "sync.chooseDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Choose Device Type",
      comment: "Choose device type for sync"
    )
    public static let addDeviceScan = NSLocalizedString(
      "sync.addDeviceScan",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sync Chain QR Code",
      comment: "Add mobile device to sync with scan"
    )
    public static let addDeviceWords = NSLocalizedString(
      "sync.addDeviceWords",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enter the sync code",
      comment: "Add device to sync with code words"
    )
    public static let addDeviceWordsTitle = NSLocalizedString(
      "sync.addDeviceWordsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enter Code Words",
      comment: "Add device to sync with code words navigation title"
    )
    public static let toDevice = NSLocalizedString(
      "sync.toDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sync to device",
      comment: "Sync to existing device"
    )
    public static let toDeviceDescription = NSLocalizedString(
      "sync.toDeviceDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Using existing synced device open Brave Settings and navigate to Settings -> Sync. Choose \"Add Device\" and scan the code displayed on the screen.",
      comment: "Sync to existing device description"
    )
    public static let addDeviceScanDescription = NSLocalizedString(
      "sync.addDeviceScanDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "On your second mobile device, navigate to Sync in the Settings panel and tap the button \"Scan Sync Code\". Use your camera to scan the QR Code below.\n\nTreat this code like a password. If someone gets hold of it, they can read and modify your synced data.",
      comment: "Sync add device description"
    )
    public static let switchBackToCameraButton = NSLocalizedString(
      "sync.switchBackToCameraButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "I'll use my camera...",
      comment: "Switch back to camera button"
    )
    public static let addDeviceWordsDescription = NSLocalizedString(
      "sync.addDeviceWordsDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "On your device, navigate to Sync in the Settings panel and tap the button \"%@\". Enter the sync chain code words shown below.\n\nTreat this code like a password. If someone gets hold of it, they can read and modify your synced data.",
      comment: "Sync add device description"
    )
    public static let noConnectionTitle = NSLocalizedString(
      "sync.noConnectionTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Can't connect to sync",
      comment: "No internet connection alert title."
    )
    public static let noConnectionBody = NSLocalizedString(
      "sync.noConnectionBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Check your internet connection and try again.",
      comment: "No internet connection alert body."
    )
    public static let enterCodeWords = NSLocalizedString(
      "sync.enterCodeWords",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enter code words",
      comment: "Sync enter code words"
    )
    public static let showCodeWords = NSLocalizedString(
      "sync.showCodeWords",
      tableName: "BraveShared",
      bundle: .module,
      value: "Show code words instead",
      comment: "Show code words instead"
    )
    public static let syncDevices = NSLocalizedString(
      "sync.Syncdevices",
      tableName: "BraveShared",
      bundle: .module,
      value: "Devices & Settings",
      comment: "Sync you browser settings across devices."
    )
    public static let devices = NSLocalizedString(
      "sync.devices",
      tableName: "BraveShared",
      bundle: .module,
      value: "Devices on sync chain",
      comment: "Sync device settings page title."
    )
    public static let codeWordInputHelp = NSLocalizedString(
      "sync.codeWordInputHelp",
      tableName: "BraveShared",
      bundle: .module,
      value: "Type your supplied sync chain code words into the form below.",
      comment: "Code words input help"
    )
    public static let copyToClipboard = NSLocalizedString(
      "sync.copyToClipboard",
      tableName: "BraveShared",
      bundle: .module,
      value: "Copy to Clipboard",
      comment: "Copy codewords title"
    )
    public static let copiedToClipboard = NSLocalizedString(
      "sync.copiedToClipboard",
      tableName: "BraveShared",
      bundle: .module,
      value: "Copied to Clipboard!",
      comment: "Copied codewords title"
    )
    public static let syncUnsuccessful = NSLocalizedString(
      "sync.syncUnsuccessful",
      tableName: "BraveShared",
      bundle: .module,
      value: "Unsuccessful",
      comment: ""
    )
    public static let unableCreateGroup = NSLocalizedString(
      "sync.unableCreateGroup",
      tableName: "BraveShared",
      bundle: .module,
      value: "Can't sync this device",
      comment: "Description on popup when setting up a sync group fails"
    )
    public static let copied = NSLocalizedString(
      "sync.copied",
      tableName: "BraveShared",
      bundle: .module,
      value: "Copied!",
      comment: "Copied action complete title"
    )
    public static let wordCount = NSLocalizedString(
      "sync.wordCount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Word count: %i",
      comment: "Word count title"
    )
    public static let unableToConnectTitle = NSLocalizedString(
      "sync.unableToConnectTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Unable to Connect",
      comment: "Sync Alert"
    )
    public static let unableToConnectDescription = NSLocalizedString(
      "sync.unableToConnectDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Unable to join sync group. Please check the entered words and try again.",
      comment: "Sync Alert"
    )
    public static let enterCodeWordsBelow = NSLocalizedString(
      "sync.enterCodeWordsBelow",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enter code words below",
      comment: "Enter sync code words below"
    )
    public static let removeThisDevice = NSLocalizedString(
      "sync.removeThisDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove this device",
      comment: "Sync remove device."
    )
    public static let removeDeviceAction = NSLocalizedString(
      "sync.removeDeviceAction",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove device",
      comment: "Remove device button for action sheet."
    )
    public static let syncRemoveThisDeviceQuestion = NSLocalizedString(
      "sync.syncRemoveThisDeviceQuestion",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove this device?",
      comment: "Sync remove device?"
    )
    public static let chooseDeviceHeader = NSLocalizedString(
      "sync.chooseDeviceHeader",
      tableName: "BraveShared",
      bundle: .module,
      value: "Choose the type of device you would like to sync to.",
      comment: "Header for device choosing screen."
    )
    public static let removeThisDeviceQuestionDesc = NSLocalizedString(
      "sync.removeThisDeviceQuestionDesc",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "This device will be disconnected from sync group and no longer receive or send sync data. All existing data will remain on device.",
      comment: "Sync remove device?"
    )
    public static let pair = NSLocalizedString(
      "sync.pair",
      tableName: "BraveShared",
      bundle: .module,
      value: "Pair",
      comment: "Sync pair device settings section title"
    )
    public static let addAnotherDevice = NSLocalizedString(
      "sync.addAnotherDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add New Device",
      comment: "Add New Device cell button."
    )
    public static let deleteAccount = NSLocalizedString(
      "sync.deleteAccount",
      tableName: "BraveShared",
      bundle: .module,
      value: "Delete Sync Account",
      comment: "Delete Sync Account cell title for button."
    )
    public static let tabletOrMobileDevice = NSLocalizedString(
      "sync.tabletOrMobileDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Tablet or Phone",
      comment: "Tablet or phone button title"
    )
    public static let addTabletOrPhoneTitle = NSLocalizedString(
      "sync.addTabletOrPhoneTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add a Tablet or Phone",
      comment: "Add Tablet or phone title"
    )
    public static let computerDevice = NSLocalizedString(
      "sync.computerDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Computer",
      comment: "Computer device button title"
    )
    public static let addComputerTitle = NSLocalizedString(
      "sync.addComputerTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Add a Computer",
      comment: "Add a Computer title"
    )
    public static let grantCameraAccess = NSLocalizedString(
      "sync.grantCameraAccess",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enable Camera",
      comment: "Grand camera access"
    )
    public static let removeDevice = NSLocalizedString(
      "sync.removeDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove",
      comment: "Action button for removing sync device."
    )
    public static let initErrorTitle = NSLocalizedString(
      "sync.initErrorTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sync Communication Error",
      comment: "Title for sync initialization error alert"
    )
    public static let initErrorMessage = NSLocalizedString(
      "sync.initErrorMessage",
      tableName: "BraveShared",
      bundle: .module,
      value: "The Sync Agent is currently offline or not reachable. Please try again later.",
      comment: "Message for sync initialization error alert"
    )
    // Remove device popups
    public static let removeLastDeviceTitle = NSLocalizedString(
      "sync.removeLastDeviceTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Removing %@ will delete the Sync Chain.",
      comment: "Title for removing last device from Sync"
    )
    public static let removeLastDeviceMessage = NSLocalizedString(
      "sync.removeLastDeviceMessage",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Data currently synced will be retained but all data in Brave’s Sync cache will be deleted. You will need to start a new sync chain to sync device data again.",
      comment: "Message for removing last device from Sync"
    )
    public static let removeLastDeviceRemoveButtonName = NSLocalizedString(
      "sync.removeLastDeviceRemoveButtonName",
      tableName: "BraveShared",
      bundle: .module,
      value: "Delete Sync Chain",
      comment: "Button name for removing last device from Sync"
    )
    public static let removeCurrentDeviceTitle = NSLocalizedString(
      "sync.removeCurrentDeviceTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove %@ from Sync Chain?",
      comment: "Title for removing the current device from Sync"
    )
    public static let removeCurrentDeviceMessage = NSLocalizedString(
      "sync.removeCurrentDeviceMessage",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Local device data will remain intact on all devices. Other devices in this Sync Chain will remain synced. ",
      comment: "Message for removing the current device from Sync"
    )
    public static let removeOtherDeviceTitle = NSLocalizedString(
      "sync.removeOtherDeviceTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Remove %@ from Sync Chain?",
      comment: "Title for removing other device from Sync"
    )
    public static let removeOtherDeviceMessage = NSLocalizedString(
      "sync.removeOtherDeviceMessage",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Removing the device from the Sync Chain will not clear previously synced data from the device.",
      comment: "Message for removing other device from Sync"
    )
    public static let removeDeviceDefaultName = NSLocalizedString(
      "sync.removeDeviceDefaultName",
      tableName: "BraveShared",
      bundle: .module,
      value: "Device",
      comment: "Default name for a device"
    )
    public static let validForTooLongError = NSLocalizedString(
      "sync.validForTooLongError",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "This code is invalid. Please check that the time and timezone are set correctly on your device.",
      comment: "Sync Error Description"
    )
    public static let deprecatedVersionError = NSLocalizedString(
      "sync.deprecatedVersionError",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "It looks like this code was created on a device running an older version of Brave. Please update to the latest version of Brave on your other device, and then create a new sync code.",
      comment: "Sync Error Description"
    )
    public static let expiredError = NSLocalizedString(
      "sync.expiredError",
      tableName: "BraveShared",
      bundle: .module,
      value: "This sync code has expired, please generate a new sync code and try again.",
      comment: "Sync Error message for when the sync code is expired"
    )
    public static let genericError = NSLocalizedString(
      "sync.genericError",
      tableName: "BraveShared",
      bundle: .module,
      value: "Sorry, something went wrong",
      comment: "Generic Sync Error Description"
    )
    public static let notEnoughWordsTitle = NSLocalizedString(
      "sync.notEnoughWordsTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Not Enough Words",
      comment: "Error title for when the sync code does not have the correct amount of words"
    )
    public static let notEnoughWordsDescription = NSLocalizedString(
      "sync.notEnoughWordsDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Please enter all of the words and try again.",
      comment: "Error message for when the sync code does not have the correct amount of words"
    )
    public static let invalidSyncCodeDescription = NSLocalizedString(
      "sync.invalidSyncCodeDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Invalid sync code, please check and try again.",
      comment: "Generic error message for when the sync code is invalid"
    )
    public static let codeExpirationTitleLabel = NSLocalizedString(
      "sync.codeExpirationTitleLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "Code expired. Generate a new one by tapping the button below.",
      comment:
        "Error title explaining the sync code is expired and it must be genrated all over again."
    )
    public static let codeTimeRemainingTitleLabel = NSLocalizedString(
      "sync.codeTimeRemainingTitleLabel",
      tableName: "BraveShared",
      bundle: .module,
      value: "This temporary code is valid for the next %@",
      comment:
        "Title explaining how loing the sync code can be used. It will be used like Ex: This temporary code is valid for the next 17 hours 33 minutes 1 second"
    )
    public static let codeTimeMultipleHourText = NSLocalizedString(
      "sync.codeTimeMultipleHourText",
      tableName: "BraveShared",
      bundle: .module,
      value: "hours",
      comment:
        "The unit text used for multiple hours when showing code validity. Ex: This temporary code is valid for the next 2 hours"
    )
    public static let codeTimeSingleHourText = NSLocalizedString(
      "sync.codeTimeSingleHourText",
      tableName: "BraveShared",
      bundle: .module,
      value: "hour",
      comment:
        "The unit text used for single hour when showing code validity. Ex: This temporary code is valid for the next 1 hour"
    )
    public static let codeTimeMinutesAbbreviation = NSLocalizedString(
      "sync.codeTimeMinutesAbbreviation",
      tableName: "BraveShared",
      bundle: .module,
      value: "min",
      comment:
        "The unit abbreviation used for minutes when showing code validity. Ex: This temporary code is valid for the next 2 min"
    )
    public static let codeTimeMultipleSecondsText = NSLocalizedString(
      "sync.codeTimeMultipleSecondsText",
      tableName: "BraveShared",
      bundle: .module,
      value: "seconds",
      comment:
        "The unit text used for multiple seconds when showing code validity. Ex: This temporary code is valid for the next 2 seconds"
    )
    public static let codeTimeSingleSecondText = NSLocalizedString(
      "sync.codeTimeSingleSecondText",
      tableName: "BraveShared",
      bundle: .module,
      value: "second",
      comment:
        "The unit text used for single second when showing code validity. Ex: This temporary code is valid for the next 1 second"
    )
    public static let bookmarksImportExportPopupTitle =
      NSLocalizedString(
        "sync.bookmarksImportPopupErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Bookmarks",
        comment: "Title of the bookmark import popup."
      )
    public static let bookmarksImportPopupSuccessMessage =
      NSLocalizedString(
        "sync.bookmarksImportPopupSuccessMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Bookmarks Imported Successfully",
        comment: "Message of the popup if bookmark import succeeds."
      )
    public static let bookmarksExportPopupSuccessMessage =
      NSLocalizedString(
        "sync.bookmarksExportPopupSuccessMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Bookmarks Exported Successfully",
        comment: "Message of the popup if bookmark export succeeds."
      )
    public static let bookmarksExportPopupFailureMessage =
      NSLocalizedString(
        "sync.bookmarksIExportPopupFailureMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Bookmark Export Failed",
        comment: "Message of the popup if bookmark export fails."
      )
    public static let bookmarksImportPopupFailureMessage =
      NSLocalizedString(
        "sync.bookmarksImportPopupFailureMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Bookmark Import Failed",
        comment: "Message of the popup if bookmark import fails."
      )
    /// Important: Do NOT change the `KEY` parameter without updating it in
    /// BraveCore's brave_bookmarks_importer.mm file.
    public static let importFolderName =
      NSLocalizedString(
        "SyncImportFolderName",
        tableName: "BraveShared",
        bundle: .module,
        value: "Imported Bookmarks",
        comment:
          "Folder name for where bookmarks are imported into when the root folder is not empty."
      )
    public static let configurationInformationText =
      NSLocalizedString(
        "sync.configurationInformationText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Manage what information you would like to sync between devices. These settings only affect this device.",
        comment:
          "Information Text underneath the toggles for enable/disable different sync types for the device"
      )
    public static let settingsTitle =
      NSLocalizedString(
        "sync.settingsTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Settings",
        comment: "Title for Sync Settings Toggle Header"
      )
    public static let deleteAccountAlertTitle =
      NSLocalizedString(
        "sync.deleteAccount",
        tableName: "BraveShared",
        bundle: .module,
        value: "Delete Sync Account",
        comment: "Title for Alert used action Delete Sync Account."
      )
    public static let deleteAccountAlertDescriptionPart1 =
      NSLocalizedString(
        "sync.deleteAccountAlertDescriptionPart1",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Deleting your account will remove your encrypted data from Brave servers and disable Sync on all of your connected devices.",
        comment: "Part 1 Description for Alert used action Delete Sync Account."
      )
    public static let deleteAccountAlertDescriptionPart2 =
      NSLocalizedString(
        "sync.deleteAccountAlertDescriptionPart2",
        tableName: "BraveShared",
        bundle: .module,
        value: "It will not however delete the data that is stored locally on those devices.",
        comment: "Part 2 Description for Alert used action Delete Sync Account."
      )
    public static let deleteAccountAlertDescriptionPart3 =
      NSLocalizedString(
        "sync.syncDeleteAccountAlertDescriptionPart3",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This deletion is permanent and there is no way to recover the data. Should you decide to start using Sync again, you will need to create a new account and re-add each device one by one.",
        comment: "Part 3 Description for Alert used action Delete Sync Account."
      )
    public static let chainAlreadyDeletedAlertTitle =
      NSLocalizedString(
        "sync.chainAlreadyDeletedAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Device",
        comment:
          "Alert title for the occasion when an user is trying to join a already deleted account"
      )
    public static let chainAlreadyDeletedAlertDescription =
      NSLocalizedString(
        "sync.chainAlreadyDeletedAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Could not join this chain. Account was deleted.",
        comment:
          "Alert description for the occasion when an user is trying to join a already deleted account"
      )
    public static let chainAccountDeletionErrorTitle =
      NSLocalizedString(
        "sync.chainAccountDeletionErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Device",
        comment: "Alert title for error while deleting sync chain"
      )
    public static let chainAccountDeletionErrorDescription =
      NSLocalizedString(
        "sync.chainAccountDeletionErrorDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Sync Device",
        comment: "Alert description for error while deleting sync chain"
      )
    public static let setPasscodeAlertTitle =
      NSLocalizedString(
        "sync.setPasscodeAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Set a Passcode",
        comment: "The title displayed in alert when a user needs to set passcode"
      )
    public static let setPasscodeAlertDescription =
      NSLocalizedString(
        "sync.setPasscodeAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "To add a device to sync chain or toggle password sync, you must first set a passcode on your device.",
        comment: "The message displayed in alert when a user needs to set a passcode"
      )
    public static let joinChainCodewordsWarning =
      NSLocalizedString(
        "sync.joinChainCodewordsWarning",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Note: You should verify you recognize each device in the list above. Devices in a sync chain may receive personal data like passwords and browsing history.",
        comment: "A warning when user adds more than 5 device to sync chain"
      )
    public static let deviceFetchErrorAlertDescription =
      NSLocalizedString(
        "sync.deviceFetchErrorAlertDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Something went wrong while retrieving devices in sync chain.",
        comment: "The message displayed in alert when a there is a problem with fetching devices"
      )
    public static let devicesInSyncChainTitle =
      NSLocalizedString(
        "sync.devicesInSyncChainTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Devices in Sync Chain",
        comment: "The message displayed in alert when a list of devices will be shown"
      )
    public static let maximumDeviceReachedErrorTitle =
      NSLocalizedString(
        "sync.maximumDeviceReachedErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Device limit",
        comment: "The warning displayed in alert when the maxmium of devices is reached"
      )
    public static let maximumDeviceReachedErrorDescription =
      NSLocalizedString(
        "sync.maximumDeviceReachedErrorDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "You've reached the maximum number of devices (10) allowed in a sync chain. Remove a device to continue.",
        comment: "The message displayed in alert when the maxmium of devices is reached"
      )
    public static let joinChainWarningTitle =
      NSLocalizedString(
        "sync.joinChainWarningTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Device Confirmation",
        comment: "Title for alert error for device confirmation"
      )
  }
}
