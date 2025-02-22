// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

class ZipImporter {
  enum ZipImportError: Error {
    case failedToUnzip
    case invalidFileSystemURL
  }

  public static func unzip(path: URL) async throws -> URL {
    // The zip file's name - The name of the extracted folder
    let fileName = path.deletingPathExtension().lastPathComponent

    // The folder where we want to extract our zip-file's folder to
    let extractionFolder =
      fileName.replacingOccurrences(of: " ", with: "").replacingOccurrences(of: "%20", with: "")
      + "-Extracted"

    // The path where our zip-file's folder will be extracted to
    let extractionPath = FileManager.default.temporaryDirectory.appending(
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
      let tempDirectoryImportPath = try? await URL.uniqueFileName(
        extractionFolder,
        in: FileManager.default.temporaryDirectory
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

    // Extract the zip file to the temporary directory
    if await JSUnzip.unzip(path.path, toDirectory: tempDirectoryImportPath.path) {
      // If the file was extracted to a folder of the same name, we return that folder
      let filePath = tempDirectoryImportPath.appending(path: fileName)
      if await AsyncFileManager.default.fileExists(atPath: filePath.path) {
        return filePath
      }

      // Otherwise return the folder we created where we extracted the contents of the file
      return tempDirectoryImportPath
    }

    throw ZipImportError.failedToUnzip
  }
}
