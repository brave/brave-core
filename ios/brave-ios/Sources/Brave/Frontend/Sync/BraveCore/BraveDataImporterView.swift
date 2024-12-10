// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import SwiftUI
import UniformTypeIdentifiers

private enum ImportError: LocalizedError {
  case failedToUnzip
  case failedToImportBookmarks
  case failedToImportHistory
  case failedToImportPasswords
  case other(desc: String)

  var errorDescription: String? {
    switch self {
    case .failedToUnzip: "An error occurred while unzipping the file"
    case .failedToImportBookmarks: "An error occurred while importing Bookmarks"
    case .failedToImportHistory: "An error occurred while importing History"
    case .failedToImportPasswords: "An error occurred while importing Passwords"
    case .other(let desc): desc
    }
  }
}

private enum ImportType {
  case bookmarks
  case history
  case passwords
  case all
}

private struct ImporterInfo {
  var shouldShow: Bool
  var fileTypes = [UTType.zip]
  var importType = ImportType.all
  var didImport = false
}

private struct BraveDataImportIconView: View {
  let imageName: String

  var body: some View {
    Image(systemName: imageName)
      .resizable()
      .frame(width: 32.0, height: 32.0)
      .foregroundColor(Color(braveSystemName: .primitivePurple40))
      .padding()
      .background(
        Color.white
      )
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .primitivePurple80), lineWidth: 2.0)
      )
      .contentShape(ContainerRelativeShape())
      .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
  }
}

struct BraveDataImporterView: View {
  @Environment(\.dismiss)
  var dismiss

  @State
  private var importerInfo = ImporterInfo(shouldShow: false)

  @State
  private var shouldShowErrorAlert = false

  @State
  private var importError: ImportError?

  var body: some View {
    List {
      VStack(alignment: .center) {
        ZStack {
          BraveDataImportIconView(imageName: "book.fill")
            .offset(x: -96.0, y: 44.0)
            .zIndex(2.0)

          BraveDataImportIconView(imageName: "clock.fill")
            .offset(x: -44.0, y: 0.0)
            .zIndex(1.0)

          BraveDataImportIconView(imageName: "safari.fill")
            .offset(x: 0.0, y: 44.0)
            .zIndex(2.0)

          BraveDataImportIconView(imageName: "key.fill")
            .offset(x: 44.0, y: 0.0)
            .zIndex(1.0)

          BraveDataImportIconView(imageName: "creditcard.fill")
            .offset(x: 96.0, y: 44.0)
            .zIndex(2.0)
        }
        .padding(.bottom, 60.0)

        Text("Import Browsing Data")
          .multilineTextAlignment(.center)
          .font(.largeTitle.weight(.bold))
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundStyle(Color(braveSystemName: .textSecondary))

        Text("Import your bookmarks, history, passwords, and other browser data into Brave.")
          .multilineTextAlignment(.center)
          .font(.headline)
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .padding()
      .listRowBackground(Color.clear)

      Section {
        Button("Import All") {
          importerInfo = ImporterInfo(shouldShow: true, fileTypes: [.zip], importType: .all)
        }
      } footer: {
        Text("Import all of your Bookmarks, History, and Passwords from Safari")
          .font(.footnote)
          .frame(maxWidth: .infinity, alignment: .leading)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }

      Section {
        Button("Import Bookmarks") {
          importerInfo = ImporterInfo(shouldShow: true, fileTypes: [.html], importType: .bookmarks)
        }

        Button("Import History") {
          importerInfo = ImporterInfo(shouldShow: true, fileTypes: [.json], importType: .history)
        }

        Button("Import Passwords") {
          importerInfo = ImporterInfo(
            shouldShow: true,
            fileTypes: [.commaSeparatedText],
            importType: .passwords
          )
        }
      } footer: {
        Text("Invidivually import Bookmarks, History, or Passwords from Safari")
          .font(.footnote)
          .frame(maxWidth: .infinity, alignment: .leading)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }

      Section {
        Button("Don't Import Anything") {
          print("Dismiss")
        }
      }
    }
    .background(Color(braveSystemName: .containerBackground))
    .alert(
      isPresented: $shouldShowErrorAlert,
      error: importError,
      actions: {
        Button("Okay") {

        }
      }
    )
    .alert(
      "Success!",
      isPresented: $importerInfo.didImport,
      actions: {
        Button("Okay") {
          dismiss()
        }
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

            importerInfo.didImport = true
          } catch {
            self.importError = error as? ImportError
            self.shouldShowErrorAlert = true
          }
        }

      case .failure(let error):
        self.importError = .other(desc: error.localizedDescription)
        self.shouldShowErrorAlert = true
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
      defer {
        Task {
          try await AsyncFileManager.default.removeItem(at: zipFileExtractedURL)
        }
      }

      try await importBookmarks(
        zipFileExtractedURL.appending(path: "Bookmarks", directoryHint: .notDirectory)
          .appendingPathExtension(
            "html"
          )
      )
      try await importHistory(
        zipFileExtractedURL.appending(path: "History", directoryHint: .notDirectory)
          .appendingPathExtension("json")
      )
      try await importPasswords(
        zipFileExtractedURL.appending(path: "Passwords", directoryHint: .notDirectory)
          .appendingPathExtension(
            "csv"
          )
      )
    }
  }

  private func unzipFile(_ file: URL) async throws -> URL {
    do {
      return try await ZipImporter.unzip(path: file)
    } catch {
      throw ImportError.failedToUnzip
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
}

#if DEBUG
struct BraveDataImporterView_Preview: PreviewProvider {

  static var previews: some View {
    return BraveDataImporterView()
      .previewLayout(.sizeThatFits)
  }
}
#endif
