// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Shared
import WebKit

private let messageOpenInSafari = "openInSafari"
private let messageCertVisitOnce = "certVisitOnce"

extension ErrorPageHelper: TabContentScript {

  static let scriptName = "ErrorPageScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let errorURL = message.frameInfo.request.url,
      let internalUrl = InternalURL(errorURL),
      internalUrl.isErrorPage,
      let originalURL = internalUrl.originalURLFromErrorPage,
      let res = message.body as? [String: String],
      let type = res["type"]
    else { return }

    switch type {
    case messageOpenInSafari:
      UIApplication.shared.open(originalURL, options: [:])
    case messageCertVisitOnce:
      if let cert = CertificateErrorPageHandler.certsFromErrorURL(errorURL)?.first,
        let host = originalURL.host
      {
        let origin = "\(host):\(originalURL.port ?? 443)"
        addCertificate(cert, forOrigin: origin)
        message.webView?.replaceLocation(with: originalURL)
        // webview.reload will not change the error URL back to the original URL
      }
    default:
      assertionFailure("Unknown error message")
    }
  }
}
