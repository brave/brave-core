// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import PassKit
import Shared

extension BrowserViewController: WKDownloadDelegate {
  
  struct PendingDownload: Hashable, Equatable {
    let fileURL: URL
    let response: URLResponse
    let suggestedFileName: String
  }
  
  public func download(_ download: WKDownload, decideDestinationUsing response: URLResponse, suggestedFilename: String) async -> URL? {
    let temporaryDir = NSTemporaryDirectory()
    let fileName = temporaryDir + "/" + suggestedFilename
    let url = URL(fileURLWithPath: fileName)
    pendingDownloads[download] = PendingDownload(fileURL: url, response: response, suggestedFileName: suggestedFilename)
    return url
  }
  
  public func download(_ download: WKDownload, decidedPolicyForHTTPRedirection response: HTTPURLResponse, newRequest request: URLRequest) async -> WKDownload.RedirectPolicy {
    return .allow
  }
  
  public func download(_ download: WKDownload, respondTo challenge: URLAuthenticationChallenge) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    return (.performDefaultHandling, nil)
  }
  
  @MainActor
  public func downloadDidFinish(_ download: WKDownload) {
    guard let downloadInfo = pendingDownloads[download] else {
      return
    }
    
    pendingDownloads.removeValue(forKey: download)
    
    let response = URLResponse(url: downloadInfo.fileURL,
                               mimeType: downloadInfo.response.mimeType,
                               expectedContentLength: Int(downloadInfo.response.expectedContentLength),
                               textEncodingName: downloadInfo.response.textEncodingName)
    
    if let passbookHelper = OpenPassBookHelper(request: nil, response: response, canShowInWebView: false, forceDownload: false, browserViewController: self) {
      passbookHelper.open()
    }
    
    Task {
      try FileManager.default.removeItem(at: downloadInfo.fileURL)
    }
  }
  
  @MainActor
  public func download(_ download: WKDownload, didFailWithError error: Error, resumeData: Data?) {
    // display an error
    let alertController = UIAlertController(
      title: Strings.unableToAddPassErrorTitle,
      message: Strings.unableToAddPassErrorMessage,
      preferredStyle: .alert)
    alertController.addAction(
      UIAlertAction(title: Strings.unableToAddPassErrorDismiss, style: .cancel) { (action) in
        // Do nothing.
      }
    )
    present(alertController, animated: true, completion: nil)
  }
}
