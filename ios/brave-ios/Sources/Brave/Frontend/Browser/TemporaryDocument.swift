// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Shared

private let temporaryDocumentOperationQueue = OperationQueue()

class TemporaryDocument: NSObject {
  private weak var tab: Tab?
  private let request: URLRequest
  private let filename: String

  fileprivate var session: URLSession?

  fileprivate var downloadTask: URLSessionDownloadTask?
  private var localFileURL: URL?
  private var pendingContinuation: CheckedContinuation<URL, Never>?
  private var pendingTask: Task<URL, Never>?

  init(preflightResponse: URLResponse, request: URLRequest, tab: Tab) {
    self.request = request
    self.filename = preflightResponse.suggestedFilename ?? "unknown"
    self.tab = tab

    super.init()

    self.session = URLSession(
      configuration: .default,
      delegate: self,
      delegateQueue: temporaryDocumentOperationQueue
    )
  }

  deinit {
    downloadTask?.cancel()
    downloadTask = nil

    // Delete the temp file.
    if let url = localFileURL {
      Task {
        try await AsyncFileManager.default.removeItem(at: url)
      }
    }
  }

  /// Get a deferred url to the locally stored file.
  /// - Note: If an error occurs, the original web url will be returned
  func getURL() async -> URL {
    if let url = localFileURL {
      return url
    }

    if let task = pendingTask {
      return await task.value
    }

    let task = Task<URL, Never> { @MainActor in
      return await withCheckedContinuation { continuation in
        pendingContinuation = continuation

        downloadTask = session?.downloadTask(with: request)
        downloadTask?.resume()

        if let tab = self.tab, let url = request.url {
          // Calls `onDocumentDownloaded` on completion
          ResourceDownloadScriptHandler.downloadResource(for: tab, url: url)
        } else {
          Task {
            await onDocumentDownloaded(document: nil, error: nil)
          }
        }
      }
    }
    pendingTask = task
    return await task.value
  }

  /// A callback available when there is a download response
  @MainActor func onDocumentDownloaded(document: DownloadedResourceResponse?, error: Error?) async {
    // Store the blob/data in a local temporary file.
    if let document = document, let data = document.data, !data.isEmpty {
      let tempDirectory = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(
        "TempDocs"
      )
      let url = tempDirectory.appending(path: filename)

      do {
        try await AsyncFileManager.default.createDirectory(
          at: tempDirectory,
          withIntermediateDirectories: true,
          attributes: nil
        )

        // Delete the file if there is already one. We will replace it with a new file
        if await AsyncFileManager.default.fileExists(atPath: url.absoluteString) {
          try await AsyncFileManager.default.removeItem(at: url)
        }

        await AsyncFileManager.default.createFile(
          atPath: url.path(percentEncoded: false),
          contents: data
        )

        localFileURL = url
        pendingContinuation?.resume(returning: url)
        pendingContinuation = nil
        return
      } catch {
        // let the error pass through to the below handler..
      }
    }

    // If we encounter an error downloading the temp file, just return with the
    // original remote URL so it can still be shared as a web URL.
    if let url = request.url {
      pendingContinuation?.resume(returning: url)
      pendingContinuation = nil
    } else {
      assertionFailure("No URL found to return which is undefined behaviour")
      // Continuation must run regardless
      pendingContinuation?.resume(returning: NSURL() as URL)
      pendingContinuation = nil
    }
  }
}

extension TemporaryDocument: URLSessionTaskDelegate, URLSessionDownloadDelegate {
  func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
    // If we encounter an error downloading the temp file, just return with the
    // original remote URL so it can still be shared as a web URL.
    if error != nil, let remoteURL = request.url {
      pendingContinuation?.resume(returning: remoteURL)
      pendingContinuation = nil
    }
  }

  func urlSession(
    _ session: URLSession,
    downloadTask: URLSessionDownloadTask,
    didFinishDownloadingTo location: URL
  ) {
    // This method will delete the downloaded file immediately after this delegate method returns
    // so we must synchonously move the file to a temporary directory first before processing it
    // on a different thread since the Task will execute after this method returns
    let temporaryLocation = FileManager.default.temporaryDirectory
      .appending(component: "\(filename)-\(location.lastPathComponent)")
    try? FileManager.default.moveItem(at: location, to: temporaryLocation)
    Task {
      let tempDirectory = URL(fileURLWithPath: NSTemporaryDirectory()).appending(path: "TempDocs")
      let url = tempDirectory.appending(path: filename)

      try? await AsyncFileManager.default.createDirectory(
        at: tempDirectory,
        withIntermediateDirectories: true,
        attributes: nil
      )
      try? await AsyncFileManager.default.removeItem(at: url)

      do {
        try await AsyncFileManager.default.moveItem(at: temporaryLocation, to: url)
        localFileURL = url
        pendingContinuation?.resume(returning: url)
        pendingContinuation = nil
      } catch {
        // If we encounter an error downloading the temp file, just return with the
        // original remote URL so it can still be shared as a web URL.
        if let remoteURL = request.url {
          pendingContinuation?.resume(returning: remoteURL)
          pendingContinuation = nil
        }
      }
    }
  }
}
