/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared

public struct UIConstants {
    static let aboutHomePage = URL(string: "\(WebServer.sharedInstance.base)/about/home/")!

    static let defaultPadding: CGFloat = 10
    static let snackbarButtonHeight: CGFloat = 48
    static let topToolbarHeight: CGFloat = 44
    static var toolbarHeight: CGFloat = 44
    static var bottomToolbarHeight: CGFloat {
        get {
            let bottomInset: CGFloat = UIApplication.shared.keyWindow?.safeAreaInsets.bottom ?? 0.0
            return toolbarHeight + bottomInset
        }
    }

    // Static fonts
    static let defaultChromeSize: CGFloat = 16
    static let defaultChromeSmallSize: CGFloat = 11
    static let passcodeEntryFontSize: CGFloat = 36
    static let defaultChromeFont: UIFont = UIFont.systemFont(ofSize: defaultChromeSize, weight: UIFont.Weight.regular)
    static let defaultChromeSmallFontBold = UIFont.boldSystemFont(ofSize: defaultChromeSmallSize)
    static let passcodeEntryFont = UIFont.systemFont(ofSize: passcodeEntryFontSize, weight: UIFont.Weight.bold)

    // Used as backgrounds for favicons
    static let defaultColorStrings = ["2e761a", "399320", "40a624", "57bd35", "70cf5b", "90e07f", "b1eea5", "881606", "aa1b08", "c21f09", "d92215", "ee4b36", "f67964", "ffa792", "025295", "0568ba", "0675d3", "0996f8", "2ea3ff", "61b4ff", "95cdff", "00736f", "01908b", "01a39d", "01bdad", "27d9d2", "58e7e6", "89f4f5", "c84510", "e35b0f", "f77100", "ff9216", "ffad2e", "ffc446", "ffdf81", "911a2e", "b7223b", "cf2743", "ea385e", "fa526e", "ff7a8d", "ffa7b3" ]

    /// JPEG compression quality for persisted screenshots. Must be between 0-1.
    static let screenshotQuality: Float = 0.3
    static let activeScreenshotQuality: CGFloat = 0.5
}
