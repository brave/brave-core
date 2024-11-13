// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import Foundation
import Shared
import os.log

class BookmarksImportExportUtility {

  // Import an array of bookmarks into BraveCore
  @MainActor
  func importBookmarks(
    from array: [BraveImportedBookmark]
  ) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Importing while an Import/Export operation is in progress"
    )

    state = .importing
    return await withCheckedContinuation { continuation in
      self.importer.import(from: array, topLevelFolderName: Strings.Sync.importFolderName) {
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
  func importBookmarks(from path: URL) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Importing while an Import/Export operation is in progress"
    )

    let doImport = { (path: URL, nativePath: String) async -> Bool in
      await withCheckedContinuation { continuation in
        self.importer.import(
          fromFile: nativePath,
          topLevelFolderName: Strings.Sync.importFolderName,
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

    guard let nativePath = BookmarksImportExportUtility.nativeURLPathFromURL(path) else {
      Logger.module.error("Bookmarks Import - Invalid FileSystem Path")
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

      guard let nativeImportsPath = BookmarksImportExportUtility.nativeURLPathFromURL(importsPath)
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
        let bookmarksFileURL = importsPath.appending(path: "Bookmarks").appendingPathExtension(
          "html"
        )
        guard
          let nativeBookmarksPath = BookmarksImportExportUtility.nativeURLPathFromURL(
            bookmarksFileURL
          )
        else {
          Logger.module.error("Bookmarks Import - Invalid FileSystem Path")
          return false
        }

        // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
        // (Note: this is not reference counted)
        defer { path.stopAccessingSecurityScopedResource() }
        return await doImport(bookmarksFileURL, nativeBookmarksPath)
      }

      return false
    }

    // Each call to startAccessingSecurityScopedResource must be balanced with a call to stopAccessingSecurityScopedResource
    // (Note: this is not reference counted)
    defer { path.stopAccessingSecurityScopedResource() }
    return await doImport(path, nativePath)
  }

  // Export bookmarks from BraveCore to a file
  @MainActor
  func exportBookmarks(to path: URL) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Exporting while an Import/Export operation is in progress"
    )

    guard let nativePath = BookmarksImportExportUtility.nativeURLPathFromURL(path) else {
      Logger.module.error("Bookmarks Export - Invalid FileSystem Path")
      return false
    }

    self.state = .exporting
    return await withCheckedContinuation { continuation in
      self.exporter.export(toFile: nativePath) { [weak self] state in
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

// MARK: - Parsing
extension BookmarksImportExportUtility {
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
