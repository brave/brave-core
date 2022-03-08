/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

@objc public protocol MenuHelperInterface {
    @objc optional func menuHelperCopy()
    @objc optional func menuHelperOpenWebsite()
    @objc optional func menuHelperReveal()
    @objc optional func menuHelperSecure()
    @objc optional func menuHelperFindInPage()
}

open class MenuHelper: NSObject {
    public static let selectorCopy: Selector = #selector(MenuHelperInterface.menuHelperCopy)
    public static let selectorHide: Selector = #selector(MenuHelperInterface.menuHelperSecure)
    public static let selectorOpenWebsite: Selector = #selector(MenuHelperInterface.menuHelperOpenWebsite)
    public static let selectorReveal: Selector = #selector(MenuHelperInterface.menuHelperReveal)
    public static let selectorFindInPage: Selector = #selector(MenuHelperInterface.menuHelperFindInPage)

    open class var defaultHelper: MenuHelper {
        struct Singleton {
            static let instance = MenuHelper()
        }
        return Singleton.instance
    }

    open func setItems() {
        let revealPasswordItem = UIMenuItem(title: Strings.menuItemRevealPasswordTitle, action: MenuHelper.selectorReveal)
        let hidePasswordItem = UIMenuItem(title: Strings.menuItemHidePasswordTitle, action: MenuHelper.selectorHide)
        let copyItem = UIMenuItem(title: Strings.menuItemCopyTitle, action: MenuHelper.selectorCopy)
        let openWebsiteItem = UIMenuItem(title: Strings.menuItemOpenWebsiteTitle, action: MenuHelper.selectorOpenWebsite)
        let findInPageItem = UIMenuItem(title: Strings.findInPage, action: MenuHelper.selectorFindInPage)

        UIMenuController.shared.menuItems = [copyItem, revealPasswordItem, hidePasswordItem, openWebsiteItem, findInPageItem]
    }
}
