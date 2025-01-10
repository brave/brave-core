// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

protocol SessionRestoreScriptHandlerDelegate: AnyObject {
  func sessionRestore(_ handler: SessionRestoreScriptHandler, didRestoreSessionForTab tab: Tab)
}

class SessionRestoreScriptHandler: TabContentScript {
  weak var delegate: SessionRestoreScriptHandlerDelegate?

  static let scriptName = "SessionRestoreScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
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
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    if let params = message.body as? [String: AnyObject] {
      if params["name"] as? String == "didRestoreSession" {
        DispatchQueue.main.async {
          self.delegate?.sessionRestore(self, didRestoreSessionForTab: tab)
        }
      }
    }
  }
}
