// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit

class TabWKDownloadHandler: NSObject, WKDownloadDelegate {
  var didCreateDownload: (Download) -> Void
  var didFinishDownload: (Download, Error?) -> Void

  private var pendingDownload: WebKitDownload?

  init(
    didCreateDownload: @escaping (Download) -> Void,
    didFinishDownload: @escaping (Download, Error?) -> Void
  ) {
    self.didCreateDownload = didCreateDownload
    self.didFinishDownload = didFinishDownload
  }

  public func download(
    _ download: WKDownload,
    decideDestinationUsing response: URLResponse,
    suggestedFilename: String,
    completionHandler: @escaping @MainActor (URL?) -> Void
  ) {
    let pendingDownload = WebKitDownload(
      response: response,
      suggestedFileName: suggestedFilename,
      download: download,
      downloadDecisionHandler: completionHandler
    )

    self.pendingDownload = pendingDownload

    didCreateDownload(pendingDownload)
  }

  public func download(
    _ download: WKDownload,
    respondTo challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return (.performDefaultHandling, nil)
  }

  @MainActor
  public func downloadDidFinish(_ download: WKDownload) {
    guard let pendingDownload else { return }
    didFinishDownload(pendingDownload, nil)
  }

  @MainActor
  public func download(_ download: WKDownload, didFailWithError error: Error, resumeData: Data?) {
    guard let pendingDownload else { return }
    didFinishDownload(pendingDownload, error)
  }
}
