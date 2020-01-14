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
        static let height: CGFloat = UIConstants.topToolbarHeight
    }
    
    struct HomePanel {
        static let backgroundColorPBM = greyJ
        static let backgroundColor = UIColor.white
        static let statTitleColor = UIColor(rgb: 0x8C9094)
    }

    static let braveOrange = UIColor(rgb: 0xfb542b)
    
    static let blue = UIColor(rgb: 0x424acb)
    static let lightBlue = UIColor(rgb: 0x4A90E2)
    static let purple = UIColor(rgb: 0x8236b9)
    static let green = UIColor(rgb: 0x1bc760)
    static let red = UIColor(rgb: 0xE2052A)
    
    static let white = UIColor.white
    static let black = UIColor.black
    
    static let greyA = UIColor(rgb: 0xF7F8F9)
    static let greyB = UIColor(rgb: 0xE7EBEE)
    static let greyC = UIColor(rgb: 0xDBDFE3)
    static let greyD = UIColor(rgb: 0xCDD1D5)
    static let greyE = UIColor(rgb: 0xA7ACB2)
    static let greyF = UIColor(rgb: 0x999EA2)
    static let greyG = UIColor(rgb: 0x818589)
    static let greyH = UIColor(rgb: 0x606467)
    static let greyI = UIColor(rgb: 0x484B4E)
    static let greyJ = UIColor(rgb: 0x222326)
}
