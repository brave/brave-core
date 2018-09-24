/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

class RequestDesktopSiteActivity: UIActivity {
    private let tab: Tab?
    fileprivate let callback: () -> Void

    init(tab: Tab?, callback: @escaping () -> Void) {
        self.tab = tab
        self.callback = callback
    }

    private var isDesktopSite: Bool {
        return tab?.desktopSite ?? true
    }
    
    override var activityTitle: String? {
        return isDesktopSite ? Strings.AppMenuViewMobileSiteTitleString : Strings.AppMenuViewDesktopSiteTitleString
    }

    override var activityImage: UIImage? {
        return isDesktopSite ? #imageLiteral(resourceName: "shareRequestMobileSite") : #imageLiteral(resourceName: "shareRequestDesktopSite")
    }

    override func perform() {
        callback()
        activityDidFinish(true)
    }

    override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
        return true
    }
}
