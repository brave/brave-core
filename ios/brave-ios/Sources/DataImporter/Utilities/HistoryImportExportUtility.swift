// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

class HistoryImportExportUtility {

  @MainActor
  func importHistory(from path: URL) async -> Bool {
    precondition(
      state == .none,
      "History Import - Error Importing while an Import/Export operation is in progress"
    )

    let doImport = { (path: URL, nativePath: String) async -> Bool in
      await withCheckedContinuation { continuation in
        self.importer.import(
          fromFile: nativePath,
          automaticImport: true
        ) { [weak self] state, historyItems in
          guard let self else {
            continuation.resume(returning: false)
            return
          }

          self.state = .none
          switch state {
          case .started:
            Logger.module.debug("History Import - Import Started")
          case .cancelled:
            Logger.module.debug("History Import - Import Cancelled")
            continuation.resume(returning: false)
          case .autoCompleted, .completed:
            Logger.module.debug("History Import - Import Completed")
            continuation.resume(returning: true)
          @unknown default:
            fatalError()
          }
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
        return false
      }

      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      // File names in the zip file are localized, so we iterate all json files
      // and try to import
      let contents = try? await AsyncFileManager.default.contentsOfDirectory(
        at: zipFileExtractedURL,
        includingPropertiesForKeys: nil
      )
      let potentialHistoryFiles = contents?.filter { $0.pathExtension.lowercased() == "json" } ?? []

      // If there's no files to import, return true
      // as it is fine to have not export history in the zip file
      if potentialHistoryFiles.isEmpty {
        return true
      }

      for historyFileURL in potentialHistoryFiles {
        if await doImport(historyFileURL, historyFileURL.path) {
          return true
        }
      }

      return false
    }

    return await doImport(path, path.path)
  }

  // MARK: - Private
  private var state: State = .none
  private let importer = BraveHistoryImporter()

  private enum State {
    case importing
    case exporting
    case none
  }
}
