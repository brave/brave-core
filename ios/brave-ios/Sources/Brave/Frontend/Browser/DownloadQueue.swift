// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Web
import WebKit

private let downloadOperationQueue = OperationQueue()

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
