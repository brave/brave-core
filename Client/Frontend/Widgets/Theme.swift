/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

protocol Themeable {
    
    func applyTheme(_ theme: Theme)
    
}

enum Theme: String {
    
    /// Regular browsing.
    case regular

    /// Private browsing.
    case `private`
    
    /// Textual representation suitable for debugging.
    var debugDescription: String {
        switch self {
        case .regular:
            return "Regular theme"
        case .private:
            return "Private theme"
        }
    }

    /// Returns whether the theme is private or not.
    var isPrivate: Bool {
        switch self {
        case .regular:
            return false
        case .private:
            return true
        }
    }
    
    /// Returns the theme of the given Tab, if the tab is nil returns a regular theme.
    ///
    /// - parameter tab: An object representing a Tab.
    /// - returns: A Tab theme.
    static func of(_ tab: Tab?) -> Theme {
        if let tab = tab {
            switch TabType.of(tab) {
            case .regular:
                return .regular
            case .private:
                return .private
            }
        }
        return PrivateBrowsingManager.shared.isPrivateBrowsing ? .private : .regular
    }
    
}
