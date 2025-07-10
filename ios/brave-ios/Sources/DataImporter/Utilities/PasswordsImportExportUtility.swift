// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

class PasswordsImportExportUtility {

  @MainActor
  func importPasswords(from path: URL) async -> BravePasswordImporter.Results? {
    precondition(
      state == .none,
      "Passwords Import - Error Importing while an Import/Export operation is in progress"
    )

    let doImport = { (path: URL, nativePath: String) async -> BravePasswordImporter.Results? in
      await withCheckedContinuation { continuation in
        self.importer.importPasswords(nativePath) { [weak self] results in
          guard let self else {
            // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
            // (Note: this is not reference counted)
            continuation.resume(returning: results)
            return
          }

          self.state = .none

          if results.status == .success || results.status == .none {
            Logger.module.debug("Passwords Import - Import Completed")
          } else {
            Logger.module.debug(
              "Passwords Import - Import Completed With Status: \(String(describing: results.status))"
            )
          }

          continuation.resume(returning: results)
        }
      }
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
        return nil
      }

      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      // File names in the zip file are localized, so we iterate all csv files
      // and try to import
      let contents = try? await AsyncFileManager.default.contentsOfDirectory(
        at: zipFileExtractedURL,
        includingPropertiesForKeys: nil
      )
      let potentialPasswordsFiles =
        contents?.filter { $0.pathExtension.lowercased() == "csv" } ?? []

      // If there's no files to import, return nil
      // as it is fine to have not export passwords in the zip file
      if potentialPasswordsFiles.isEmpty {
        return nil
      }

      for passwordsFileURL in potentialPasswordsFiles {
        if let result = await doImport(passwordsFileURL, passwordsFileURL.path),
          result.status != .badFormat
        {
          return result
        }
      }

      return nil
    }

    return await doImport(path, path.path)
  }

  func continueImporting(
    _ entries: [BravePasswordImportEntry]
  ) async -> BravePasswordImporter.Results? {
    state = .importing
    defer { state = .none }
    return await importer.continueImport(entries)
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
