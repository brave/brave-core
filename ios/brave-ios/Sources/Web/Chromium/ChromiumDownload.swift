// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

class ChromiumDownload: Download, CWVDownloadTaskDelegate {
  var downloadTask: CWVDownloadTask
  var didFinish: (ChromiumDownload, Error?) -> Void

  init(
    downloadTask: CWVDownloadTask,
    didFinish: @escaping (ChromiumDownload, Error?) -> Void
  ) {
    self.downloadTask = downloadTask
    self.didFinish = didFinish

    super.init(
      suggestedFilename: downloadTask.suggestedFileName,
      originalURL: downloadTask.originalURL,
      mimeType: downloadTask.mimeType
    )

    self.bytesDownloaded = downloadTask.receivedBytes
    self.totalBytesExpected = downloadTask.totalBytes

    downloadTask.delegate = self
  }

  override func startDownloadToLocalFileAtPath(_ path: String) {
    super.startDownloadToLocalFileAtPath(path)
    downloadTask.startDownloadToLocalFile(atPath: path)
  }

  override func cancel() {
    super.cancel()
    downloadTask.cancel()
  }

  override func resume() {
    guard let destinationURL else { return }
    downloadTask.startDownloadToLocalFile(atPath: destinationURL.path)
  }

  func downloadTask(_ downloadTask: CWVDownloadTask, didFinishWithError error: (any Error)?) {
    didFinish(self, error)
  }

  func downloadTaskProgressDidChange(_ downloadTask: CWVDownloadTask) {
    totalBytesExpected = downloadTask.totalBytes
    bytesDownloaded = downloadTask.receivedBytes
    delegate?.downloadDidUpgradeProgress(self)
  }
}
