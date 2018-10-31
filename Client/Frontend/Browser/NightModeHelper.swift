/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import BraveShared

class NightModeHelper: TabContentScript {
    fileprivate weak var tab: Tab?

    required init(tab: Tab) {
        self.tab = tab
    }

    static func name() -> String {
        return "NightMode"
    }

    func scriptMessageHandlerName() -> String? {
        return "NightMode"
    }

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        // Do nothing.
    }

    static func toggle(tabManager: TabManager) {
        let isActive = Preferences.General.nightMode.value
        setNightMode(tabManager: tabManager, enabled: !isActive)
    }
    
    static func setNightMode(tabManager: TabManager, enabled: Bool) {
        Preferences.General.nightMode.value = enabled
        for tab in tabManager.allTabs {
            tab.setNightMode(enabled)
        }
    }

    static func isActivated() -> Bool {
        return Preferences.General.nightMode.value
    }
}
