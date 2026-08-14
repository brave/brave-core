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

    tab.proceedAnywaysDomainList?.insert(baseDomain)
    if let pendingURL = tab.braveSearch?.pendingQuickViewURL, pendingURL == url {
      tab.braveSearch?.pendingQuickViewURL = nil
      tab.braveSearch?.presentInQuickView?(url, tab)
      tab.goBack()
    } else {
      tab.loadRequest(URLRequest(url: url))
    }
  }

  private func blockedDomainDidGoBack(tab: some TabState) {
    tab.braveSearch?.pendingQuickViewURL = nil
    guard let url = tab.visibleURL?.strippedInternalURL else {
      assertionFailure(
        "There should be no way this method can be triggered if the tab is not on an internal url"
      )
      return
    }

    guard let listItem = tab.backForwardList?.backList.reversed().first(where: { $0.url != url })
    else {
      // How is this even possible?
      // All testing indicates no, so we will not handle.
      // If we find it is, then we need to disable or hide the "Go Back" button in these cases.
      // But this would require heavy changes or ugly mechanisms to InternalSchemeHandler.
      tab.goBack()
      return
    }

    tab.goToBackForwardListItem(listItem)
  }
}
