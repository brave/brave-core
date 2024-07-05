// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import MobileCoreServices
import Shared
import UniformTypeIdentifiers
import WebKit
import os.log

private struct BlobDownloadInfo: Codable {
  let url: URL
  let mimeType: String
  let size: Int64
  let base64String: String
}

class DownloadContentScriptHandler: TabContentScript {
  private weak var tab: Tab?
  private weak var browserViewController: BrowserViewController?
  private static var blobUrlForDownload: URL?

  init(browserController: BrowserViewController, tab: Tab) {
    self.browserViewController = browserController
    self.tab = tab
  }

  static let scriptName = "DownloadContentScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "downloadContentScript"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  static func downloadBlob(url: URL, tab: Tab) -> Bool {
    let safeUrl = url.absoluteString.replacingOccurrences(of: "'", with: "%27")
    guard url.scheme == "blob" else {
      return false
    }

    tab.webView?.evaluateSafeJavaScript(
      functionName: "window.__firefox__.download",
      args: [safeUrl],
      contentWorld: scriptSandbox
    )
    return true
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    do {
      guard let body = message.body as? [String: Any?] else {
        return (nil, nil)
      }

      let info = try JSONDecoder().decode(
        BlobDownloadInfo.self,
        from: JSONSerialization.data(withJSONObject: body)
      )
      guard let _ = Bytes.decodeBase64(info.base64String) else {
        return (nil, nil)
      }

      defer {
        browserViewController?.pendingDownloadWebView = nil
        Self.blobUrlForDownload = nil
      }

      guard let requestedUrl = Self.blobUrlForDownload else {
        Logger.module.error("\(Self.scriptName): no url was requested")
        return (nil, nil)
      }

      guard requestedUrl == info.url else {
        Logger.module.error("\(Self.scriptName): URL mismatch")
        return (nil, nil)
      }

      var filename = info.url.absoluteString.components(separatedBy: "/").last ?? "data"
      if filename.isEmpty {
        filename = "data"
      }

      if !filename.contains(".") {
        if let fileExtension = UTType(mimeType: info.mimeType)?.preferredFilenameExtension {
          filename += ".\(fileExtension)"
        }
      }

      //      let response = DownloadedResourceResponse(statusCode: 200, data: data)
      //      tab?.temporaryDocument?.onDocumentDownloaded(document: response, error: nil)
      //
      //      let download = Download(filename: filename, mimeType: mimeType, size: size, data: data)
      //      browserViewController?.downloadQueue.enqueue(download)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }

    return (nil, nil)
  }
}
