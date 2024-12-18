// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

class PasswordsImportExportUtility {

  @MainActor
  func importPasswords(from path: URL) async -> Bool {
    precondition(
      state == .none,
      "Passwords Import - Error Importing while an Import/Export operation is in progress"
    )

    let doImport = { (path: URL, nativePath: String) async -> Bool in
      await withCheckedContinuation { continuation in
        self.importer.importPasswords(nativePath) { [weak self] in
          guard let self else {
            // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
            // (Note: this is not reference counted)
            continuation.resume(returning: false)
            return
          }

          self.state = .none

          Logger.module.debug("Passwords Import - Import Completed")
          continuation.resume(returning: true)
        }
      }
    }

    guard let nativePath = path.fileSystemRepresentation else {
      Logger.module.error("Passwords Import - Invalid FileSystem Path")
      return false
    }

    // While accessing document URL from UIDocumentPickerViewController to access the file
    // startAccessingSecurityScopedResource should be called for that URL
    // Reference: https://stackoverflow.com/a/73912499/2239348
    let hasSecurityAccess = path.startAccessingSecurityScopedResource()
    defer {
      // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
      // (Note: this is not reference counted)
      if hasSecurityAccess {
        path.stopAccessingSecurityScopedResource()
      }
    }

    state = .importing
    defer { state = .none }

    if path.pathExtension.lowercased() == "zip" {
      guard let zipFileExtractedURL = try? await ZipImporter.unzip(path: path) else {
        return false
      }

      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      let passwordsFileURL = zipFileExtractedURL.appending(path: "Passwords")
        .appendingPathExtension(
          "csv"
        )

      guard
        let nativePasswordsPath = passwordsFileURL.fileSystemRepresentation
      else {
        Logger.module.error("Passwords Import - Invalid FileSystem Path")
        return false
      }

      return await doImport(passwordsFileURL, nativePasswordsPath)
    }

    return await doImport(path, nativePath)
  }

  // MARK: - Private
  private var state: State = .none
  private let importer = BravePasswordImporter()

  private enum State {
    case importing
    case exporting
    case none
  }
}
