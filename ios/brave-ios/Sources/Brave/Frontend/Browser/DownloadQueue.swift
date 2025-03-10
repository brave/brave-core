// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import WebKit

private let downloadOperationQueue = OperationQueue()

protocol DownloadDelegate {
  func download(_ download: Download, didCompleteWithError error: Error?)
  func downloadDidUpgradeProgress(_ download: Download)
  func download(_ download: Download, didFinishDownloadingTo location: URL)
}

class Download: NSObject {
  var delegate: DownloadDelegate?

  fileprivate(set) var filename: String
  fileprivate(set) var mimeType: String
  fileprivate(set) var originalURL: URL?
  fileprivate(set) var destinationURL: URL?

  fileprivate(set) var isStarted: Bool = false
  fileprivate(set) var isComplete = false

  fileprivate(set) var totalBytesExpected: Int64?
  fileprivate(set) var bytesDownloaded: Int64

  init(
    suggestedFilename: String,
    originalURL: URL?,
    mimeType: String? = nil
  ) {
    self.filename = suggestedFilename
    self.originalURL = originalURL
    self.mimeType = mimeType ?? "application/octet-stream"

    self.bytesDownloaded = 0

    super.init()
  }

  func startDownloadToLocalFileAtPath(_ path: String) {
    if isStarted {
      return
    }
    isStarted = true
    let destination = URL(fileURLWithPath: path)
    self.destinationURL = destination
  }
  func cancel() {}
  func pause() {}
  func resume() {}

  static func uniqueDownloadPathForFilename(_ filename: String) async throws -> URL {
    let downloadsPath = try await AsyncFileManager.default.downloadsPath()
    let basePath = downloadsPath.appending(path: filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while await AsyncFileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = downloadsPath.appending(path: proposedFilenameWithoutExtension)
        .appendingPathExtension(fileExtension)
    }

    return proposedPath
  }

  // Used to avoid name spoofing using Unicode RTL char to change file extension
  public static func stripUnicode(fromFilename string: String) -> String {
    let validFilenameSet = CharacterSet(charactersIn: ":/")
      .union(.newlines)
      .union(.controlCharacters)
      .union(.illegalCharacters)
    return string.components(separatedBy: validFilenameSet).joined()
  }
}

protocol DownloadQueueDelegate {
  func downloadQueue(_ downloadQueue: DownloadQueue, didStartDownload download: Download)
  func downloadQueue(
    _ downloadQueue: DownloadQueue,
    didDownloadCombinedBytes combinedBytesDownloaded: Int64,
    combinedTotalBytesExpected: Int64?
  )
  func downloadQueue(
    _ downloadQueue: DownloadQueue,
    download: Download,
    didFinishDownloadingTo location: URL
  )
  func downloadQueue(_ downloadQueue: DownloadQueue, didCompleteWithError error: Error?)
}

class DownloadQueue {
  var downloads: [Download]

  var delegate: DownloadQueueDelegate?

  var isEmpty: Bool {
    return downloads.isEmpty
  }

  fileprivate var combinedBytesDownloaded: Int64 = 0
  fileprivate var combinedTotalBytesExpected: Int64?
  fileprivate var lastDownloadError: Error?

  init() {
    self.downloads = []
  }

  func enqueue(_ download: Download) {
    // Clear the download stats if the queue was empty at the start.
    if downloads.isEmpty {
      combinedBytesDownloaded = 0
      combinedTotalBytesExpected = 0
      lastDownloadError = nil
    }

    downloads.append(download)
    download.delegate = self

    if let totalBytesExpected = download.totalBytesExpected, combinedTotalBytesExpected != nil {
      combinedTotalBytesExpected! += totalBytesExpected
    } else {
      combinedTotalBytesExpected = nil
    }

    download.resume()
    delegate?.downloadQueue(self, didStartDownload: download)
  }

  func cancelAll() {
    for download in downloads where !download.isComplete {
      download.cancel()
    }
  }

  func pauseAll() {
    for download in downloads where !download.isComplete {
      download.pause()
    }
  }

  func resumeAll() {
    for download in downloads where !download.isComplete {
      download.resume()
    }
  }
}

extension DownloadQueue: DownloadDelegate {
  func download(_ download: Download, didCompleteWithError error: Error?) {
    guard let error = error, let index = downloads.firstIndex(of: download) else {
      return
    }

    lastDownloadError = error
    downloads.remove(at: index)

    if downloads.isEmpty {
      delegate?.downloadQueue(self, didCompleteWithError: lastDownloadError)
    }
  }

  func downloadDidUpgradeProgress(_ download: Download) {
    combinedBytesDownloaded = downloads.reduce(into: 0) { $0 += $1.bytesDownloaded }
    combinedTotalBytesExpected = downloads.reduce(into: 0) { $0 += ($1.totalBytesExpected ?? 0) }
    delegate?.downloadQueue(
      self,
      didDownloadCombinedBytes: combinedBytesDownloaded,
      combinedTotalBytesExpected: combinedTotalBytesExpected
    )
  }

  func download(_ download: Download, didFinishDownloadingTo location: URL) {
    guard let index = downloads.firstIndex(of: download) else {
      return
    }

    downloads.remove(at: index)
    delegate?.downloadQueue(self, download: download, didFinishDownloadingTo: location)

    NotificationCenter.default.post(name: .fileDidDownload, object: location)

    if downloads.isEmpty {
      delegate?.downloadQueue(self, didCompleteWithError: lastDownloadError)
    }
  }
}

class WebKitDownload: Download {
  weak var download: WKDownload?

  private weak var webView: WKWebView?
  private var downloadResumeData: Data?
  private var completedUnitCountObserver: NSKeyValueObservation?
  private var downloadDecisionHandler: ((URL?) -> Void)?

  init(
    response: URLResponse,
    suggestedFileName: String,
    download: WKDownload,
    downloadDecisionHandler: @escaping (URL?) -> Void
  ) {
    self.download = download
    self.webView = download.webView
    self.downloadDecisionHandler = downloadDecisionHandler

    super.init(
      suggestedFilename: suggestedFileName,
      originalURL: download.originalRequest?.url,
      mimeType: response.mimeType
    )

    self.originalURL = download.originalRequest?.url

    self.totalBytesExpected =
      response.expectedContentLength > 0 ? response.expectedContentLength : nil

    completedUnitCountObserver = download.progress.observe(
      \.completedUnitCount,
      changeHandler: { [weak self] progress, value in
        guard let self = self else { return }

        self.bytesDownloaded = progress.completedUnitCount
        self.totalBytesExpected = progress.totalUnitCount
        self.delegate?.downloadDidUpgradeProgress(self)
      }
    )
  }

  override func startDownloadToLocalFileAtPath(_ path: String) {
    super.startDownloadToLocalFileAtPath(path)
    downloadDecisionHandler?(destinationURL)
    downloadDecisionHandler = nil
  }

  deinit {
    if !isStarted {
      downloadDecisionHandler?(nil)
      downloadDecisionHandler = nil
      return
    }
  }

  override func cancel() {
    if !isStarted {
      downloadDecisionHandler?(nil)
      downloadDecisionHandler = nil
      return
    }
    download?.cancel({ [weak self] data in
      guard let self = self else { return }
      if let data = data {
        self.downloadResumeData = data
      }

      self.delegate?.download(self, didCompleteWithError: nil)
    })
  }

  override func pause() {

  }

  override func resume() {
    if let downloadResumeData = downloadResumeData {
      webView?.resumeDownload(
        fromResumeData: downloadResumeData,
        completionHandler: { [weak self] download in
          guard let self = self else { return }
          self.download = download
          self.bytesDownloaded = download.progress.completedUnitCount
          self.totalBytesExpected = download.progress.totalUnitCount
          self.delegate?.downloadDidUpgradeProgress(self)
        }
      )
    }
  }
}
