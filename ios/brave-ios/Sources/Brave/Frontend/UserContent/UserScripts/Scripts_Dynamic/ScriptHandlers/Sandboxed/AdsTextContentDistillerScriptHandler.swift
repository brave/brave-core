// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
@_spi(ChromiumWebViewAccess) import Web
import WebKit
import os.log

/// Legacy (`WKWebView`) counterpart of `TextContentDistillerJavaScriptFeature`, used to distill a
/// page's text content for ad text classification when
/// `FeatureList.kUseProfileWebViewConfiguration` is disabled. Shares its underlying script
/// (`TextContentDistillerScript.js`) with `BraveLeoScriptHandler`'s AI Chat article distillation.
class AdsTextContentDistillerScriptHandler: NSObject, TabContentScript {
  static let getTextContent = "getTextContent\(uniqueID)"

  static let scriptName = "TextContentDistillerScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerNamesMap: ["$<getTextContent>": getTextContent],
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // This handler only exposes a callable function and never receives script messages.
    replyHandler(nil, nil)
  }
}
