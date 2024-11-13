// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// A drop-in replacement for FileManager which guarantees at compile time that operations execute
/// on the cooperative thread pool and not the UI thread. Reach for this by default unless file i/o
/// is required to be done in sequence with other calls (at which point, use FileManager + Dispatch)
///
/// Simply replace `FileManager.default` with `AsyncFileManager.default` and add `await`
public final class AsyncFileManager {
  public static let `default` = AsyncFileManager(fileManager: .default)

  /// The underlying file manager to execute items on
  private let fileManager: FileManager

  /// Create an `AsyncFileManager` with a `FileManager`
  public init(fileManager: FileManager) {
    self.fileManager = fileManager
  }

  // MARK: - Accessing User Directories

  /// Locates the specified common directory in a domain.
  public func url(
    for directory: FileManager.SearchPathDirectory,
    in domain: FileManager.SearchPathDomainMask
  ) throws -> URL {
    // This method is non-async as `create` is explicitly passed as `false`
    try fileManager.url(for: directory, in: domain, appropriateFor: nil, create: false)
  }

  /// Locates and optionally creates the specified common directory in a domain.
  public func url(
    for directory: FileManager.SearchPathDirectory,
    in domain: FileManager.SearchPathDomainMask,
    create: Bool
  ) async throws -> URL {
    try fileManager.url(for: directory, in: domain, appropriateFor: nil, create: create)
  }

  // MARK: - Discovering Directory Contents

  /// Performs a shallow search of the specified directory and returns URLs for the contained items.
  public func contentsOfDirectory(
    at url: URL,
    includingPropertiesForKeys keys: [URLResourceKey]?,
    options mask: FileManager.DirectoryEnumerationOptions = []
  ) async throws -> [URL] {
    try fileManager.contentsOfDirectory(
      at: url,
      includingPropertiesForKeys: keys,
      options: mask
    )
  }

  /// Performs a shallow search of the specified directory and returns the paths of any contained
  /// items.
  public func contentsOfDirectory(
    atPath path: String
  ) async throws -> [String] {
    try fileManager.contentsOfDirectory(atPath: path)
  }

  /// Returns an async stream that can be used to perform a deep enumeration of the directory at the
  /// specified URL.
  public func enumerator(
    at url: URL,
    includingPropertiesForKeys keys: [URLResourceKey]?,
    options mask: FileManager.DirectoryEnumerationOptions = []
  ) -> AsyncStream<URL> {
    let (stream, continuation) = AsyncStream.makeStream(of: URL.self)
    let task = Task {
      if let enumerator = fileManager.enumerator(
        at: url,
        includingPropertiesForKeys: keys,
        options: mask
      ) {
        while let fileURL = enumerator.nextObject() as? URL {
          try Task.checkCancellation()
          continuation.yield(fileURL)
        }
        continuation.finish()
      } else {
        continuation.finish()
      }
    }
    continuation.onTermination = { _ in
      task.cancel()
    }
    return stream
  }

  /// Performs a deep enumeration of the specified directory and returns the paths of all of the
  /// contained subdirectories.
  public func subpathsOfDirectory(atPath path: String) async throws -> [String] {
    try fileManager.subpathsOfDirectory(atPath: path)
  }

  // MARK: - Creating and Deleting Items

  /// Creates a directory with the given attributes at the specified URL.
  public func createDirectory(
    at url: URL,
    withIntermediateDirectories createIntermediates: Bool,
    attributes: [FileAttributeKey: Any]? = nil
  ) async throws {
    try fileManager.createDirectory(
      at: url,
      withIntermediateDirectories: createIntermediates,
      attributes: attributes
    )
  }

  /// Creates a directory with given attributes at the specified path.
  public func createDirectory(
    atPath path: String,
    withIntermediateDirectories createIntermediates: Bool,
    attributes: [FileAttributeKey: Any]? = nil
  ) async throws {
    try fileManager.createDirectory(
      atPath: path,
      withIntermediateDirectories: createIntermediates,
      attributes: attributes
    )
  }

  /// Creates a file with the specified content and attributes at the given location.
  @discardableResult
  public func createFile(
    atPath path: String,
    contents data: Data?,
    attributes attr: [FileAttributeKey: Any]? = nil
  ) async -> Bool {
    fileManager.createFile(atPath: path, contents: data, attributes: attr)
  }

  /// Removes the file or directory at the specified URL.
  public func removeItem(at url: URL) async throws {
    try fileManager.removeItem(at: url)
  }

  /// Removes the file or directory at the specified path.
  public func removeItem(atPath path: String) async throws {
    try fileManager.removeItem(atPath: path)
  }

  // MARK: - Replacing Items

  /// Replaces the contents of the item at the specified URL in a manner that ensures no data
  /// loss occurs.
  public func replaceItemAt(
    _ originalItemURL: URL,
    withItemAt newItemURL: URL,
    backupItemName: String? = nil,
    options: FileManager.ItemReplacementOptions = []
  ) async throws -> URL? {
    try fileManager.replaceItemAt(
      originalItemURL,
      withItemAt: newItemURL,
      backupItemName: backupItemName,
      options: options
    )
  }

  // MARK: - Moving and Copying Items

  /// Copies the file at the specified URL to a new location.
  public func copyItem(
    at srcURL: URL,
    to dstURL: URL
  ) async throws {
    try fileManager.copyItem(at: srcURL, to: dstURL)
  }

  /// Copies the item at the specified path to a new location.
  public func copyItem(
    atPath srcPath: String,
    toPath dstPath: String
  ) async throws {
    try fileManager.copyItem(atPath: srcPath, toPath: dstPath)
  }

  /// Moves the file at the specified URL to a new location.
  public func moveItem(
    at srcURL: URL,
    to dstURL: URL
  ) async throws {
    try fileManager.moveItem(at: srcURL, to: dstURL)
  }

  /// Moves the item at the specified path to a new location.
  public func moveItem(
    atPath srcPath: String,
    toPath dstPath: String
  ) async throws {
    try fileManager.moveItem(atPath: srcPath, toPath: dstPath)
  }

  // MARK: - Determining Access to Files

  /// Returns a Boolean value that indicates whether a file or directory exists at a specified path.
  public func fileExists(atPath path: String) async -> Bool {
    fileManager.fileExists(atPath: path)
  }

  /// Returns a Boolean value that indicates whether a file or directory exists at a specified path.
  public func fileExists(
    atPath path: String,
    isDirectory: UnsafeMutablePointer<ObjCBool>?
  ) async -> Bool {
    fileManager.fileExists(atPath: path, isDirectory: isDirectory)
  }

  // MARK: - Getting and Setting Attributes

  /// Returns the attributes of the item at a given path.
  public func attributesOfItem(atPath path: String) async throws -> [FileAttributeKey: Any] {
    try fileManager.attributesOfItem(atPath: path)
  }

  /// Sets the attributes of the specified file or directory.
  public func setAttributes(
    _ attributes: [FileAttributeKey: Any],
    ofItemAtPath path: String
  ) async throws {
    try fileManager.setAttributes(attributes, ofItemAtPath: path)
  }

  // MARK: - Getting and Comparing File Contents

  /// Returns the contents of the file at the specified path.
  public func contents(atPath path: String) async -> Data? {
    fileManager.contents(atPath: path)
  }

  /// Returns a Boolean value that indicates whether the files or directories in specified paths
  /// have the same contents.
  public func contentsEqual(
    atPath path1: String,
    andPath path2: String
  ) async -> Bool {
    fileManager.contentsEqual(atPath: path1, andPath: path2)
  }
}

/// Utility methods that combine multiple file manager calls
extension AsyncFileManager {

  /// Constructs a url from a system directory, creates it if needed and excludes it from backups if
  /// not otherwise marked to not.
  ///
  /// - Note: Do not use this method if the intent is to pass in
  ///         `FileManager.SearchPathDirectory.itemReplacementDirectory`
  public func url(
    for directory: FileManager.SearchPathDirectory,
    appending pathComponent: String,
    create: Bool,
    excludeFromBackups: Bool = true
  ) async throws -> URL {
    assert(directory != .itemReplacementDirectory, "This method does not support item replacement")
    let searchPathDirectory = try fileManager.url(
      for: directory,
      in: .userDomainMask,
      appropriateFor: nil,
      create: false
    )
    var directory = searchPathDirectory.appending(path: pathComponent)
    let exists = await fileExists(atPath: directory.path(percentEncoded: false))
    if !exists && create {
      try await createDirectory(at: directory, withIntermediateDirectories: true)
      if excludeFromBackups {
        var resourceValues = URLResourceValues()
        resourceValues.isExcludedFromBackup = true
        try directory.setResourceValues(resourceValues)
      }
    }
    return directory
  }

  /// Obtains the total size of all files found in a given directory
  public func sizeOfDirectory(at url: URL) async throws -> UInt64 {
    let allocatedSizeResourceKeys: Set<URLResourceKey> = [
      .isRegularFileKey,
      .fileAllocatedSizeKey,
      .totalFileAllocatedSizeKey,
    ]
    var totalSize: UInt64 = 0
    for await url in enumerator(
      at: url,
      includingPropertiesForKeys: Array(allocatedSizeResourceKeys)
    ) {
      let values = try url.resourceValues(forKeys: allocatedSizeResourceKeys)
      if let isRegularFile = values.isRegularFile, !isRegularFile {
        continue
      }
      totalSize += UInt64(values.totalFileAllocatedSize ?? values.fileAllocatedSize ?? 0)
    }
    return totalSize
  }
}

/// String-based file I/O
extension AsyncFileManager {
  /// Creates a file with a string encoded with UTF8 and attributes at the given location.
  @discardableResult
  public func createUTF8File(
    atPath path: String,
    contents string: String,
    attributes attr: [FileAttributeKey: Any]? = nil
  ) async -> Bool {
    fileManager.createFile(atPath: path, contents: Data(string.utf8), attributes: attr)
  }

  /// Returns the string contents of the file at the specified path.
  public func utf8Contents(
    at url: URL
  ) async -> String? {
    guard let data = fileManager.contents(atPath: url.path(percentEncoded: false)) else {
      return nil
    }
    return String(decoding: data, as: UTF8.self)
  }
}

/// Explicit reusable directories
extension AsyncFileManager {
  /// URL where files downloaded by user are stored.
  /// If the download folder doesn't exists it creates a new one
  public func downloadsPath() async throws -> URL {
    try await url(for: .documentDirectory, appending: "Downloads", create: true)
  }

  /// URL where temporary files are stored.
  public var temporaryDirectory: URL {
    fileManager.temporaryDirectory
  }
}
