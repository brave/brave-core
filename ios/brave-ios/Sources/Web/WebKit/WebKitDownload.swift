// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

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
