// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Data
import BraveShared

class FingerprintingProtection: TabContentScript {
    fileprivate weak var tab: Tab?
    
    init(tab: Tab) {
        self.tab = tab
    }
    
    static func name() -> String {
        return "FingerprintingProtection"
    }
    
    func scriptMessageHandlerName() -> String? {
        return "fingerprintingProtection"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        if let stats = self.tab?.contentBlocker.stats {
            self.tab?.contentBlocker.stats = stats.addingFingerprintingBlock()
            BraveGlobalShieldStats.shared.fpProtection += 1
        }
    }
}
