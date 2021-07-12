/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

protocol FindInPageHelperDelegate: AnyObject {
    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateCurrentResult currentResult: Int)
    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateTotalResults totalResults: Int)
}

class FindInPageHelper: TabContentScript {
    weak var delegate: FindInPageHelperDelegate?
    fileprivate weak var tab: Tab?

    class func name() -> String {
        return "FindInPage"
    }

    required init(tab: Tab) {
        self.tab = tab
    }

    func scriptMessageHandlerName() -> String? {
        return "findInPageHandler"
    }

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard let body = message.body as? [String: AnyObject] else {
            return
        }
        
        if UserScriptManager.isMessageHandlerTokenMissing(in: body) {
            log.debug("Missing required security token.")
            return
        }
        
        guard let data = body["data"] as? [String: Int] else {
            log.error("Could not find a message body or the data did not meet expectations: \(message.body)")
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
