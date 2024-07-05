// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

class FocusScriptHandler: TabContentScript {
  fileprivate weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "FocusScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "focusHelper"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    guard let body = message.body as? [String: AnyObject] else {
      Logger.module.error("FocusHelper.js sent wrong type of message")
      return (nil, nil)
    }

    guard let data = body["data"] as? [String: String] else {
      Logger.module.error("FocusHelper.js sent wrong type of message")
      return (nil, nil)
    }

    guard let _ = data["elementType"],
      let eventType = data["eventType"]
    else {
      Logger.module.error("FocusHelper.js sent wrong keys for message")
      return (nil, nil)
    }

    switch eventType {
    case "focus":
      tab?.isEditing = true
    case "blur":
      tab?.isEditing = false
    default:
      Logger.module.error("FocusHelper.js sent unhandled eventType")
    }

    return (nil, nil)
  }
}
