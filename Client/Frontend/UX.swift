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
        static let height: CGFloat = UIConstants.TopToolbarHeight
    }
    
    struct HomePanel {
        static let BackgroundColorPBM = GreyJ
        static let BackgroundColor = UIColor.white
        static let StatTitleColor = UIColor(rgb: 0x8C9094)
    }
    
    struct Favorites {
        static let cellLabelColorNormal = UIColor(rgb: 0x2D2D2D)
        static let cellLabelColorPrivate = UIColor(rgb: 0xDBDBDB)
    }
    
    static let BraveOrange = UIColor(rgb: 0xfb542b)
    
    static let Blue = UIColor(rgb: 0x424acb)
    static let LightBlue = UIColor(rgb: 0x4A90E2)
    static let Purple = UIColor(rgb: 0x8236b9)
    static let Green = UIColor(rgb: 0x1bc760)
    static let Red = UIColor(rgb: 0xE2052A)
    
    static let White = UIColor.white
    static let Black = UIColor.black
    
    static let GreyA = UIColor(rgb: 0xF7F8F9)
    static let GreyB = UIColor(rgb: 0xE7EBEE)
    static let GreyC = UIColor(rgb: 0xDBDFE3)
    static let GreyD = UIColor(rgb: 0xCDD1D5)
    static let GreyE = UIColor(rgb: 0xA7ACB2)
    static let GreyF = UIColor(rgb: 0x999EA2)
    static let GreyG = UIColor(rgb: 0x818589)
    static let GreyH = UIColor(rgb: 0x606467)
    static let GreyI = UIColor(rgb: 0x484B4E)
    static let GreyJ = UIColor(rgb: 0x222326)
}
