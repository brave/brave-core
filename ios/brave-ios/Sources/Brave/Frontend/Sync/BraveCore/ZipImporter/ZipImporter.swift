// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

class ZipImporter {
  enum ZipImportError: Error {
    case failedToUnzip
    case invalidFileSystemURL
  }

  @MainActor
  static func unzip(path: URL) async throws -> URL {
    // The folder where we want to extract our zip-file's folder to
    let extractionFolder = "SafariImports"

    // The zip file's name - The name of the extracted folder
    let fileName = path.deletingPathExtension().lastPathComponent

    // The path where our zip-file's folder will be extracted to
    let extractionPath = AsyncFileManager.default.temporaryDirectory.appending(
      path: extractionFolder
    )

    // If the extraction path already exists, delete the folder
    // If deletion fails, we'll continue extraction to a unique folder
    if await AsyncFileManager.default.fileExists(atPath: extractionPath.path) {
      do {
        try await AsyncFileManager.default.removeItem(atPath: extractionPath.path)
      } catch {
        Logger.module.error("ZipImporter - Error deleting directory: \(error)")
      }
    }

    // Get unique extraction path
    guard
      let tempDirectoryImportPath = try? await ZipImporter.uniqueFileName(
        extractionFolder,
        folder: AsyncFileManager.default.temporaryDirectory
      )
    else {
      throw ZipImportError.failedToUnzip
    }

    // Create the temporary directory we'll extract the zip to
    do {
      try await AsyncFileManager.default.createDirectory(
        at: tempDirectoryImportPath,
        withIntermediateDirectories: true
      )
    } catch {
      Logger.module.error("ZipImporter - Error creating directory: \(error)")
      throw ZipImportError.failedToUnzip
    }

    // Path to the zip file
    guard let nativeImportPath = path.fileSystemRepresentation else {
      Logger.module.error("ZipImporter - Invalid FileSystem Path")
      throw ZipImportError.invalidFileSystemURL
    }

    // Path to where the zip will be extracted
    guard let nativeDestinationPath = tempDirectoryImportPath.fileSystemRepresentation else {
      throw ZipImportError.invalidFileSystemURL
    }

    // Extract the zip file to the temporary directory
    if await Unzip.unzip(nativeImportPath, toDirectory: nativeDestinationPath) {
      return tempDirectoryImportPath.appending(path: fileName)
    }

    throw ZipImportError.failedToUnzip
  }
}

// MARK: - Parsing
extension ZipImporter {
  static func uniqueFileName(_ filename: String, folder: URL) async throws -> URL {
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
