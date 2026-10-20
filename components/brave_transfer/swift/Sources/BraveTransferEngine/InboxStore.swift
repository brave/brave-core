// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// A received and accepted item (design plan §4.4).
public struct InboxEntry: Codable, Equatable, Sendable {
  public var id: Int64
  /// Milliseconds since the Unix epoch.
  public var receivedTime: Double
  public var deviceGUID: String
  public var deviceName: String
  public var kind: TransferItemKind
  /// Text or URL for text and link items.
  public var text: String
  /// Where the file was saved, for files and for media that fell back to
  /// Files. Nil for items saved to Photos and for text and links.
  public var filePath: String?
  public var fileName: String
  public var mimeType: String
  public var size: UInt64
  public var sha256: String
  /// A small thumbnail as a data: URL, made at receipt time (with add-only
  /// Photos access the asset cannot be read back).
  public var thumbnailURL: String
  public var savedToPhotos: Bool
  public var savedToFallback: Bool

  public init(
    id: Int64 = 0, receivedTime: Double, deviceGUID: String, deviceName: String,
    kind: TransferItemKind, text: String = "", filePath: String? = nil, fileName: String = "",
    mimeType: String = "", size: UInt64 = 0, sha256: String = "", thumbnailURL: String = "",
    savedToPhotos: Bool = false, savedToFallback: Bool = false
  ) {
    self.id = id
    self.receivedTime = receivedTime
    self.deviceGUID = deviceGUID
    self.deviceName = deviceName
    self.kind = kind
    self.text = text
    self.filePath = filePath
    self.fileName = fileName
    self.mimeType = mimeType
    self.size = size
    self.sha256 = sha256
    self.thumbnailURL = thumbnailURL
    self.savedToPhotos = savedToPhotos
    self.savedToFallback = savedToFallback
  }

  /// Whether a saved file has been moved or deleted since it was received.
  public var isFileMissing: Bool {
    guard let filePath else { return false }
    return !FileManager.default.fileExists(atPath: filePath)
  }
}

public struct InboxPage: Equatable, Sendable {
  public var entries: [InboxEntry]
  public var hasMore: Bool
}

public protocol InboxStore: AnyObject {
  /// Returns the stored entry with its id assigned, or nil on failure.
  @discardableResult
  func add(_ entry: InboxEntry) -> InboxEntry?
  /// Newest first.
  func page(offset: Int, count: Int) -> InboxPage
  func entry(id: Int64) -> InboxEntry?
  /// Deletes every entry. Saved files and Photos assets are never deleted.
  func clear() -> Bool
}

/// The inbox as a JSON file (on iOS, `Application Support/BraveTransfer/
/// inbox.json` in the main app's container). Text is capped at 64 KiB per
/// entry, so the file stays small enough to rewrite on each change.
public final class FileInboxStore: InboxStore {
  public static let maxPage = 500

  private struct Contents: Codable {
    var nextID: Int64 = 1
    /// Oldest first.
    var entries: [InboxEntry] = []
  }

  private let url: URL?
  private var contents = Contents()

  /// `url` nil keeps the inbox in memory (tests).
  public init(url: URL?) {
    self.url = url
    if let url, let data = try? Data(contentsOf: url),
      let decoded = try? JSONDecoder().decode(Contents.self, from: data)
    {
      contents = decoded
    }
  }

  @discardableResult
  public func add(_ entry: InboxEntry) -> InboxEntry? {
    var stored = entry
    stored.id = contents.nextID
    contents.nextID += 1
    contents.entries.append(stored)
    guard save() else {
      contents.entries.removeLast()
      contents.nextID -= 1
      return nil
    }
    return stored
  }

  public func page(offset: Int, count: Int) -> InboxPage {
    let newestFirst = contents.entries.reversed()
    let start = min(max(offset, 0), newestFirst.count)
    let end = min(start + min(max(count, 0), Self.maxPage), newestFirst.count)
    return InboxPage(
      entries: Array(newestFirst.dropFirst(start).prefix(end - start)),
      hasMore: end < newestFirst.count)
  }

  public func entry(id: Int64) -> InboxEntry? {
    contents.entries.first { $0.id == id }
  }

  public func clear() -> Bool {
    let old = contents
    contents.entries.removeAll()
    if !save() {
      contents = old
      return false
    }
    return true
  }

  private func save() -> Bool {
    guard let url else { return true }
    do {
      try FileManager.default.createDirectory(
        at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
      try JSONEncoder().encode(contents).write(to: url, options: .atomic)
      return true
    } catch {
      return false
    }
  }
}
