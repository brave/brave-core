// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import WebKit

class HTTPBlockedScriptHandler: TabContentScript {
  private weak var tab: Tab?

  required init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "HTTPBlockedScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let params = message.body as? [String: AnyObject],
      let action = params["action"] as? String
    else {
      assertionFailure("Missing required params.")
      return
    }

    switch action {
    case "didProceed":
      didProceed()
    case "didGoBack":
      didGoBack()
    default:
      assertionFailure("Unhandled action `\(action)`")
    }
  }

  private func didProceed() {
    guard let tab, let url = tab.upgradedHTTPSRequest?.url ?? tab.url?.strippedInternalURL else {
      //      assertionFailure(
      //        "There should be no way this method can be triggered if the tab is not on an internal url"
      //      )
      return
    }

    // When restoring the page, `upgradedHTTPSRequest` will be nil
    // So we default to the embedded internal page URL
    let request = tab.upgradedHTTPSRequest ?? URLRequest(url: url)
    if let httpsUpgradeService = HttpsUpgradeServiceFactory.get(privateMode: tab.isPrivate),
      let host = url.host(percentEncoded: false)
    {
      httpsUpgradeService.allowHttp(forHost: host)
    }
    tab.loadRequest(request)
  }

  private func didGoBack() {
    let etldP1 =
      tab?.upgradedHTTPSRequest?.url?.baseDomain
      ?? tab?.url?.strippedInternalURL?.baseDomain

    guard
      let listItem = tab?.backList?.reversed().first(where: {
        // It is not the blocked page or the internal page
        $0.url.baseDomain != etldP1 && $0.url != tab?.webView?.url
      })
    else {
      tab?.goBack()
      return
    }

    tab?.upgradedHTTPSRequest = nil
    tab?.goToBackForwardListItem(listItem)
  }
}
