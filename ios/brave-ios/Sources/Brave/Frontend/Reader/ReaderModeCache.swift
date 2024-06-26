// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation

private let readerModeCacheErrorDomain = "com.mozilla.client.readermodecache."
enum ReaderModeCacheErrorCode: Int {
  case noPathsFound = 0
}

// NSObject wrapper around ReadabilityResult Swift struct for adding into the NSCache
private class ReadabilityResultWrapper: NSObject {
  let result: ReadabilityResult

  init(readabilityResult: ReadabilityResult) {
    self.result = readabilityResult
    super.init()
  }
}

protocol ReaderModeCache {
  func put(_ url: URL, _ readabilityResult: ReadabilityResult) async throws

  func get(_ url: URL) async throws -> ReadabilityResult

  func delete(_ url: URL, error: NSErrorPointer) async

  func contains(_ url: URL) async -> Bool
}

/// A non-persistent cache for readerized content for times when you don't want to write reader data to disk.
/// For example, when the user is in a private tab, we want to make sure that we leave no trace on the file system
class MemoryReaderModeCache: ReaderModeCache {
  var cache: NSCache<AnyObject, AnyObject>

  init(cache: NSCache<AnyObject, AnyObject> = NSCache()) {
    self.cache = cache
  }

  static let sharedInstance = MemoryReaderModeCache()

  func put(_ url: URL, _ readabilityResult: ReadabilityResult) throws {
    cache.setObject(
      ReadabilityResultWrapper(readabilityResult: readabilityResult),
      forKey: url as AnyObject
    )
  }

  func get(_ url: URL) throws -> ReadabilityResult {
    guard let resultWrapper = cache.object(forKey: url as AnyObject) as? ReadabilityResultWrapper
    else {
      throw NSError(
        domain: readerModeCacheErrorDomain,
        code: ReaderModeCacheErrorCode.noPathsFound.rawValue,
        userInfo: nil
      )
    }
    return resultWrapper.result
  }

  func delete(_ url: URL, error: NSErrorPointer) {
    cache.removeObject(forKey: url as AnyObject)
  }

  func contains(_ url: URL) -> Bool {
    return cache.object(forKey: url as AnyObject) != nil
  }
}

/// Really basic persistent cache to store readerized content. Has a simple hashed structure
/// to avoid storing many items in the same directory.
///
/// This currently lives in ~/Library/Caches so that the data can be pruned in case the OS needs
/// more space. Whether that is a good idea or not is not sure. We have a bug on file to investigate
/// and improve at a later time.
class DiskReaderModeCache: ReaderModeCache {
  static let sharedInstance = DiskReaderModeCache()

  func put(_ url: URL, _ readabilityResult: ReadabilityResult) async throws {
    guard let (cacheDirectoryPath, contentFilePath) = cachePathsForURL(url) else {
      throw NSError(
        domain: readerModeCacheErrorDomain,
        code: ReaderModeCacheErrorCode.noPathsFound.rawValue,
        userInfo: nil
      )
    }

    try await AsyncFileManager.default.createDirectory(
      atPath: cacheDirectoryPath,
      withIntermediateDirectories: true,
      attributes: nil
    )
    let string: String = readabilityResult.encode()
    await AsyncFileManager.default.createUTF8File(atPath: contentFilePath, contents: string)
  }

  func get(_ url: URL) async throws -> ReadabilityResult {
    guard let (_, contentFilePath) = cachePathsForURL(url),
      await AsyncFileManager.default.fileExists(atPath: contentFilePath),
      let string = await AsyncFileManager.default.utf8Contents(at: URL(filePath: contentFilePath)),
      let value = ReadabilityResult(string: string)
    else {
      throw NSError(
        domain: readerModeCacheErrorDomain,
        code: ReaderModeCacheErrorCode.noPathsFound.rawValue,
        userInfo: nil
      )
    }
    return value
  }

  func delete(_ url: URL, error: NSErrorPointer) async {
    guard let (cacheDirectoryPath, _) = cachePathsForURL(url) else { return }

    if await AsyncFileManager.default.fileExists(atPath: cacheDirectoryPath) {
      do {
        try await AsyncFileManager.default.removeItem(atPath: cacheDirectoryPath)
      } catch let error1 as NSError {
        error?.pointee = error1
      }
    }
  }

  func contains(_ url: URL) async -> Bool {
    if let (_, contentFilePath) = cachePathsForURL(url),
      await AsyncFileManager.default.fileExists(atPath: contentFilePath)
    {
      return true
    }

    return false
  }

  fileprivate func cachePathsForURL(
    _ url: URL
  ) -> (cacheDirectoryPath: String, contentFilePath: String)? {
    let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true)
    if !paths.isEmpty, let hashedPath = hashedPathForURL(url) {
      let cacheDirectoryURL = URL(
        fileURLWithPath: NSString.path(withComponents: [paths[0], "ReaderView", hashedPath])
      )
      return (cacheDirectoryURL.path, cacheDirectoryURL.appendingPathComponent("content.json").path)
    }

    return nil
  }

  fileprivate func hashedPathForURL(_ url: URL) -> String? {
    guard let hash = hashForURL(url) else { return nil }

    return NSString.path(withComponents: [
      hash.substring(with: NSRange(location: 0, length: 2)),
      hash.substring(with: NSRange(location: 2, length: 2)), hash.substring(from: 4),
    ]) as String
  }

  fileprivate func hashForURL(_ url: URL) -> NSString? {
    guard let data = url.absoluteString.data(using: .utf8) else { return nil }

    return data.sha1.hexEncodedString as NSString?
  }
}
