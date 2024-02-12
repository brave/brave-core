// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

class BlockedDomainScriptHandler: TabContentScript {
  private weak var tab: Tab?

  required init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "BlockedDomainScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let params = message.body as? [String: AnyObject], let action = params["action"] as? String else {
      assertionFailure("Missing required params.")
      return
    }
    
    switch action {
    case "didProceed":
      blockedDomainDidProceed()
    case "didGoBack":
      blockedDomainDidGoBack()
    default:
      assertionFailure("Unhandled action `\(action)`")
    }
  }
  
  private func blockedDomainDidProceed() {
    guard let url = tab?.url?.stippedInternalURL, let etldP1 = url.baseDomain else {
      assertionFailure("There should be no way this method can be triggered if the tab is not on an internal url")
      return
    }
    
    let request = URLRequest(url: url)
    tab?.proceedAnywaysDomainList.insert(etldP1)
    tab?.loadRequest(request)
  }
  
  private func blockedDomainDidGoBack() {
    guard let url = tab?.url?.stippedInternalURL else {
      assertionFailure("There should be no way this method can be triggered if the tab is not on an internal url")
      return
    }
    
    guard let listItem = tab?.backList?.reversed().first(where: { $0.url != url }) else {
      // How is this even possible?
      // All testing indicates no, so we will not handle.
      // If we find it is, then we need to disable or hide the "Go Back" button in these cases.
      // But this would require heavy changes or ugly mechanisms to InternalSchemeHandler.
      return
    }
    
    tab?.goToBackForwardListItem(listItem)
  }
}
