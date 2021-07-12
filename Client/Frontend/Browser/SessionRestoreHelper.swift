/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

protocol SessionRestoreHelperDelegate: AnyObject {
    func sessionRestoreHelper(_ helper: SessionRestoreHelper, didRestoreSessionForTab tab: Tab)
}

class SessionRestoreHelper: TabContentScript {
    weak var delegate: SessionRestoreHelperDelegate?
    fileprivate weak var tab: Tab?

    required init(tab: Tab) {
        self.tab = tab
    }

    func scriptMessageHandlerName() -> String? {
        return "sessionRestoreHelper"
    }

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard let body = message.body as? [String: AnyObject] else {
            return
        }
        
        if UserScriptManager.isMessageHandlerTokenMissing(in: body) {
            log.debug("Missing required security token.")
            return
        }
        
        if let tab = tab {
            if body["name"] as? String == "didRestoreSession" {
                DispatchQueue.main.async {
                    self.delegate?.sessionRestoreHelper(self, didRestoreSessionForTab: tab)
                }
            }
        }
    }

    class func name() -> String {
        return "SessionRestoreHelper"
    }
}
