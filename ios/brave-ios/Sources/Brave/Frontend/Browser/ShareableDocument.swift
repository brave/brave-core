// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Shared
import Web

/// A tab's committed non-HTML contents (such as a PDF document) that can be downloaded on-demand
/// and handed to a share sheet as a file.
///
/// Instances are created by `ShareableDocumentTabHelper` for each main frame navigation that
/// commits a shareable document and are discarded when the tab navigates elsewhere.
///
/// Each document is stored in its own unique directory so concurrent shares never collide. The
/// directory is removed when the document is deallocated, so callers should retain it for as long
/// as the file is needed (e.g. until the share sheet is dismissed).
@MainActor
final class ShareableDocument {
  /// The committed URL of the document in the tab
  let url: URL

  private weak var tab: (any TabState)?
  private let contentDisposition: String?
  private let directoryURL: URL

  private var downloadTask: Task<URL, Error>?

  init(tab: some TabState, url: URL, contentDisposition: String?) {
    self.tab = tab
    self.url = url
    self.contentDisposition = contentDisposition
    self.directoryURL = Self.rootDirectory.appending(
      path: UUID().uuidString,
      directoryHint: .isDirectory
    )
  }

  deinit {
    let directoryURL = directoryURL
    Task.detached(priority: .utility) {
      try? await AsyncFileManager.default.removeItem(at: directoryURL)
    }
  }

  /// Whether or not the tab is currently displaying contents that should be downloaded in order to
  /// be shared as a file.
  static func isShareable(_ tab: some TabState) -> Bool {
    guard let mimeType = tab.contentsMimeType, !mimeType.isEmpty, !mimeType.isKindOfHTML,
      let url = tab.lastCommittedURL,
      url.isWebPage() || url.isFileURL || url.scheme == "blob"
    else {
      return false
    }
    return true
  }

  /// Returns the local file URL of the document, downloading it first if needed.
  ///
  /// Concurrent calls share the same download. The downloaded file is reused for subsequent calls
  /// and a failed download will be retried on the next call.
  ///
  /// Throws `CancellationError` if the download is cancelled via `cancel()`
  func fileURL() async throws -> URL {
    if let downloadTask {
      return try await downloadTask.value
    }
    let task = Task<URL, Error> { @MainActor in
      try await download()
    }
    downloadTask = task
    do {
      return try await task.value
    } catch {
      if downloadTask == task {
        downloadTask = nil
      }
      throw error
    }
  }

  /// Cancels any in-flight download
  func cancel() {
    downloadTask?.cancel()
    downloadTask = nil
  }

  // MARK: -

  private func download() async throws -> URL {
    guard let tab, tab.lastCommittedURL == url else {
      throw URLError(.cancelled)
    }
    await Self.removeStaleDocumentsIfNeeded()
    try Task.checkCancellation()

    let fileURL = directoryURL.appending(
      path: tab.suggestedFilenameForCurrentPage(contentDisposition: contentDisposition)
    )
    try await AsyncFileManager.default.createDirectory(
      at: directoryURL,
      withIntermediateDirectories: true,
      attributes: nil
    )
    do {
      try await tab.downloadCurrentPage(to: fileURL)
    } catch {
      try? await AsyncFileManager.default.removeItem(at: fileURL)
      throw error
    }
    return fileURL
  }

  private static let rootDirectory = FileManager.default.temporaryDirectory.appending(
    path: "ShareableDocuments",
    directoryHint: .isDirectory
  )

  /// Removes any documents left behind by a previous launch (e.g. if the app was terminated while a
  /// share sheet was presented). Only runs once per launch.
  private static let removeStaleDocumentsTask = Task { @MainActor in
    try? await AsyncFileManager.default.removeItem(at: rootDirectory)
  }

  private static func removeStaleDocumentsIfNeeded() async {
    await removeStaleDocumentsTask.value
  }
}
