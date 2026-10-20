// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

/// A received file being written.
public struct IncomingFile: Equatable, Sendable {
  public var key: String
  /// Already sanitized.
  public var name: String
  public var kind: TransferItemKind
  public var mimeType: String
  public var size: UInt64
  public var sha256: Bytes

  public init(
    key: String, name: String, kind: TransferItemKind, mimeType: String, size: UInt64,
    sha256: Bytes
  ) {
    self.key = key
    self.name = name
    self.kind = kind
    self.mimeType = mimeType
    self.size = size
    self.sha256 = sha256
  }
}

/// Where a received file ended up.
public struct SavedFile: Equatable, Sendable {
  /// Nil when the file went to Photos.
  public var url: URL?
  public var fileName: String
  public var savedToPhotos: Bool
  /// Media that went to Files because Photos access was denied or the format
  /// was rejected.
  public var usedFallback: Bool
  public var thumbnailURL: String

  public init(
    url: URL?, fileName: String, savedToPhotos: Bool = false, usedFallback: Bool = false,
    thumbnailURL: String = ""
  ) {
    self.url = url
    self.fileName = fileName
    self.savedToPhotos = savedToPhotos
    self.usedFallback = usedFallback
    self.thumbnailURL = thumbnailURL
  }
}

/// Receives the bytes of accepted file items. The session has verified the
/// SHA-256 of every byte before `commit` is called.
public protocol IncomingFileSink: AnyObject {
  func begin(_ file: IncomingFile) throws
  func append(key: String, data: Bytes) throws
  /// `completion` runs on the engine's scheduler with nil on failure.
  func commit(key: String, completion: @escaping (SavedFile?) -> Void)
  /// Deletes the partial file.
  func abort(key: String)
}

/// Moves a complete temporary file to its final place. Called off the engine
/// queue; may complete on any thread.
public protocol ReceivedFileFinalizer: AnyObject {
  func finalize(
    _ file: IncomingFile, temporaryURL: URL, completion: @escaping (SavedFile?) -> Void)
}

/// Streams items into `<temporaryDirectory>/<uuid>.part`, then hands each
/// complete file to a finalizer (Photos or Files on iOS).
public final class TemporaryFileSink: IncomingFileSink {
  private struct Open {
    var file: IncomingFile
    var url: URL
    var handle: FileHandle
  }

  private let temporaryDirectory: URL
  private let finalizer: ReceivedFileFinalizer
  private let scheduler: TransferScheduler
  private let ioQueue = DispatchQueue(label: "com.brave.transfer.finalize")
  private var open: [String: Open] = [:]

  public init(
    temporaryDirectory: URL, finalizer: ReceivedFileFinalizer, scheduler: TransferScheduler
  ) {
    self.temporaryDirectory = temporaryDirectory
    self.finalizer = finalizer
    self.scheduler = scheduler
  }

  public func begin(_ file: IncomingFile) throws {
    try FileManager.default.createDirectory(
      at: temporaryDirectory, withIntermediateDirectories: true)
    let url = temporaryDirectory.appendingPathComponent(UUID().uuidString + ".part")
    guard FileManager.default.createFile(atPath: url.path, contents: nil) else {
      throw CocoaError(.fileWriteUnknown)
    }
    open[file.key] = Open(file: file, url: url, handle: try FileHandle(forWritingTo: url))
  }

  public func append(key: String, data: Bytes) throws {
    guard let entry = open[key] else { throw CocoaError(.fileNoSuchFile) }
    try entry.handle.write(contentsOf: data)
  }

  public func commit(key: String, completion: @escaping (SavedFile?) -> Void) {
    guard let entry = open.removeValue(forKey: key) else {
      scheduler.post { completion(nil) }
      return
    }
    do {
      try entry.handle.close()
    } catch {
      try? FileManager.default.removeItem(at: entry.url)
      scheduler.post { completion(nil) }
      return
    }
    let scheduler = self.scheduler
    let finalizer = self.finalizer
    ioQueue.async {
      finalizer.finalize(entry.file, temporaryURL: entry.url) { saved in
        if saved == nil || FileManager.default.fileExists(atPath: entry.url.path) {
          try? FileManager.default.removeItem(at: entry.url)
        }
        scheduler.post { completion(saved) }
      }
    }
  }

  public func abort(key: String) {
    guard let entry = open.removeValue(forKey: key) else { return }
    try? entry.handle.close()
    try? FileManager.default.removeItem(at: entry.url)
  }

  public var openFileCount: Int { open.count }
}

/// Moves files into a directory with a unique name. On iOS this is
/// Documents/Downloads (Files › On My iPhone › Brave › Downloads).
public final class DirectoryFinalizer: ReceivedFileFinalizer {
  private let directory: () throws -> URL

  public init(directory: @escaping () throws -> URL) {
    self.directory = directory
  }

  public func finalize(
    _ file: IncomingFile, temporaryURL: URL, completion: @escaping (SavedFile?) -> Void
  ) {
    do {
      let directory = try directory()
      try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
      let destination = try Self.moveToUniqueName(temporaryURL, in: directory, name: file.name)
      completion(SavedFile(url: destination, fileName: destination.lastPathComponent))
    } catch {
      completion(nil)
    }
  }

  /// Moves `source` to `directory/name`, or `name (1).ext`, `name (2).ext`…
  /// if that exists.
  public static func moveToUniqueName(_ source: URL, in directory: URL, name: String) throws
    -> URL
  {
    let base = (name as NSString).deletingPathExtension
    let ext = (name as NSString).pathExtension
    for attempt in 0..<10_000 {
      var candidate = attempt == 0 ? base : "\(base) (\(attempt))"
      if !ext.isEmpty {
        candidate += ".\(ext)"
      }
      let destination = directory.appendingPathComponent(candidate)
      do {
        try FileManager.default.moveItem(at: source, to: destination)
        return destination
      } catch CocoaError.fileWriteFileExists {
        continue
      } catch {
        if FileManager.default.fileExists(atPath: destination.path) {
          continue
        }
        throw error
      }
    }
    throw CocoaError(.fileWriteFileExists)
  }
}
