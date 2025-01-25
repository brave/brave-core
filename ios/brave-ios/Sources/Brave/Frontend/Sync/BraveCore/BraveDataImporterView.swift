// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import DesignSystem
import SwiftUI
import UniformTypeIdentifiers
import os.log

private enum ImportType {
  case bookmarks
  case history
  case passwords
  case all
}

private enum ImportError: LocalizedError {
  case failedToUnzip
  case failedToImportBookmarks
  case failedToImportHistory
  case failedToImportPasswords
  case invalidZipFileData
  case other(desc: String)

  var errorDescription: String? {
    switch self {
    case .failedToUnzip: "An error occurred while unzipping the file"
    case .failedToImportBookmarks: "An error occurred while importing Bookmarks"
    case .failedToImportHistory: "An error occurred while importing History"
    case .failedToImportPasswords: "An error occurred while importing Passwords"
    case .invalidZipFileData: "The zip file does not contain import data"
    case .other(let desc): desc
    }
  }
}

private struct ImporterInfo {
  var shouldShow: Bool
  var fileTypes = [UTType.zip]
  var importType = ImportType.all
  var didImport = false
}

private class ImportState: ObservableObject {
  @Published
  var zipFileURL: URL?

  @Published
  var profiles = [String: [String: URL]]()

  @Published
  var error: ImportError?

  @Published
  var isImporting = false

  lazy var shouldShowMultipleProfileSelector = Binding(
    get: {
      self.zipFileURL != nil && !self.profiles.isEmpty
    },
    set: { value in
      if !value {
        self.zipFileURL = nil
        self.profiles.removeAll()
        self.isImporting = false
      }
    }
  )

  lazy var shouldShowErrorAlert = Binding(
    get: {
      self.error != nil
    },
    set: { value in
      if !value {
        self.error = nil
      }
    }
  )
}

struct BraveDataImportView: View {

  @State
  private var importerInfo = ImporterInfo(shouldShow: false)

  @ObservedObject
  private var importState = ImportState()

  var body: some View {
    VStack {
      VStack(alignment: .center, spacing: 16.0) {
        Image(
          "browsing_data_import_logo",
          bundle: .module
        )
        .padding(.bottom, 24.0)

        Text("Import Bookmarks and more")
          .font(.body.weight(.semibold))
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .padding(.horizontal, 24.0)

        Text("Bring your bookmarks, history, and other browser data into Brave.")
          .font(.footnote)
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .padding(.horizontal, 24.0)

        Button(
          action: {
            importerInfo = ImporterInfo(shouldShow: true, fileTypes: [.zip], importType: .all)
          },
          label: {
            Text("Choose a file...")
              .font(.body.weight(.semibold))
              .padding()
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
              .frame(maxWidth: .infinity, alignment: .center)
              .background(
                Color(braveSystemName: .buttonBackground),
                in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
              )
          }
        )
        .buttonStyle(.plain)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)

      Spacer()

      NavigationLink(destination: BraveDataImporterTutorialView()) {
        HStack {
          Image(
            "safari-icon",
            bundle: .module
          )

          Text("How to export from Safari")
            .font(.subheadline.weight(.semibold))
            .foregroundStyle(Color(braveSystemName: .textInteractive))
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(12.0)

          Image(braveSystemName: "leo.carat.right")
            .foregroundStyle(Color(braveSystemName: .iconDefault))
            .frame(alignment: .trailing)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.horizontal, 16.0)
        .background(
          Color(braveSystemName: .containerDisabled),
          in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
        )
      }
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
    .navigationTitle("Import Browsing Data")
    .braveSheet(
      isPresented: importState.shouldShowMultipleProfileSelector,
      onDismiss: {
        if !self.importState.isImporting {
          Task {
            if let zipFileURL = importState.zipFileURL {
              try await AsyncFileManager.default.removeItem(at: zipFileURL)
            }
          }
        }
      },
      content: {
        if let zipFileURL = importState.zipFileURL {
          BraveDataImporterMultipleProfilesView(
            zipFileExtractedURL: zipFileURL,
            profiles: importState.profiles,
            onProfileSelected: { profile in
              let zipFileURL = self.importState.zipFileURL
              let profiles = self.importState.profiles

              self.importState.isImporting = true
              self.importState.shouldShowMultipleProfileSelector.wrappedValue = false

              if let zipFileURL, let selectedProfile = profiles[profile] {
                Task {
                  defer {
                    Task {
                      try await AsyncFileManager.default.removeItem(at: zipFileURL)
                    }
                  }

                  do {
                    try await self.importProfile(selectedProfile)
                    importerInfo.didImport = true
                  } catch {
                    importState.error = error as? ImportError
                  }
                }
              }
            }
          )
          .edgesIgnoringSafeArea(.bottom)
        }
      }
    )
    .alert(
      isPresented: importState.shouldShowErrorAlert,
      error: importState.error,
      actions: {
        Button("Okay") {

        }
      }
    )
    .alert(
      "Success",
      isPresented: $importerInfo.didImport,
      actions: {
        Button("Okay") {

        }
      },
      message: {
        Text("Data imported successfully")
      }
    )
    .fileImporter(
      isPresented: $importerInfo.shouldShow,
      allowedContentTypes: importerInfo.fileTypes,
      allowsMultipleSelection: false
    ) { result in
      switch result {
      case .success(let files):
        Task {
          do {
            try await files.asyncForEach { file in
              if file.startAccessingSecurityScopedResource() {
                defer { file.stopAccessingSecurityScopedResource() }
                try await processFile(file, importType: importerInfo.importType)
              }
            }
          } catch {
            importState.error = error as? ImportError
          }
        }
      case .failure(let error):
        importState.error = .other(desc: error.localizedDescription)
      }
    }
  }

  private func processFile(_ file: URL, importType: ImportType) async throws {
    switch importType {
    case .bookmarks:
      try await importBookmarks(file)
    case .history:
      try await importHistory(file)
    case .passwords:
      try await importPasswords(file)
    case .all:
      let zipFileExtractedURL = try await unzipFile(file)
      let profiles = await getProfiles(zipFileExtractedURL)

      if profiles.keys.count > 1 {
        importState.zipFileURL = zipFileExtractedURL
        importState.profiles = profiles
        return
      }

      // Cleanup on exit
      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      // TODO: Localize "Personal"
      if let defaultProfile = profiles["Personal"] {
        try await importProfile(defaultProfile)
        importerInfo.didImport = true
      } else {
        importState.error = .invalidZipFileData
      }
    }
  }

  private func unzipFile(_ file: URL) async throws -> URL {
    do {
      return try await ZipImporter.unzip(path: file)
    } catch {
      throw ImportError.failedToUnzip
    }
  }

  private func getProfiles(_ directory: URL) async -> [String: [String: URL]] {
    var groupedFiles = [String: [String: URL]]()
    let files = await enumerateFiles(in: directory, withExtensions: ["html", "json", "csv"])

    for file in files {
      let fileName = file.lastPathComponent
      let components = fileName.components(separatedBy: " - ")

      let itemName =
        !components.isEmpty ? String(components[0].split(separator: ".").first ?? "") : ""

      // TODO: Localize
      let profile =
        components.count == 2 ? String(components[1].split(separator: ".").first ?? "") : "Personal"

      if groupedFiles[profile] == nil {
        groupedFiles[profile] = [:]
      }

      groupedFiles[profile]?[itemName] = file
    }

    return groupedFiles
  }

  private func importProfile(_ profile: [String: URL]) async throws {
    if let bookmarks = profile["Bookmarks"] {
      try await importBookmarks(bookmarks)
    }

    if let history = profile["History"] {
      try await importHistory(history)
    }

    if let passwords = profile["Passwords"] {
      try await importPasswords(passwords)
    }
  }

  private func importBookmarks(_ file: URL) async throws {
    let importer = BookmarksImportExportUtility()
    let didImport = await importer.importBookmarks(from: file)
    if !didImport {
      throw ImportError.failedToImportBookmarks
    }
  }

  private func importHistory(_ file: URL) async throws {
    let importer = HistoryImportExportUtility()
    let didImport = await importer.importHistory(from: file)
    if !didImport {
      throw ImportError.failedToImportHistory
    }
  }

  private func importPasswords(_ file: URL) async throws {
    let importer = PasswordsImportExportUtility()
    let didImport = await importer.importPasswords(from: file)
    if !didImport {
      throw ImportError.failedToImportPasswords
    }
  }

  func enumerateFiles(
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

#Preview {
  BraveDataImportView()
}
