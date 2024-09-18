// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import Foundation
import WebKit

struct DownloadedResourceResponse: Decodable {
  let statusCode: Int
  let data: Data?

  static func from(message: WKScriptMessage) throws -> DownloadedResourceResponse? {
    if !JSONSerialization.isValidJSONObject(message.body) {
      return nil
    }

    let data = try JSONSerialization.data(withJSONObject: message.body, options: .prettyPrinted)
    return try JSONDecoder().decode(DownloadedResourceResponse.self, from: data)
  }

  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    self.statusCode = try container.decode(Int.self, forKey: .statusCode)
    self.data = Data(base64Encoded: try container.decode(String.self, forKey: .base64Data))
  }

  private enum CodingKeys: String, CodingKey {
    case statusCode
    case base64Data
  }
}

class ResourceDownloadScriptHandler: TabContentScript {
  static let scriptName = "ResourceDownloaderScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  private static let downloadName = "\(scriptName)_\(uniqueID)"
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    Task { @MainActor in
      do {
        let response = try DownloadedResourceResponse.from(message: message)
        await tab.temporaryDocument?.onDocumentDownloaded(document: response, error: nil)
      } catch {
        await tab.temporaryDocument?.onDocumentDownloaded(document: nil, error: error)
      }
      replyHandler(nil, nil)
    }
  }

  static func downloadResource(for tab: Tab, url: URL) {
    tab.webView?.evaluateSafeJavaScript(
      functionName: "window.__firefox__.downloadManager.download",
      args: [url.absoluteString],
      contentWorld: self.scriptSandbox
    ) { _, error in
      if let error = error {
        Task {
          await tab.temporaryDocument?.onDocumentDownloaded(document: nil, error: error)
        }
      }
    }
  }
}
