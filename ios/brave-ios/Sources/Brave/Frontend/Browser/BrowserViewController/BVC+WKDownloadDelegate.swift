// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit

extension BrowserViewController: WKDownloadDelegate {
  public func download(
    _ download: WKDownload,
    decideDestinationUsing response: URLResponse,
    suggestedFilename: String,
    completionHandler: @escaping @MainActor (URL?) -> Void
  ) {
    guard let webView = download.webView, let tab = tabManager[webView] else {
      completionHandler(nil)
      return
    }

    let pendingDownload = WebKitDownload(
      response: response,
      suggestedFileName: suggestedFilename,
      download: download,
      downloadDecisionHandler: completionHandler
    )

    self.tab(tab, didCreateDownload: pendingDownload)
  }

  public func download(
    _ download: WKDownload,
    respondTo challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return (.performDefaultHandling, nil)
  }

  @MainActor
  public func downloadDidFinish(_ download: WKDownload) {
    guard let webView = download.webView, let tab = tabManager[webView],
      let downloadInfo = downloadQueue.downloads.compactMap({ $0 as? WebKitDownload }).first(
        where: { $0.download == download })
    else {
      return
    }
    self.tab(tab, didFinishDownload: downloadInfo, error: nil)
  }

  @MainActor
  public func download(_ download: WKDownload, didFailWithError error: Error, resumeData: Data?) {
    guard let webView = download.webView, let tab = tabManager[webView],
      let downloadInfo = downloadQueue.downloads.compactMap({ $0 as? WebKitDownload }).first(
        where: { $0.download == download })
    else {
      return
    }
    self.tab(tab, didFinishDownload: downloadInfo, error: error)
  }
}
