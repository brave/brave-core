// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

public class BookmarksImportExportUtility {

  public init() {}

  // Import an array of bookmarks into BraveCore
  @MainActor
  public func importBookmarks(
    from array: [BraveImportedBookmark]
  ) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Importing while an Import/Export operation is in progress"
    )

    state = .importing
    return await withCheckedContinuation { continuation in
      self.importer.import(from: array, topLevelFolderName: Strings.DataImporter.importFolderName) {
        state in

        self.state = .none
        switch state {
        case .started:
          Logger.module.debug("Bookmarks Import - Import Started")
        case .cancelled:
          Logger.module.debug("Bookmarks Import - Import Cancelled")
          continuation.resume(returning: false)
        case .autoCompleted, .completed:
          Logger.module.debug("Bookmarks Import - Import Completed")
          continuation.resume(returning: true)
        @unknown default:
          fatalError()
        }
      }
    }
  }

  // Import bookmarks from a file into BraveCore
  @MainActor
  public func importBookmarks(from path: URL) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Importing while an Import/Export operation is in progress"
    )

    let doImport = { (path: URL, nativePath: String) async -> Bool in
      await withCheckedContinuation { continuation in
        self.importer.import(
          fromFile: nativePath,
          topLevelFolderName: Strings.DataImporter.importFolderName,
          automaticImport: true
        ) { [weak self] state, bookmarks in
          guard let self else {
            continuation.resume(returning: false)
            return
          }

          self.state = .none
          switch state {
          case .started:
            Logger.module.debug("Bookmarks Import - Import Started")
          case .cancelled:
            Logger.module.debug("Bookmarks Import - Import Cancelled")
            continuation.resume(returning: false)
          case .autoCompleted, .completed:
            Logger.module.debug("Bookmarks Import - Import Completed")
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

      // File names in the zip file are localized, so we iterate all html files
      // and try to import
      let contents = try? await AsyncFileManager.default.contentsOfDirectory(
        at: zipFileExtractedURL,
        includingPropertiesForKeys: nil
      )
      let potentialBookmarksFiles =
        contents?.filter { $0.pathExtension.lowercased() == "html" } ?? []

      // If there's no files to import, return true anyway
      // as it is fine to have not export bookmarks in the zip file
      if potentialBookmarksFiles.isEmpty {
        return true
      }

      for bookmarksFileURL in potentialBookmarksFiles {
        if await doImport(bookmarksFileURL, bookmarksFileURL.path) {
          return true
        }
      }

      return false
    }

    return await doImport(path, path.path)
  }

  // Export bookmarks from BraveCore to a file
  @MainActor
  public func exportBookmarks(to path: URL) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Exporting while an Import/Export operation is in progress"
    )

    self.state = .exporting
    return await withCheckedContinuation { continuation in
      self.exporter.export(toFile: path.path) { [weak self] state in
        guard let self = self else {
          continuation.resume(returning: false)
          return
        }

        self.state = .none
        switch state {
        case .started:
          Logger.module.debug("Bookmarks Export - Export Started")
        case .errorCreatingFile, .errorWritingHeader, .errorWritingNodes:
          Logger.module.debug("Bookmarks Export - Export Error")
          continuation.resume(returning: false)
        case .cancelled:
          Logger.module.debug("Bookmarks Export - Export Cancelled")
          continuation.resume(returning: false)
        case .completed:
          Logger.module.debug("Bookmarks Export - Export Completed")
          continuation.resume(returning: true)
        @unknown default:
          fatalError()
        }
      }
    }
  }

  // MARK: - Private
  private var state: State = .none
  private let importer = BraveBookmarksImporter()
  private let exporter = BraveBookmarksExporter()

  private enum State {
    case importing
    case exporting
    case none
  }
}
