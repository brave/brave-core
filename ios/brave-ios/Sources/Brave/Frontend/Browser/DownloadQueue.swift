/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit

private let downloadOperationQueue = OperationQueue()

protocol DownloadDelegate {
  func download(_ download: Download, didCompleteWithError error: Error?)
  func download(_ download: Download, didDownloadBytes bytesDownloaded: Int64)
  func download(_ download: Download, didFinishDownloadingTo location: URL)
}

class Download: NSObject {
  var delegate: DownloadDelegate?

  fileprivate(set) var filename: String
  fileprivate(set) var mimeType: String

  fileprivate(set) var isComplete = false

  fileprivate(set) var totalBytesExpected: Int64?
  fileprivate(set) var bytesDownloaded: Int64

  override init() {
    self.filename = "unknown"
    self.mimeType = "application/octet-stream"

    self.bytesDownloaded = 0

    super.init()
  }

  func cancel() {}
  func pause() {}
  func resume() {}

  fileprivate func uniqueDownloadPathForFilename(_ filename: String) throws -> URL {
    let downloadsPath = try FileManager.default.downloadsPath()
    let basePath = downloadsPath.appendingPathComponent(filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension = !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while FileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = downloadsPath.appendingPathComponent(proposedFilenameWithoutExtension).appendingPathExtension(fileExtension)
    }

    return proposedPath
  }
}

class HTTPDownload: Download {
  let preflightResponse: URLResponse
  let request: URLRequest

  var state: URLSessionTask.State {
    return task?.state ?? .suspended
  }

  fileprivate(set) var session: URLSession?
  fileprivate(set) var task: URLSessionDownloadTask?
  fileprivate(set) var cookieStore: WKHTTPCookieStore

  private var resumeData: Data?

  // Used to avoid name spoofing using Unicode RTL char to change file extension
  public static func stripUnicode(fromFilename string: String) -> String {
    let validFilenameSet = CharacterSet(charactersIn: ":/")
      .union(.newlines)
      .union(.controlCharacters)
      .union(.illegalCharacters)
    return string.components(separatedBy: validFilenameSet).joined()
  }

  init(cookieStore: WKHTTPCookieStore, preflightResponse: URLResponse, request: URLRequest) {
    self.cookieStore = cookieStore
    self.preflightResponse = preflightResponse
    self.request = request

    super.init()

    if let filename = preflightResponse.suggestedFilename {
      self.filename = HTTPDownload.stripUnicode(fromFilename: filename)
    }

    if let mimeType = preflightResponse.mimeType {
      self.mimeType = mimeType
    }

    self.totalBytesExpected = preflightResponse.expectedContentLength > 0 ? preflightResponse.expectedContentLength : nil

    self.session = URLSession(configuration: .ephemeral, delegate: self, delegateQueue: downloadOperationQueue)
    self.task = session?.downloadTask(with: request)
  }

  override func cancel() {
    task?.cancel()
  }

  override func pause() {
    task?.cancel(byProducingResumeData: { resumeData in
      self.resumeData = resumeData
    })
  }

  override func resume() {
    cookieStore.getAllCookies { [self] cookies in
      cookies.forEach { cookie in
        session?.configuration.httpCookieStorage?.setCookie(cookie)
      }

      guard let resumeData = self.resumeData else {
        self.task?.resume()
        return
      }
      self.task = session?.downloadTask(withResumeData: resumeData)
      self.task?.resume()
    }
  }
}

extension HTTPDownload: URLSessionTaskDelegate, URLSessionDownloadDelegate {
  func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
    // Don't bubble up cancellation as an error if the
    // error is `.cancelled` and we have resume data.
    if let urlError = error as? URLError,
      urlError.code == .cancelled,
      resumeData != nil {
      return
    }

    delegate?.download(self, didCompleteWithError: error)
  }

  func urlSession(_ session: URLSession, downloadTask: URLSessionDownloadTask, didWriteData bytesWritten: Int64, totalBytesWritten: Int64, totalBytesExpectedToWrite: Int64) {
    bytesDownloaded = totalBytesWritten
    totalBytesExpected = totalBytesExpectedToWrite

    delegate?.download(self, didDownloadBytes: bytesWritten)
  }

  func urlSession(_ session: URLSession, downloadTask: URLSessionDownloadTask, didFinishDownloadingTo location: URL) {
    do {
      let destination = try uniqueDownloadPathForFilename(filename)
      try FileManager.default.moveItem(at: location, to: destination)
      isComplete = true
      delegate?.download(self, didFinishDownloadingTo: destination)
    } catch {
      delegate?.download(self, didCompleteWithError: error)
    }
  }
}

protocol DownloadQueueDelegate {
  func downloadQueue(_ downloadQueue: DownloadQueue, didStartDownload download: Download)
  func downloadQueue(_ downloadQueue: DownloadQueue, didDownloadCombinedBytes combinedBytesDownloaded: Int64, combinedTotalBytesExpected: Int64?)
  func downloadQueue(_ downloadQueue: DownloadQueue, download: Download, didFinishDownloadingTo location: URL)
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

  func download(_ download: Download, didDownloadBytes bytesDownloaded: Int64) {
    combinedBytesDownloaded += bytesDownloaded
    delegate?.downloadQueue(self, didDownloadCombinedBytes: combinedBytesDownloaded, combinedTotalBytesExpected: combinedTotalBytesExpected)
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
