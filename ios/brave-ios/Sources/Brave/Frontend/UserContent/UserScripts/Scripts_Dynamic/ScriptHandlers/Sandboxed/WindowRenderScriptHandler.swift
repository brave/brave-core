// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

class WindowRenderScriptHandler: TabContentScript {
  static let scriptName = "WindowRenderScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  private static let resizeWindowFunction = "\(scriptName)_\(uniqueID)"

  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerNamesMap: ["$<window_render_script>": resizeWindowFunction],
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
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // Do nothing with the messages received.
    // For now.. It's useful for debugging though.
  }

  static func executeScript(for tab: Tab) {
    tab.webView?.evaluateSafeJavaScript(
      functionName: "window.__firefox__.\(resizeWindowFunction).resizeWindow",
      contentWorld: scriptSandbox
    )
  }
}
