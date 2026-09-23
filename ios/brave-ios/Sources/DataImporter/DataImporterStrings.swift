// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct DataImporter {
    /// Important: Do NOT change the `KEY` parameter without updating it in
    /// BraveCore's brave_bookmarks_importer.mm file.
    public static let importFolderName =
      NSLocalizedString(
        "SyncImportFolderName",
        bundle: .module,
        value: "Imported Bookmarks",
        comment:
          "Folder name for where bookmarks are imported into when the root folder is not empty."
      )

    // MARK: - Data Import Internals

    public static let personalImportTitle = NSLocalizedString(
      "personalImportTitle",
      bundle: .module,
      value: "Personal",
      comment:
        "The name of the folder or profile containing Personal data that should be imported. IE: You can have work profiles, personal profiles, etc."
    )

    // MARK: - Data Import Loading Screen

    public static let loadingTitle = NSLocalizedString(
      "loadingTitle",
      bundle: .module,
      value: "Importing Your Data...",
      comment:
        "The title shown on the import data screen when the data is being loaded or imported."
    )

    public static let loadingMessage = NSLocalizedString(
      "loadingMessage",
      bundle: .module,
      value: "Please wait while we securely transfer your data.\nThis might take a few moments.",
      comment:
        "The message shown on the import data screen when the data is being loaded or imported."
    )

    // MARK: - Multi-Profile screen

    public static let multipleProfilesTitle = NSLocalizedString(
      "multipleProfilesTitle",
      bundle: .module,
      value: "Multiple profiles detected",
      comment:
        "The title shown on the import data screen when there are multiple browser profiles such as Personal, Work, School, etc."
    )

    public static let multipleProfilesMessage = NSLocalizedString(
      "multipleProfilesMessage",
      bundle: .module,
      value: "Choose the profile you want to import data from",
      comment:
        "The message shown on the import data screen when there are multiple profiles such as Personal, Work, School, etc."
    )

    // MARK: - Import State View

    public static let importStateFailureTitle = NSLocalizedString(
      "importStateFailureTitle",
      bundle: .module,
      value: "Import Failed",
      comment:
        "The title shown on the import data screen when importing the data from the browser profile failed."
    )

    public static let importStateFailureMessage = NSLocalizedString(
      "importStateFailureMessage",
      bundle: .module,
      value:
        "Something went wrong while importing your data.\nPlease try again.\nIf the problem persists, contact support.",
      comment:
        "The message shown on the import data screen when importing the data from the browser profile failed."
    )

    public static let importStateSuccessTitle = NSLocalizedString(
      "importStateSuccessTitle",
      bundle: .module,
      value: "Successful!",
      comment:
        "The title shown on the import data screen when importing the data from the browser profile succeeded."
    )

    public static let importStateSuccessMessage = NSLocalizedString(
      "importStateSuccessMessage",
      bundle: .module,
      value:
        "Your data has been successfully imported.\nYou're all set to start browsing with Brave.",
      comment:
        "The message shown on the import data screen when importing the data from the browser profile succeeded."
    )

    public static let importStatePasswordConflictTitle = NSLocalizedString(
      "importStatePasswordConflictTitle",
      bundle: .module,
      value: "Password conflicts",
      comment:
        "The title shown on the import data screen when importing the data from has conflicting passwords (when two passwords exist for the same account, but are different, they are conflicting)."
    )

    public static let importStatePasswordConflictMessage = NSLocalizedString(
      "importStatePasswordConflictMessage",
      bundle: .module,
      value: "%d Password conflicts were found.\nChoose what to import.",
      comment:
        "The message shown on the import data screen when importing the data from has conflicting passwords (when two passwords exist for the same account, but are different, they are conflicting). %d is a placeholder for the number of conflicting passwords."
    )

    public static let importStateTryAgainTitle = NSLocalizedString(
      "importStateTryAgainTitle",
      bundle: .module,
      value: "Try Again",
      comment:
        "The title of the button on the import error screen that lets the user try to import the data again"
    )

    public static let importStatePasswordConflictKeepExistingPasswordsTitle = NSLocalizedString(
      "importStatePasswordConflictKeepExistingPasswordsTitle",
      bundle: .module,
      value: "Keep passwords from Brave",
      comment:
        "The title of the button on the import error screen when there is a conflict, that lets the user keep your existing Brave passwords."
    )

    public static let importStatePasswordConflictKeepImportedPasswordsTitle = NSLocalizedString(
      "importStatePasswordConflictKeepImportedPasswordsTitle",
      bundle: .module,
      value: "Use passwords from Safari",
      comment:
        "The title of the button on the import error screen when there is a conflict, that lets the user use the passwords that were imported, overwriting your existing passwords."
    )

    public static let importStateSuccessContinueTitle = NSLocalizedString(
      "importStateSuccessContinueTitle",
      bundle: .module,
      value: "Continue",
      comment:
        "The title of the button on the import success screen to let the user continue using the app as before."
    )

    // MARK: - Import Tutorial View

    public static let importTutorialStepTitle = NSLocalizedString(
      "importTutorialStepTitle",
      bundle: .module,
      value: "Step %d",
      comment:
        "The title on each card indicating the tutorial step that has been completed. Step 1, Step 2, Step 3, etc... %d is a placeholder for the step number."
    )

    public static let importTutorialStepOneTitle = NSLocalizedString(
      "importTutorialStepOneTitle",
      bundle: .module,
      value: "Open **Settings**, scroll down, and tap **Apps**.",
      comment:
        "The title on each card for the tutorial step that has been completed. Anything in between ** is bold. Do not remove the **"
    )

    public static let importTutorialStepTwoTitle = NSLocalizedString(
      "importTutorialStepTwoTitle",
      bundle: .module,
      value: "From the list of apps (sorted alphabetically), find and tap **Safari**.",
      comment:
        "The title on each card for the tutorial step that has been completed. Anything in between ** is bold. Do not remove the **"
    )

    public static let importTutorialStepThreeTitle = NSLocalizedString(
      "importTutorialStepThreeTitle",
      bundle: .module,
      value: "Scroll down and tap **Export**.",
      comment:
        "The title on each card for the tutorial step that has been completed. Anything in between ** is bold. Do not remove the **"
    )

    public static let importTutorialStepFourTitle = NSLocalizedString(
      "importTutorialStepFourTitle",
      bundle: .module,
      value:
        "**Select** the data you want to export (Bookmarks, History, Credit Cards, Passwords).",
      comment:
        "The title on each card for the tutorial step that has been completed. Anything in between ** is bold. Do not remove the **"
    )

    public static let importTutorialStepFiveTitle = NSLocalizedString(
      "importTutorialStepFiveTitle",
      bundle: .module,
      value: "Save the file in **Files**, choosing a location you can easily find later.",
      comment:
        "The title on each card for the tutorial step that has been completed. Anything in between ** is bold. Do not remove the **"
    )

    public static let importTutorialDetailedProcessTitle = NSLocalizedString(
      "importTutorialDetailedProcessTitle",
      bundle: .module,
      value: "Want a detailed export process?",
      comment:
        "The title on the tutorial page explaining the detailed process to export your data from other browsers."
    )

    public static let importTutorialDetailedProcessMessage = NSLocalizedString(
      "importTutorialDetailedProcessMessage",
      bundle: .module,
      value: "Open Safari help page",
      comment:
        "The message on the tutorial page explaining the detailed process to export your data from other browsers."
    )

    public static let importTutorialScreenTitle = NSLocalizedString(
      "importTutorialScreenTitle",
      bundle: .module,
      value: "How to export from Safari",
      comment:
        "The title of the tutorial page that shows in the Navigation Bar."
    )

    // MARK: - Data Import View

    public static let importDataFileSelectorTitle = NSLocalizedString(
      "importDataFileSelectorTitle",
      bundle: .module,
      value: "Import Bookmarks and more",
      comment:
        "The title of a Text on screen that explains to the user what type of data can be imported."
    )

    public static let importDataFileSelectorMessage = NSLocalizedString(
      "importDataFileSelectorMessage",
      bundle: .module,
      value: "Bring your bookmarks, history, and other browser data into Brave.",
      comment:
        "The message of a Text on screen that explains to the user what type of data can be imported."
    )

    public static let importDataFileSelectorButtonTitle = NSLocalizedString(
      "importDataFileSelectorButtonTitle",
      bundle: .module,
      value: "Choose a file...",
      comment:
        "The title of the button on the file selector screen that lets the user select a zip file (or other files) containing data to be imported."
    )

    public static let importDataViewTutorialButtonTitle = NSLocalizedString(
      "importDataViewTutorialButtonTitle",
      bundle: .module,
      value: "How to export from Safari",
      comment:
        "The title of the button on the file selector screen that lets the user view the tutorial screen on how to import data."
    )

    public static let importDataFileSelectorNavigationTitle = NSLocalizedString(
      "importDataFileSelectorNavigationTitle",
      bundle: .module,
      value: "Import Browsing Data",
      comment:
        "The title of the main screen that lets the user import data. The title is shown in the navigation bar."
    )
  }

  public struct DataImportError {
    public static let failedToUnzipError =
      NSLocalizedString(
        "failedToUnzipError",
        bundle: .module,
        value: "An error occurred while unzipping the file.",
        comment:
          "Error message shown when unzipping a zip file failed"
      )

    public static let failedToImportBookmarksError =
      NSLocalizedString(
        "failedToImportBookmarks",
        bundle: .module,
        value: "An error occurred while importing Bookmarks.",
        comment:
          "Error message shown when importing bookmarks failed"
      )

    public static let failedToImportHistoryError =
      NSLocalizedString(
        "failedToImportHistory",
        bundle: .module,
        value: "An error occurred while importing History.",
        comment:
          "Error message shown when importing history failed"
      )

    public static let failedToImportPasswordsError =
      NSLocalizedString(
        "failedToImportPasswordsError",
        bundle: .module,
        value: "An error occurred while importing Passwords.",
        comment:
          "Error message shown when importing passwords failed"
      )

    public static let failedToImportPasswordsDueToConflictError =
      NSLocalizedString(
        "failedToImportPasswordsDueToConflictError",
        bundle: .module,
        value:
          "An error occurred while importing Passwords.\nSome passwords could not be imported due to conflicts.",
        comment:
          "Error message shown when importing passwords failed due to a password already existing (a conflict)"
      )

    public static let invalidZipFileDataError =
      NSLocalizedString(
        "invalidZipFileDataError",
        bundle: .module,
        value: "The zip file does not contain import data.",
        comment:
          "Error message shown when the zip file doesn't contain any history, bookmarks, passwords, or credit cards to import"
      )

    public static let unknownError =
      NSLocalizedString(
        "unknownError",
        bundle: .module,
        value: "The zip file does not contain import data.",
        comment:
          "An unknown error occurred during data importing."
      )
  }
}
