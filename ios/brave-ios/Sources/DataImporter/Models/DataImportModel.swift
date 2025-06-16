// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import SwiftUI
import UniformTypeIdentifiers
import os.log

enum DataImportType {
  case bookmarks
  case history
  case passwords
  case all
}

enum DataImportError: LocalizedError {
  case failedToUnzip
  case failedToImportBookmarks
  case failedToImportHistory
  case failedToImportPasswords
  case failedToImportPasswordsDueToConflict(BravePasswordImporter.Results)
  case invalidZipFileData
  case unknown

  var errorDescription: String? {
    switch self {
    case .failedToUnzip: Strings.DataImportError.failedToUnzipError
    case .failedToImportBookmarks: Strings.DataImportError.failedToImportBookmarksError
    case .failedToImportHistory: Strings.DataImportError.failedToImportHistoryError
    case .failedToImportPasswords: Strings.DataImportError.failedToImportPasswordsError
    case .failedToImportPasswordsDueToConflict:
      Strings.DataImportError.failedToImportPasswordsDueToConflictError
    case .invalidZipFileData: Strings.DataImportError.invalidZipFileDataError
    case .unknown: Strings.DataImportError.unknownError
    }
  }
}

enum DataImportPasswordConflictOption {
  case keepBravePasswords
  case keepSafariPasswords
  case abortImport
}

enum DataImportState {
  case none
  case importing
  case loadingProfiles
  case dataConflict
  case success
  case failure
}

@MainActor
class DataImportModel: ObservableObject {
  @Published
  private(set) var zipFileURL: URL?

  @Published
  private(set) var profiles = [String: [String: URL]]()

  @Published
  var importError: DataImportError?

  @Published
  var importState: DataImportState = .none

  var isLoadingProfiles: Bool {
    get {
      importState == .loadingProfiles
    }
    set {
      if !newValue, importState == .loadingProfiles {
        removeZipFile()
        resetAllStates()
      }
    }
  }

  var hasDataConflict: Bool {
    get {
      importState == .dataConflict
    }
    set {
      if !newValue, importState == .dataConflict {
        removeZipFile()
        resetAllStates()
      }
    }
  }

  // MARK: - Importers

  private let bookmarksImporter = BookmarksImportExportUtility()
  private let historyImporter = HistoryImportExportUtility()
  private var passwordImporter = PasswordsImportExportUtility()

  // MARK: - Importing Data

  @MainActor
  func importData(from profile: [String: URL]) async {
    do {
      self.importState = .importing
      try await importProfile(profile)
      self.importError = nil
      self.importState = .success
    } catch let error as DataImportError {
      self.importError = error

      if case .failedToImportPasswordsDueToConflict = error {
        try? await Task.sleep(seconds: 1.0)
        self.importState = .dataConflict
      } else {
        self.importState = .failure
      }
    } catch {
      Logger.module.error("[DataImporter] - Error: \(error)")
      self.importError = .unknown
      self.importState = .failure
    }
  }

  @MainActor
  func importData(from file: URL, importType: DataImportType) async {
    do {
      switch importType {
      case .bookmarks:
        self.importState = .importing
        try await importBookmarks(file)
        self.importState = .success
      case .history:
        self.importState = .importing
        try await importHistory(file)
        self.importState = .success
      case .passwords:
        self.importState = .importing
        try await importPasswords(file)
        self.importState = .success
      case .all:
        let zipFileExtractedURL = try await unzipFile(file)
        let profiles = await loadProfiles(zipFileExtractedURL)

        if profiles.keys.count > 1 {
          self.zipFileURL = zipFileExtractedURL
          self.profiles = profiles
          self.importError = nil
          self.importState = .loadingProfiles
          return
        }

        // Cleanup on exit
        defer {
          removeZipFile()
        }

        if let defaultProfile = profiles[Strings.DataImporter.personalImportTitle] {
          self.importState = .importing
          try await importProfile(defaultProfile)
          self.importError = nil
          self.importState = .success
        } else {
          throw DataImportError.invalidZipFileData
        }
      }
    } catch let error as DataImportError {
      self.importError = error

      if case .failedToImportPasswordsDueToConflict = error {
        try? await Task.sleep(seconds: 1.0)
        self.importState = .dataConflict
      } else {
        self.importState = .failure
      }
    } catch {
      Logger.module.error("[DataImporter] - Error: \(error)")
      self.importError = .unknown
      self.importState = .failure
    }
  }

  func removeZipFile() {
    Task {
      if let zipFileURL, importState != .importing {
        try await AsyncFileManager.default.removeItem(at: zipFileURL)
      }
    }
  }

  @MainActor
  func resetAllStates() {
    // Reset password importer
    passwordImporter = PasswordsImportExportUtility()
    importError = nil

    if [.loadingProfiles, .dataConflict, .failure].contains(importState) {
      importState = .none
    }
  }

  @MainActor
  func keepPasswords(option: DataImportPasswordConflictOption) async {
    if option == .abortImport {
      self.importError = nil
      self.importState = .none
      return
    }

    do {
      self.importState = .importing
      if case .failedToImportPasswordsDueToConflict(let results) = importError,
        let results = await passwordImporter.continueImporting(
          option == .keepBravePasswords ? [] : results.displayedEntries
        )
      {
        switch results.status {
        case .none, .success:
          self.importError = nil
          self.importState = .success
          return

        case .unknownError, .ioError, .badFormat, .dismissed, .maxFileSize, .importAlreadyActive,
          .numPasswordsExceeded:
          throw DataImportError.failedToImportPasswords

        case .conflicts:
          throw DataImportError.failedToImportPasswordsDueToConflict(results)

        default:
          throw DataImportError.failedToImportPasswords
        }
      }

      throw DataImportError.failedToImportPasswords
    } catch let error as DataImportError {
      self.importError = error

      if case .failedToImportPasswordsDueToConflict = error {
        self.importState = .dataConflict
      } else {
        self.importState = .failure
      }
    } catch {
      Logger.module.error("[DataImporter] - Error: \(error)")
      self.importError = .failedToImportPasswords
      self.importState = .failure
    }
  }

  private func unzipFile(_ file: URL) async throws -> URL {
    do {
      return try await ZipImporter.unzip(path: file)
    } catch {
      throw DataImportError.failedToUnzip
    }
  }

  private func loadProfiles(_ directory: URL) async -> [String: [String: URL]] {
    var groupedFiles = [String: [String: URL]]()
    let files = await enumerateFiles(in: directory, withExtensions: ["html", "json", "csv"])

    for file in files {
      let fileName = file.lastPathComponent
      let components = fileName.components(separatedBy: " - ")

      let itemName =
        !components.isEmpty ? String(components[0].split(separator: ".").first ?? "") : ""

      let profile =
        components.count == 2
        ? String(components[1].split(separator: ".").first ?? "")
        : Strings.DataImporter.personalImportTitle

      if groupedFiles[profile] == nil {
        groupedFiles[profile] = [:]
      }

      groupedFiles[profile]?[itemName] = file
    }

    return groupedFiles
  }

  private func importProfile(_ profile: [String: URL]) async throws {
    // Safari localizes the file names.
    // In Russian for example, "PaymentCards.csv", "Закладки.html", "История.json"
    // So we need to enumerate all such files and `try` to import them.

    let importFile = {
      (files: [URL], doImport: (URL) async throws -> Void, error: Error) async throws -> Void in
      for file in files {
        do {
          try await doImport(file)
          return
        } catch {
          Logger.module.error("[DataImportModel] - Failed to import file: \(error)")
        }
      }

      throw error
    }

    // Try to import bookmarks
    let bookmarks = profile.values.filter({ $0.pathExtension.lowercased() == "html" })
    if !bookmarks.isEmpty {
      try await importFile(bookmarks, importBookmarks, DataImportError.failedToImportBookmarks)
    }

    // Try to import history
    let history = profile.values.filter({ $0.pathExtension.lowercased() == "json" })
    if !history.isEmpty {
      try await importFile(history, importHistory, DataImportError.failedToImportHistory)
    }

    // Try to import passwords
    let passwords = profile.values.filter({ $0.pathExtension.lowercased() == "csv" })
    if !passwords.isEmpty {
      for passwordFile in passwords {
        do {
          try await importPasswords(passwordFile)
          return
        } catch let error as DataImportError {
          if case .failedToImportPasswords = error {
            Logger.module.error("[DataImportModel] - Failed to import file: \(error)")
            continue
          }

          throw error
        }
      }

      throw DataImportError.failedToImportPasswords
    }
  }

  private func importBookmarks(_ file: URL) async throws {
    let didImport = await bookmarksImporter.importBookmarks(from: file)
    if !didImport {
      throw DataImportError.failedToImportBookmarks
    }
  }

  private func importHistory(_ file: URL) async throws {
    let didImport = await historyImporter.importHistory(from: file)
    if !didImport {
      throw DataImportError.failedToImportHistory
    }
  }

  private func importPasswords(_ file: URL) async throws {
    if let results = await passwordImporter.importPasswords(from: file) {
      switch results.status {
      case .none, .success: return
      case .unknownError, .ioError, .badFormat, .dismissed, .maxFileSize, .importAlreadyActive,
        .numPasswordsExceeded:
        throw DataImportError.failedToImportPasswords

      case .conflicts:
        throw DataImportError.failedToImportPasswordsDueToConflict(results)

      default:
        throw DataImportError.failedToImportPasswords
      }
    }

    throw DataImportError.failedToImportPasswords
  }

  private func enumerateFiles(
    in directory: URL,
    withExtensions extensions: [String] = []
  ) async -> [URL] {
    let enumerator = AsyncFileManager.default.enumerator(
      at: directory,
      includingPropertiesForKeys: [.isRegularFileKey]
    )

    var result: [URL] = []
    for await fileURL in enumerator {
      do {
        let resourceValues = try fileURL.resourceValues(forKeys: [.isRegularFileKey])
        if resourceValues.isRegularFile == true && extensions.isEmpty
          || extensions.contains(fileURL.pathExtension)
        {
          result.append(fileURL)
        }
      } catch {
        Logger.module.error("Error reading file \(fileURL): \(error)")
      }
    }

    return result
  }
}
