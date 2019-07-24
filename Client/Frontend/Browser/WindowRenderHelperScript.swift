// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

class WindowRenderHelperScript: TabContentScript {
    fileprivate weak var tab: Tab?
    
    class func name() -> String {
        return "WindowRenderHelper"
    }
    
    required init(tab: Tab) {
        self.tab = tab
    }
    
    func scriptMessageHandlerName() -> String? {
        return "windowRenderHelper"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        // Do nothing with the messages received.
        // For now.. It's useful for debugging though.
    }
    
    static func executeScript(for tab: Tab) {
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        
        tab.webView?.evaluateJavaScript("W\(token).resizeWindow();", completionHandler: nil)
    }
}
