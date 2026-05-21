// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Web
import WebKit
import os.log

class PrintScriptHandler: TabContentScript {
  private weak var browserController: BrowserViewController?
  private var isPresentingController = false
  private var printCounter = 0
  private var isBlocking = false
  private var currentDomain: String?

  required init(browserController: BrowserViewController) {
    self.browserController = browserController
  }

  static let scriptName = "PrintScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "printScriptHandler"
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
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    tab.print?.print(view: tab.view, title: nil)
  }
}
