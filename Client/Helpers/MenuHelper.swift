/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

@objc public protocol MenuHelperInterface {
    @objc optional func menuHelperCopy()
    @objc optional func menuHelperOpenAndFill()
    @objc optional func menuHelperReveal()
    @objc optional func menuHelperSecure()
    @objc optional func menuHelperFindInPage()
}

open class MenuHelper: NSObject {
    public static let SelectorCopy: Selector = #selector(MenuHelperInterface.menuHelperCopy)
    public static let SelectorHide: Selector = #selector(MenuHelperInterface.menuHelperSecure)
    public static let SelectorOpenAndFill: Selector = #selector(MenuHelperInterface.menuHelperOpenAndFill)
    public static let SelectorReveal: Selector = #selector(MenuHelperInterface.menuHelperReveal)
    public static let SelectorFindInPage: Selector = #selector(MenuHelperInterface.menuHelperFindInPage)

    open class var defaultHelper: MenuHelper {
        struct Singleton {
            static let instance = MenuHelper()
        }
        return Singleton.instance
    }

    open func setItems() {
        let revealPasswordItem = UIMenuItem(title: Strings.MenuItemRevealPasswordTitle, action: MenuHelper.SelectorReveal)
        let hidePasswordItem = UIMenuItem(title: Strings.MenuItemHidePasswordTitle, action: MenuHelper.SelectorHide)
        let copyItem = UIMenuItem(title: Strings.MenuItemCopyTitle, action: MenuHelper.SelectorCopy)
        let openAndFillItem = UIMenuItem(title: Strings.MenuItemOpenAndFillTitle, action: MenuHelper.SelectorOpenAndFill)
        let findInPageItem = UIMenuItem(title: Strings.Find_in_Page, action: MenuHelper.SelectorFindInPage)

        UIMenuController.shared.menuItems = [copyItem, revealPasswordItem, hidePasswordItem, openAndFillItem, findInPageItem]
    }
}
