// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import BraveTransferEngine
import Foundation
import UniformTypeIdentifiers

/// One thing the user shared, ready to hash or to hand to the app.
public enum SharedItem: Equatable, Sendable {
  case text(String)
  case link(String)
  /// A copy in the extension's work folder, which the extension owns.
  case file(url: URL, name: String, mimeType: String, size: UInt64)

  public var size: UInt64 {
    switch self {
    case .text(let text), .link(let text): return UInt64(text.utf8.count)
    case .file(_, _, _, let size): return size
    }
  }
}

/// Reads the share sheet's `NSItemProvider`s. Files are copied out of the
/// provider's temporary location, which is deleted when the load callback
/// returns; nothing is held in memory.
public enum SharedItemLoader {
  public static func load(_ providers: [NSItemProvider], into directory: URL) async -> [SharedItem]
  {
    try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    var items: [SharedItem] = []
    for provider in providers.prefix(TransferLimits.maxItems) {
      if let item = await load(provider, into: directory) {
        items.append(item)
      }
    }
    return items
  }

  static func fileType(of provider: NSItemProvider) -> UTType? {
    let types = provider.registeredTypeIdentifiers.compactMap(UTType.init)
    if let media = types.first(where: {
      $0.conforms(to: .image) || $0.conforms(to: .movie) || $0.conforms(to: .audio)
    }) {
      return media
    }
    if types.contains(where: { $0.conforms(to: .url) || $0.conforms(to: .text) }) {
      return nil
    }
    return types.first { $0.conforms(to: .data) }
  }

  private static func load(_ provider: NSItemProvider, into directory: URL) async -> SharedItem? {
    if let type = fileType(of: provider) {
      return await loadFile(provider, type: type, into: directory)
    }
    if provider.hasItemConformingToTypeIdentifier(UTType.url.identifier),
      let url = await loadURL(provider)
    {
      if url.isFileURL {
        return copy(
          url,
          name: url.lastPathComponent,
          type: UTType(filenameExtension: url.pathExtension),
          into: directory
        )
      }
      return .link(url.absoluteString)
    }
    if provider.hasItemConformingToTypeIdentifier(UTType.text.identifier),
      let text = await loadText(provider), !text.isEmpty
    {
      return .text(text)
    }
    return nil
  }

  private static func loadFile(
    _ provider: NSItemProvider,
    type: UTType,
    into directory: URL
  ) async -> SharedItem? {
    let suggestedName = provider.suggestedName
    return await withCheckedContinuation { continuation in
      _ = provider.loadFileRepresentation(forTypeIdentifier: type.identifier) { url, _ in
        guard let url else {
          continuation.resume(returning: nil)
          return
        }
        var name = suggestedName ?? url.lastPathComponent
        if (name as NSString).pathExtension.isEmpty {
          let ext = url.pathExtension.isEmpty ? type.preferredFilenameExtension : url.pathExtension
          if let ext {
            name = (name as NSString).appendingPathExtension(ext) ?? name
          }
        }
        continuation.resume(returning: copy(url, name: name, type: type, into: directory))
      }
    }
  }

  private static func copy(
    _ source: URL,
    name: String,
    type: UTType?,
    into directory: URL
  ) -> SharedItem? {
    let destination = directory.appendingPathComponent(UUID().uuidString)
    let accessing = source.startAccessingSecurityScopedResource()
    defer {
      if accessing {
        source.stopAccessingSecurityScopedResource()
      }
    }
    do {
      try FileManager.default.copyItem(at: source, to: destination)
    } catch {
      return nil
    }
    let attributes = try? FileManager.default.attributesOfItem(atPath: destination.path)
    let size = (attributes?[.size] as? NSNumber)?.uint64Value ?? 0
    let mimeType =
      type?.preferredMIMEType ?? MimeTypes.guess((name as NSString).pathExtension)
    let safeName = FilenameSanitizer.sanitize(name)
    return .file(url: destination, name: safeName, mimeType: mimeType, size: size)
  }

  private static func loadURL(_ provider: NSItemProvider) async -> URL? {
    await withCheckedContinuation { continuation in
      _ = provider.loadObject(ofClass: URL.self) { url, _ in
        continuation.resume(returning: url)
      }
    }
  }

  private static func loadText(_ provider: NSItemProvider) async -> String? {
    await withCheckedContinuation { continuation in
      _ = provider.loadObject(ofClass: String.self) { text, _ in
        continuation.resume(returning: text)
      }
    }
  }
}

extension Array where Element == SharedItem {
  public var totalSize: UInt64 { reduce(0) { $0 + $1.size } }
}
