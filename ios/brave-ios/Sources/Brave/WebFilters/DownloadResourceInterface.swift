// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An object representing errors with the resource downloader
enum ResourceFileError: Error {
  case failedToCreateCacheFolder
}

/// An object providing the interface of a download resource which can be used with the `ResourceDownloader`.
/// This provides a generic multi-purpose way of downloading any files.
public protocol DownloadResourceInterface: Sendable {
  /// The folder name under which this data should be saved under
  var cacheFolderName: String { get }
  var cacheFileName: String { get }
  var externalURL: URL { get }
  var headers: [String: String] { get }
}

extension DownloadResourceInterface {
  /// The directory to which we should store all the dowloaded files into
  private static var cacheFolderDirectory: FileManager.SearchPathDirectory {
    return FileManager.SearchPathDirectory.applicationSupportDirectory
  }
  
  /// The name of the etag save into the cache folder
  var etagFileName: String {
    return [cacheFileName, "etag"].joined(separator: ".")
  }
  
  /// Get the downloaded file URL for this resource
  ///
  /// - Note: Returns nil if the file does not exist
  var downloadedFileURL: URL? {
    guard let cacheFolderURL = createdCacheFolderURL else {
      return nil
    }
    
    let fileURL = cacheFolderURL.appendingPathComponent(cacheFileName)
    
    if FileManager.default.fileExists(atPath: fileURL.path) {
      return fileURL
    } else {
      return nil
    }
  }
  
  /// Get the file url for the downloaded file's etag
  ///
  /// - Note: Returns nil if the etag does not exist
  var createdEtagURL: URL? {
    guard let cacheFolderURL = createdCacheFolderURL else { return nil }
    let fileURL = cacheFolderURL.appendingPathComponent(etagFileName)
    
    if FileManager.default.fileExists(atPath: fileURL.path) {
      return fileURL
    } else {
      return nil
    }
  }
  
  /// Get the cache folder for this resource
  ///
  /// - Note: Returns nil if the cache folder does not exist
  var createdCacheFolderURL: URL? {
    guard let folderURL = Self.cacheFolderDirectory.url else { return nil }
    let cacheFolderURL = folderURL.appendingPathComponent(cacheFolderName)
    
    if FileManager.default.fileExists(atPath: cacheFolderURL.path) {
      return cacheFolderURL
    } else {
      return nil
    }
  }
  
  /// Load the data for this resource
  ///
  /// - Note: Return nil if the data does not exist
  func downloadedData() throws -> Data? {
    guard let fileUrl = downloadedFileURL else { return nil }
    return FileManager.default.contents(atPath: fileUrl.path)
  }
  
  /// Load the string for this resource.
  ///
  /// - Note: Return nil if the data does not exist or if the file is not in the correct encoding
  func downloadedString(encoding: String.Encoding = .utf8) throws -> String? {
    guard let data = try downloadedData() else { return nil }
    return String(data: data, encoding: encoding)
  }
  
  /// Get a creation date for the downloaded file
  ///
  /// - Note: Return nil if the data does not exist
  func creationDate() throws -> Date? {
    guard let fileURL = downloadedFileURL else { return nil }
    let fileAttributes = try FileManager.default.attributesOfItem(atPath: fileURL.path)
    return fileAttributes[.creationDate] as? Date
  }
  
  /// Get an existing etag for this resource..
  ///
  /// - Note: If no etag is created (i.e. the file is not downloaded) a nil is returned.
  func createdEtag() throws -> String? {
    guard let fileURL = createdEtagURL else { return nil }
    guard let data = FileManager.default.contents(atPath: fileURL.path) else { return nil }
    return String(data: data, encoding: .utf8)
  }
  
  /// Removes file for the given `Resource`. The containing folder is not removed.
  func removeFile() throws {
    guard
      let fileURL = downloadedFileURL
    else {
      return
    }
    
    try FileManager.default.removeItem(atPath: fileURL.path)
  }
  
  /// Removes all the data for the given `Resource`
  func removeCacheFolder() throws {
    guard
      let folderURL = createdCacheFolderURL
    else {
      return
    }
    
    try FileManager.default.removeItem(atPath: folderURL.path)
  }
  
  /// Get or create a cache folder for the given `Resource`
  ///
  /// - Note: This technically can't really return nil as the location and folder are hard coded
  func getOrCreateCacheFolder() throws -> URL {
    guard let folderURL = FileManager.default.getOrCreateFolder(
      name: cacheFolderName,
      location: Self.cacheFolderDirectory
    ) else {
      throw ResourceFileError.failedToCreateCacheFolder
    }
    
    return folderURL
  }
  
  /// Get an object representing the cached download result.
  /// If nothing is downloaded, nil is returned.
  func cachedResult() throws -> ResourceDownloader<Self>.DownloadResult? {
    guard let fileURL = downloadedFileURL else { return nil }
    guard let creationDate = try creationDate() else { return nil }
    return ResourceDownloader<Self>.DownloadResult(date: creationDate, fileURL: fileURL, isModified: false)
  }
}
