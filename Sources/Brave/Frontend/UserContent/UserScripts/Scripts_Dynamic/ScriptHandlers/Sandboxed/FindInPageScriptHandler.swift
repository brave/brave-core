/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import WebKit
import os.log

@available(iOS, obsoleted: 16.0, message: "Replaced by UIFindInteraction")
protocol FindInPageScriptHandlerDelegate: AnyObject {
  func findInPageHelper(_ findInPageScriptHandler: FindInPageScriptHandler, didUpdateCurrentResult currentResult: Int)
  func findInPageHelper(_ findInPageScriptHandler: FindInPageScriptHandler, didUpdateTotalResults totalResults: Int)
}

@available(iOS, obsoleted: 16.0, message: "Replaced by UIFindInteraction")
class FindInPageScriptHandler: TabContentScript {
  weak var delegate: FindInPageScriptHandlerDelegate?
  fileprivate weak var tab: Tab?

  required init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "FindInPageScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "findInPageHandler"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }
    
    guard let body = message.body as? [String: AnyObject] else {
      return
    }

    guard let data = body["data"] as? [String: Int] else {
      if let body = message.body as? String {
        Logger.module.error("Could not find a message body or the data did not meet expectations: \(body))")
      }
      
      return
    }

    if let currentResult = data["currentResult"] {
      delegate?.findInPageHelper(self, didUpdateCurrentResult: currentResult)
    }

    if let totalResults = data["totalResults"] {
      delegate?.findInPageHelper(self, didUpdateTotalResults: totalResults)
    }
  }
}
