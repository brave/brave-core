// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

class FocusScriptHandler: TabContentScript {
  static let scriptName = "FocusScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "focusHelper"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

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

    guard let body = message.body as? [String: AnyObject] else {
      return Logger.module.error("FocusHelper.js sent wrong type of message")
    }

    guard let data = body["data"] as? [String: String] else {
      return Logger.module.error("FocusHelper.js sent wrong type of message")
    }

    guard let _ = data["elementType"],
      let eventType = data["eventType"]
    else {
      return Logger.module.error("FocusHelper.js sent wrong keys for message")
    }

    switch eventType {
    case "focus":
      tab.isEditing = true
    case "blur":
      tab.isEditing = false
    default:
      return Logger.module.error("FocusHelper.js sent unhandled eventType")
    }
  }
}
