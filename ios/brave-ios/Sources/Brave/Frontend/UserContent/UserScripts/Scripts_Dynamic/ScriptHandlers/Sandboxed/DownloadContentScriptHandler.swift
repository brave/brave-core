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
  private weak var browserViewController: BrowserViewController?
  private static var blobUrlForDownload: URL?

  init(browserController: BrowserViewController) {
    self.browserViewController = browserController
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

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    do {
      guard let body = message.body as? [String: Any?] else {
        return
      }

      let info = try JSONDecoder().decode(
        BlobDownloadInfo.self,
        from: JSONSerialization.data(withJSONObject: body)
      )
      guard let _ = Bytes.decodeBase64(info.base64String) else {
        return
      }

      defer {
        Self.blobUrlForDownload = nil
      }

      guard let requestedUrl = Self.blobUrlForDownload else {
        Logger.module.error("\(Self.scriptName): no url was requested")
        return
      }

      guard requestedUrl == info.url else {
        Logger.module.error("\(Self.scriptName): URL mismatch")
        return
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
  }
}
