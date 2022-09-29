/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import BraveShared

class NoImageModeScriptHandler: TabContentScript {
  fileprivate weak var tab: Tab?

  required init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "NoImageModeScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    // Do nothing.
  }

  static var isActivated: Bool {
    return Preferences.Shields.blockImages.value
  }
  
  static func executeScript(for webView: WKWebView) {
    webView.evaluateSafeJavaScript(functionName: "__firefox__.NoImageMode.setEnabled", args: [true], contentWorld: NoImageModeScriptHandler.scriptSandbox)
  }
}
