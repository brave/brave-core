// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import BraveTor
import Foundation
import WebKit

/// Adopt WebKit's existing network load; never recreate it in a native URLSession.
// Download's legacy synchronous methods enter the main actor explicitly below.
// Stored transfer state and WebKit callbacks are confined to that actor.
final class TorDownload: Download, WKDownloadDelegate, @unchecked Sendable {
  @MainActor private let task: WKDownload
  @MainActor private let activityID = UUID()
  @MainActor private var destinationDecision: (@MainActor (URL?) -> Void)?
  @MainActor private var progressObservation: NSKeyValueObservation?
  @MainActor private var ready: (@MainActor (TorDownload) -> Void)?
  @MainActor private var isCancelling = false
  @MainActor private var finished: (@MainActor (TorDownload, Error?) -> Void)?

  @MainActor
  init(
    task: WKDownload, originalURL: URL,
    ready: @escaping @MainActor (TorDownload) -> Void,
    finished: @escaping @MainActor (TorDownload, Error?) -> Void
  ) {
    self.task = task
    self.ready = ready
    self.finished = finished
    super.init(suggestedFilename: "download", originalURL: originalURL,
               redirectedURL: nil, originatingHost: originalURL.host ?? "")
    task.delegate = self
    progressObservation = task.progress.observe(\.completedUnitCount) { [weak self] _, _ in
      Task { @MainActor [weak self] in self?.updateProgress() }
    }
  }

  @MainActor
  static func safeFilename(_ suggested: String) -> String {
    let leaf = suggested.replacingOccurrences(of: "\\", with: "/")
      .split(separator: "/").last.map(String.init) ?? "download"
    var name = ""
    for scalar in leaf.unicodeScalars where !scalar.properties.isBidiControl
      && !CharacterSet.controlCharacters.contains(scalar) && scalar != ":" {
      if name.utf8.count + String(scalar).utf8.count > 180 { break }
      name.unicodeScalars.append(scalar)
    }
    name = name.trimmingCharacters(in: .whitespacesAndNewlines)
    return name.isEmpty || name == "." || name == ".." ? "download" : name
  }

  nonisolated override func startDownloadToLocalFileAtPath(_ path: String) {
    MainActor.assumeIsolated {
      guard !isStarted, !isComplete, !isCancelling, let decide = destinationDecision else { return }
      super.startDownloadToLocalFileAtPath(path)
      destinationDecision = nil
      TorDownloadActivity.shared.begin(activityID)
      decide(URL(fileURLWithPath: path))
    }
  }

  nonisolated override func cancel() {
    MainActor.assumeIsolated { cancelTransfer() }
  }
  // Resume data is deliberately discarded: never restart through another network stack.
  nonisolated override func pause() { cancel() }
  nonisolated override func resume() {}

  @MainActor
  func cancelTransfer() {
    guard !isComplete, !isCancelling else { return }
    isCancelling = true
    let decide = destinationDecision
    destinationDecision = nil
    decide?(nil)
    // Wait for WebKit to stop writing before removing any partial file.
    task.cancel { [self] _ in complete(URLError(.cancelled)) }
  }

  @MainActor
  private func updateProgress() {
    guard !isComplete else { return }
    bytesDownloaded = task.progress.completedUnitCount
    totalBytesExpected = task.progress.totalUnitCount > 0 ? task.progress.totalUnitCount : nil
    delegate?.downloadDidUpgradeProgress(self)
  }

  @MainActor
  private func complete(_ error: Error?) {
    guard !isComplete else { return }
    updateProgress()
    isComplete = true
    progressObservation = nil
    TorDownloadActivity.shared.end(activityID)
    if error != nil, let destinationURL { try? FileManager.default.removeItem(at: destinationURL) }
    let callback = finished
    ready = nil
    finished = nil
    callback?(self, error)
  }

  @MainActor
  func download(
    _ download: WKDownload, decideDestinationUsing response: URLResponse,
    suggestedFilename: String, completionHandler: @escaping @MainActor (URL?) -> Void
  ) {
    guard !isComplete, let url = response.url, TorDestination.isAllowed(url),
      destinationDecision == nil, let ready
    else { completionHandler(nil); cancelTransfer(); return }
    redirectedURL = url
    filename = Self.safeFilename(suggestedFilename)
    mimeType = response.mimeType ?? "application/octet-stream"
    totalBytesExpected = response.expectedContentLength >= 0 ? response.expectedContentLength : nil
    destinationDecision = completionHandler
    self.ready = nil
    ready(self)
  }

  @MainActor
  func download(
    _ download: WKDownload, willPerformHTTPRedirection response: HTTPURLResponse,
    newRequest request: URLRequest,
    decisionHandler: @escaping @MainActor (WKDownload.RedirectPolicy) -> Void
  ) {
    guard !isComplete, let url = request.url, TorDestination.isAllowed(url) else {
      decisionHandler(.cancel)
      cancelTransfer()
      return
    }
    redirectedURL = url
    decisionHandler(.allow)
  }

  @MainActor
  func downloadDidFinish(_ download: WKDownload) { complete(isCancelling ? URLError(.cancelled) : nil) }
  @MainActor
  func download(
    _ download: WKDownload, didReceive challenge: URLAuthenticationChallenge,
    completionHandler: @escaping @MainActor (URLSession.AuthChallengeDisposition, URLCredential?) -> Void
  ) {
    completionHandler(challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust
      ? .performDefaultHandling : .cancelAuthenticationChallenge, nil)
  }
  @MainActor
  func download(_ download: WKDownload, didFailWithError error: Error, resumeData: Data?) {
    let decide = destinationDecision
    destinationDecision = nil
    decide?(nil)
    complete(isCancelling ? URLError(.cancelled) : error)
  }
}
