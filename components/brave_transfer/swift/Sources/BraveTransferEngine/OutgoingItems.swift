// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

public enum OutgoingItems {
  /// Text that is a single http(s) URL is sent as a link, everything else as
  /// text, matching the desktop's SendText.
  public static func text(_ text: String) -> OutgoingItem? {
    guard !text.isEmpty else { return nil }
    let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
    let lower = trimmed.lowercased()
    if (lower.hasPrefix("http://") || lower.hasPrefix("https://")),
      trimmed.utf8.count <= TransferLimits.maxInlineBytes,
      !trimmed.contains(where: { $0 == " " || $0 == "\t" || $0 == "\r" || $0 == "\n" }),
      let url = URL(string: trimmed), url.host != nil
    {
      let name = trimmed.utf8.count <= TransferLimits.maxNameBytes ? trimmed : "link"
      return .fromBytes(kind: .url, name: name, mimeType: "text/uri-list", data: Bytes(trimmed.utf8))
    }
    return .fromBytes(kind: .text, name: "text.txt", mimeType: "text/plain", data: Bytes(text.utf8))
  }

  /// An item that streams from a file. Hashes the whole file first, reading
  /// 1 MiB at a time, so call it off the engine queue for large files.
  public static func file(
    at url: URL, name: String? = nil, mimeType: String? = nil
  ) throws -> OutgoingItem {
    let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
    let size = (attributes[.size] as? NSNumber)?.uint64Value ?? 0
    let digest = try sha256(of: url)
    let resolvedMime = mimeType.flatMap { $0.isEmpty ? nil : $0 } ?? MimeTypes.guess(url.pathExtension)
    var displayName = truncateUTF8(name ?? url.lastPathComponent, maxBytes: TransferLimits.maxNameBytes)
    if displayName.isEmpty {
      displayName = "file"
    }
    let reader = FileChunkReader(url: url, size: size)
    return OutgoingItem(
      kind: resolvedMime.lowercased().hasPrefix("image/") ? .image : .file,
      name: displayName,
      mimeType: resolvedMime.utf8.count <= TransferLimits.maxMimeTypeBytes
        ? resolvedMime : "application/octet-stream",
      size: size, sha256: digest,
      read: { offset, length in try reader.read(offset: offset, length: length) })
  }

  public static func sha256(of url: URL) throws -> Bytes {
    let handle = try FileHandle(forReadingFrom: url)
    defer { try? handle.close() }
    var digest = SHA256Digest()
    while let data = try handle.read(upToCount: 1 << 20), !data.isEmpty {
      digest.update(Bytes(data))
    }
    return digest.finalize()
  }

  static func truncateUTF8(_ value: String, maxBytes: Int) -> String {
    guard value.utf8.count > maxBytes else { return value }
    var result = ""
    var used = 0
    for character in value {
      let count = String(character).utf8.count
      if used + count > maxBytes { break }
      result.append(character)
      used += count
    }
    return result
  }
}

/// Reads a file sequentially, keeping one handle open. The session reads each
/// item from start to end, so this never buffers more than one chunk.
final class FileChunkReader {
  private let url: URL
  private let size: UInt64
  private var handle: FileHandle?

  init(url: URL, size: UInt64) {
    self.url = url
    self.size = size
  }

  deinit {
    try? handle?.close()
  }

  func read(offset: UInt64, length: Int) throws -> Bytes {
    if handle == nil {
      handle = try FileHandle(forReadingFrom: url)
    }
    guard let handle else { throw CocoaError(.fileReadUnknown) }
    try handle.seek(toOffset: offset)
    let data = try handle.read(upToCount: length) ?? Data()
    if offset + UInt64(data.count) >= size {
      try? handle.close()
      self.handle = nil
    }
    return Bytes(data)
  }
}

public enum MimeTypes {
  private static let byExtension: [String: String] = [
    "jpg": "image/jpeg", "jpeg": "image/jpeg", "png": "image/png", "gif": "image/gif",
    "heic": "image/heic", "heif": "image/heif", "webp": "image/webp", "tif": "image/tiff",
    "tiff": "image/tiff", "bmp": "image/bmp", "dng": "image/x-adobe-dng",
    "mov": "video/quicktime", "mp4": "video/mp4", "m4v": "video/x-m4v", "hevc": "video/hevc",
    "pdf": "application/pdf", "txt": "text/plain", "zip": "application/zip",
    "json": "application/json", "html": "text/html", "csv": "text/csv",
    "doc": "application/msword", "key": "application/vnd.apple.keynote",
    "pages": "application/vnd.apple.pages", "numbers": "application/vnd.apple.numbers",
    "m4a": "audio/mp4", "mp3": "audio/mpeg", "wav": "audio/wav",
  ]

  public static func guess(_ pathExtension: String) -> String {
    byExtension[pathExtension.lowercased()] ?? "application/octet-stream"
  }

  /// Images and videos go to Photos on iOS.
  public static func isImageOrVideo(_ mimeType: String) -> Bool {
    let lower = mimeType.lowercased()
    return lower.hasPrefix("image/") || lower.hasPrefix("video/")
  }
}
