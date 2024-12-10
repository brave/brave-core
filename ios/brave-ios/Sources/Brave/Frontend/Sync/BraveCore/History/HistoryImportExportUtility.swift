// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

    guard let nativePath = path.fileSystemRepresentation else {
      Logger.module.error("History Import - Invalid FileSystem Path")
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
      // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
      // (Note: this is not reference counted)
      defer { path.stopAccessingSecurityScopedResource() }

      guard let zipFileExtractedURL = try? await ZipImporter.unzip(path: path) else {
        return false
      }

      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      let historyFileURL = zipFileExtractedURL.appending(path: "History").appendingPathExtension(
        "json"
      )
      guard
        let nativeHistoryPath = historyFileURL.fileSystemRepresentation
      else {
        Logger.module.error("History Import - Invalid FileSystem Path")
        return false
      }

      return await doImport(historyFileURL, nativeHistoryPath)
    }

    // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
    // (Note: this is not reference counted)
    defer { path.stopAccessingSecurityScopedResource() }
    return await doImport(path, nativePath)
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
