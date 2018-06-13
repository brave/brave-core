/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

struct UX {
    static let barsBackgroundSolidColor = UIColor(red: 234/255.0, green: 234/255.0, blue: 234/255.0, alpha: 1.0)
    static let barsDarkBackgroundSolidColor = UIColor(red: 63/255.0, green: 63/255.0, blue: 63/255.0, alpha: 1.0)
    
    struct TabsBar {
        static let buttonWidth = UIDevice.current.userInterfaceIdiom == .pad ? 40 : 0
        static let height: CGFloat = 29
        static let minimumWidth: CGFloat =  UIDevice.current.userInterfaceIdiom == .pad ? 180 : 160
    }
    
    struct UrlBar {
        static let height: CGFloat = UIConstants.TopToolbarHeight + UX.TabsBar.height
    }
}
