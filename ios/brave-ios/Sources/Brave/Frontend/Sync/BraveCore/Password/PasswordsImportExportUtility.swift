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

    guard let nativePath = PasswordsImportExportUtility.nativeURLPathFromURL(path) else {
      Logger.module.error("Passwords Import - Invalid FileSystem Path")
      return false
    }

    // While accessing document URL from UIDocumentPickerViewController to access the file
    // startAccessingSecurityScopedResource should be called for that URL
    // Reference: https://stackoverflow.com/a/73912499/2239348
    guard path.startAccessingSecurityScopedResource() else {
      return false
    }

    state = .importing
    defer { state = .none }

    if path.pathExtension.lowercased() == "zip" {
      guard
        let importsPath = try? await uniqueFileName(
          "SafariImports",
          folder: AsyncFileManager.default.temporaryDirectory
        )
      else {
        return false
      }

      guard let nativeImportsPath = PasswordsImportExportUtility.nativeURLPathFromURL(importsPath)
      else {
        return false
      }

      do {
        try await AsyncFileManager.default.createDirectory(
          at: importsPath,
          withIntermediateDirectories: true
        )
      } catch {
        return false
      }

      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: importsPath)
        }
      }

      if await Unzip.unzip(nativePath, toDirectory: nativeImportsPath) {
        let passwordsFileURL = importsPath.appending(path: "Passwords").appendingPathExtension(
          "csv"
        )
        guard
          let nativePasswordsPath = PasswordsImportExportUtility.nativeURLPathFromURL(
            passwordsFileURL
          )
        else {
          Logger.module.error("Passwords Import - Invalid FileSystem Path")
          return false
        }

        // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
        // (Note: this is not reference counted)
        defer { path.stopAccessingSecurityScopedResource() }
        return await doImport(passwordsFileURL, nativePasswordsPath)
      }

      return false
    }

    // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
    // (Note: this is not reference counted)
    defer { path.stopAccessingSecurityScopedResource() }
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

// MARK: - Parsing
extension PasswordsImportExportUtility {
  static func nativeURLPathFromURL(_ url: URL) -> String? {
    return url.withUnsafeFileSystemRepresentation { bytes -> String? in
      guard let bytes = bytes else { return nil }
      return String(cString: bytes)
    }
  }

  func uniqueFileName(_ filename: String, folder: URL) async throws -> URL {
    let basePath = folder.appending(path: filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while await AsyncFileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = folder.appending(path: proposedFilenameWithoutExtension)
        .appending(path: fileExtension)
    }

    return proposedPath
  }
}
