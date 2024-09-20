// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import PassKit
import Shared
import WebKit

extension BrowserViewController: WKDownloadDelegate {

  public func download(
    _ download: WKDownload,
    decideDestinationUsing response: URLResponse,
    suggestedFilename: String
  ) async -> URL? {

    if let httpResponse = response as? HTTPURLResponse {
      if httpResponse.mimeType != MIMEType.passbook {
        let shouldDownload = await downloadAlert(
          download,
          response: response,
          suggestedFileName: suggestedFilename
        )
        if !shouldDownload {
          return nil
        }
      }
    }

    let temporaryDir = NSTemporaryDirectory()
    let fileName = temporaryDir + "/" + suggestedFilename
    let url = URL(fileURLWithPath: fileName)

    // WKDownload will fail with a -3000 error code if the file already exists at the given path
    if await AsyncFileManager.default.fileExists(atPath: url.path(percentEncoded: false)) {
      try? await AsyncFileManager.default.removeItem(at: url)
    }

    let pendingDownload = WebKitDownload(
      fileURL: url,
      response: response,
      suggestedFileName: suggestedFilename,
      download: download,
      downloadQueue: downloadQueue
    )

    downloadQueue.enqueue(pendingDownload)

    return url
  }

  public func download(
    _ download: WKDownload,
    decidedPolicyForHTTPRedirection response: HTTPURLResponse,
    newRequest request: URLRequest
  ) async -> WKDownload.RedirectPolicy {
    return .allow
  }

  public func download(
    _ download: WKDownload,
    respondTo challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return (.performDefaultHandling, nil)
  }

  @MainActor
  public func downloadDidFinish(_ download: WKDownload) {
    guard
      let downloadInfo = downloadQueue.downloads.compactMap({ $0 as? WebKitDownload }).first(
        where: { $0.download == download })
    else {
      return
    }

    let response = URLResponse(
      url: downloadInfo.fileURL,
      mimeType: downloadInfo.response.mimeType,
      expectedContentLength: Int(downloadInfo.response.expectedContentLength),
      textEncodingName: downloadInfo.response.textEncodingName
    )

    if downloadInfo.response.mimeType == MIMEType.passbook {
      downloadQueue.download(downloadInfo, didFinishDownloadingTo: downloadInfo.fileURL)
      if let passbookHelper = OpenPassBookHelper(
        request: nil,
        response: response,
        canShowInWebView: false,
        forceDownload: false,
        browserViewController: self
      ) {
        Task {
          await passbookHelper.open()
          try await AsyncFileManager.default.removeItem(at: downloadInfo.fileURL)
        }
      }
      return
    }

    // Handle non-passbook downloads the same as HTTPDownload
    let filename = downloadInfo.filename
    let location = downloadInfo.fileURL
    let temporaryLocation = FileManager.default.temporaryDirectory
      .appending(component: "\(filename)-\(location.lastPathComponent)")
    try? FileManager.default.moveItem(at: location, to: temporaryLocation)
    Task {
      do {
        let destination = try await downloadInfo.uniqueDownloadPathForFilename(filename)
        try await AsyncFileManager.default.moveItem(at: temporaryLocation, to: destination)
        downloadQueue.download(downloadInfo, didFinishDownloadingTo: destination)
      } catch {
        downloadQueue.download(downloadInfo, didCompleteWithError: error)
      }
    }
  }

  @MainActor
  public func download(_ download: WKDownload, didFailWithError error: Error, resumeData: Data?) {
    guard
      let downloadInfo = downloadQueue.downloads.compactMap({ $0 as? WebKitDownload }).first(
        where: { $0.download == download })
    else {
      return
    }

    downloadQueue.download(downloadInfo, didCompleteWithError: error)

    // display an error
    let alertController = UIAlertController(
      title: Strings.unableToAddPassErrorTitle,
      message: Strings.unableToAddPassErrorMessage,
      preferredStyle: .alert
    )
    alertController.addAction(
      UIAlertAction(title: Strings.unableToAddPassErrorDismiss, style: .cancel) { (action) in
        // Do nothing.
      }
    )
    present(alertController, animated: true, completion: nil)
  }

  @MainActor
  private func downloadAlert(
    _ download: WKDownload,
    response: URLResponse,
    suggestedFileName: String
  ) async -> Bool {
    // Only download if there is a valid host
    guard let host = download.originalRequest?.url?.host() else {
      return false
    }

    // Never present the download alert on a tab that isn't visible
    guard let webView = download.webView, let tab = tabManager.tabForWebView(webView),
      tab === tabManager.selectedTab
    else {
      return false
    }

    let filename = HTTPDownload.stripUnicode(fromFilename: suggestedFileName)
    let totalBytesExpected =
      response.expectedContentLength > 0 ? response.expectedContentLength : nil

    let expectedSize =
      totalBytesExpected != nil
      ? ByteCountFormatter.string(fromByteCount: totalBytesExpected!, countStyle: .file)
      : nil

    let title = "\(filename) - \(host)"

    var downloadActionText = Strings.download
    if let expectedSize = expectedSize {
      downloadActionText += " (\(expectedSize))"
    }

    return await withCheckedContinuation { continuation in
      let downloadAlert = UIAlertController(
        title: title,
        message: nil,
        preferredStyle: .actionSheet
      )

      downloadAlert.addAction(
        UIAlertAction(title: downloadActionText, style: .default) { _ in
          continuation.resume(returning: true)
        }
      )

      downloadAlert.addAction(
        UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel) { _ in
          continuation.resume(returning: false)
        }
      )

      downloadAlert.popoverPresentationController?.do {
        $0.sourceView = view
        $0.sourceRect = CGRect(x: view.bounds.midX, y: view.bounds.maxY - 16, width: 0, height: 0)
        $0.permittedArrowDirections = []
      }

      present(downloadAlert, animated: true, completion: nil)
    }
  }
}
