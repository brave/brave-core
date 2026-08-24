// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Web
import WebKit

class BlockedDomainScriptHandler: TabContentScript {
  static let scriptName = "BlockedDomainScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
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
      blockedDomainDidProceed(tab: tab)
    case "didGoBack":
      blockedDomainDidGoBack(tab: tab)
    default:
      assertionFailure("Unhandled action `\(action)`")
    }
  }

  private func blockedDomainDidProceed(tab: some TabState) {
    guard let url = tab.visibleURL?.strippedInternalURL, let baseDomain = url.baseDomain else {
      assertionFailure(
        "There should be no way this method can be triggered if the tab is not on an internal url"
      )
      return
    }

    let request = URLRequest(url: url)
    tab.proceedAnywaysDomainList?.insert(baseDomain)
    tab.loadRequest(request)
  }

  private func blockedDomainDidGoBack(tab: some TabState) {
    guard let url = tab.visibleURL?.strippedInternalURL else {
      assertionFailure(
        "There should be no way this method can be triggered if the tab is not on an internal url"
      )
      return
    }

    guard let listItem = tab.backForwardList?.backList.reversed().first(where: { $0.url != url })
    else {
      // this is possible when blocked domain interstitial screen is load explicitly
      // for example, when a blocked domain was visited in QuickView mode, it will cause
      // the QuickView to close and load the interstital explicitly in a regular tab.
      if let url = URL(string: "about:newtab") {
        tab.loadRequest(URLRequest(url: url))
      }
      return
    }

    tab.goToBackForwardListItem(listItem)
  }
}
