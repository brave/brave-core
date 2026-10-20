// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

/// When the share extension gives up on sending directly and hands the items
/// to the main app (design plan §5.3).
public struct HandoffPolicy: Sendable {
  public enum Reason: Equatable, Sendable {
    case tooLarge
    case noDevice
    case localNetworkUnavailable
    case userRequested
  }

  /// Items bigger than this in total are sent by the app, which is not
  /// jetsam-limited and can take a background-task grace period.
  public var maxDirectBytes: UInt64 = 500 * 1024 * 1024
  /// How long the extension waits for a computer to connect.
  public var noDeviceTimeout: TimeInterval = 10

  public init() {}

  public func reasonBeforeSending(totalBytes: UInt64) -> Reason? {
    totalBytes > maxDirectBytes ? .tooLarge : nil
  }
}

public struct OutboxManifest: Codable, Equatable, Sendable {
  public struct Item: Codable, Equatable, Sendable {
    public var kind: TransferItemKind
    /// Display name for files; ignored for text and links.
    public var name: String
    public var mimeType: String
    /// File in the outbox folder, for images and files.
    public var fileName: String?
    /// For text and links.
    public var text: String?

    public init(
      kind: TransferItemKind, name: String, mimeType: String, fileName: String? = nil,
      text: String? = nil
    ) {
      self.kind = kind
      self.name = name
      self.mimeType = mimeType
      self.fileName = fileName
      self.text = text
    }
  }

  public var version = 1
  public var id: UUID
  /// Seconds since the Unix epoch.
  public var createdAt: TimeInterval
  /// The device the user picked in the extension, if any.
  public var deviceID: String?
  public var items: [Item]

  public init(id: UUID, createdAt: TimeInterval, deviceID: String? = nil, items: [Item]) {
    self.id = id
    self.createdAt = createdAt
    self.deviceID = deviceID
    self.items = items
  }
}

/// `TransferOutbox/<uuid>/` in the app group: the items plus `manifest.json`.
public struct Outbox {
  public static let directoryName = "TransferOutbox"
  public static let manifestName = "manifest.json"
  public static let maxAge: TimeInterval = 24 * 60 * 60

  public let root: URL

  public init(appGroupContainer: URL) {
    root = appGroupContainer.appendingPathComponent(Self.directoryName, isDirectory: true)
  }

  public func directory(for id: UUID) -> URL {
    root.appendingPathComponent(id.uuidString, isDirectory: true)
  }

  /// Starts an outbox entry. Add files with `addFile`, then `commit`.
  public func create(now: TimeInterval = Date().timeIntervalSince1970) throws -> OutboxWriter {
    let id = UUID()
    let directory = directory(for: id)
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    return OutboxWriter(
      directory: directory, manifest: OutboxManifest(id: id, createdAt: now, items: []))
  }

  public func manifest(for id: UUID) -> OutboxManifest? {
    let url = directory(for: id).appendingPathComponent(Self.manifestName)
    guard let data = try? Data(contentsOf: url) else { return nil }
    return try? JSONDecoder().decode(OutboxManifest.self, from: data)
  }

  /// The items to send for an entry, with file items streaming from the
  /// outbox folder. Hashes every file, so call it off the engine queue.
  public func outgoingItems(for id: UUID) throws -> [OutgoingItem] {
    guard let manifest = manifest(for: id) else { throw CocoaError(.fileNoSuchFile) }
    let directory = directory(for: id)
    return try manifest.items.compactMap { item -> OutgoingItem? in
      switch item.kind {
      case .text, .link:
        return item.text.flatMap { OutgoingItems.text($0) }
      case .image, .file:
        guard let fileName = item.fileName,
          let url = Self.contained(fileName, in: directory)
        else { return nil }
        return try OutgoingItems.file(at: url, name: item.name, mimeType: item.mimeType)
      }
    }
  }

  public func remove(_ id: UUID) {
    try? FileManager.default.removeItem(at: directory(for: id))
  }

  /// Deletes entries older than `maxAge` and folders without a manifest
  /// (an extension killed mid-copy). Returns the ids that were removed.
  @discardableResult
  public func purge(now: TimeInterval = Date().timeIntervalSince1970) -> [String] {
    guard
      let children = try? FileManager.default.contentsOfDirectory(
        at: root, includingPropertiesForKeys: [.creationDateKey])
    else { return [] }
    var removed: [String] = []
    for child in children {
      let name = child.lastPathComponent
      let created: TimeInterval
      if let id = UUID(uuidString: name), let manifest = manifest(for: id) {
        created = manifest.createdAt
      } else {
        created =
          (try? child.resourceValues(forKeys: [.creationDateKey]))?.creationDate?
          .timeIntervalSince1970 ?? 0
      }
      if now - created > Self.maxAge {
        try? FileManager.default.removeItem(at: child)
        removed.append(name)
      }
    }
    return removed
  }

  /// `<scheme>://transfer/send?outbox=<uuid>`, handled by the app's
  /// NavigationRouter.
  public static func handoffURL(scheme: String, id: UUID) -> URL? {
    var components = URLComponents()
    components.scheme = scheme
    components.host = "transfer"
    components.path = "/send"
    components.queryItems = [URLQueryItem(name: "outbox", value: id.uuidString)]
    return components.url
  }

  public static func outboxID(fromHandoffURL url: URL) -> UUID? {
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
      components.host == "transfer", components.path == "/send",
      let value = components.queryItems?.first(where: { $0.name == "outbox" })?.value
    else { return nil }
    return UUID(uuidString: value)
  }

  /// Rejects manifest file names that would escape the entry's folder.
  static func contained(_ fileName: String, in directory: URL) -> URL? {
    guard !fileName.isEmpty, !fileName.contains("/"), fileName != ".", fileName != ".." else {
      return nil
    }
    return directory.appendingPathComponent(fileName)
  }
}

public final class OutboxWriter {
  public let directory: URL
  public private(set) var manifest: OutboxManifest

  init(directory: URL, manifest: OutboxManifest) {
    self.directory = directory
    self.manifest = manifest
  }

  public var id: UUID { manifest.id }

  public func setDevice(_ deviceID: String?) {
    manifest.deviceID = deviceID
  }

  public func addText(_ text: String, isLink: Bool) {
    manifest.items.append(
      OutboxManifest.Item(
        kind: isLink ? .link : .text, name: "", mimeType: isLink ? "text/uri-list" : "text/plain",
        text: text))
  }

  /// Copies (or moves, if `move`) `source` into the entry.
  public func addFile(at source: URL, name: String, mimeType: String, move: Bool = false) throws {
    let fileName = "\(manifest.items.count)-\(FilenameSanitizer.sanitize(name))"
    let destination = directory.appendingPathComponent(fileName)
    if move {
      try FileManager.default.moveItem(at: source, to: destination)
    } else {
      try FileManager.default.copyItem(at: source, to: destination)
    }
    manifest.items.append(
      OutboxManifest.Item(
        kind: mimeType.lowercased().hasPrefix("image/") ? .image : .file,
        name: name, mimeType: mimeType, fileName: fileName))
  }

  /// Writes the manifest last, so the app never sees a half-copied entry.
  public func commit() throws {
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys]
    try encoder.encode(manifest).write(
      to: directory.appendingPathComponent(Outbox.manifestName), options: .atomic)
  }

  public func discard() {
    try? FileManager.default.removeItem(at: directory)
  }
}
