// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

@objc public protocol MenuHelperInterface {
  /// Triggered when "Copy" menu item is selected
  @objc optional func menuHelperCopy()
  /// Triggered when "Open website" menu item is selected
  @objc optional func menuHelperOpenWebsite()
  /// Triggered when "Force Paste" menu item is selected"
  @objc optional func menuHelperForcePaste()
  /// Triggered when "Reveal" menu item is selected
  @objc optional func menuHelperReveal()
  /// Triggered when "Hide" menu item is selected
  @objc optional func menuHelperSecure()
  /// Triggered when "Search with Brave" menu item is selected
  @objc optional func menuHelperSearchWithBrave()
}

open class MenuHelper: NSObject {
  /// Selector for the "Copy" menu item
  public static let selectorCopy: Selector = #selector(MenuHelperInterface.menuHelperCopy)
  /// Selector for the "Hide" menu item
  public static let selectorHide: Selector = #selector(MenuHelperInterface.menuHelperSecure)
  /// Selector for the "Force Paste" menu item
  public static let selectorForcePaste: Selector = #selector(
    MenuHelperInterface.menuHelperForcePaste
  )
  /// Selector for the "Open website" menu item
  public static let selectorOpenWebsite: Selector = #selector(
    MenuHelperInterface.menuHelperOpenWebsite
  )
  /// Selector for the "Reveal" menu item
  public static let selectorReveal: Selector = #selector(MenuHelperInterface.menuHelperReveal)

  /// Selector for the "Search with Brave" menu item
  public static let selectorSearchWithBrave: Selector = #selector(
    MenuHelperInterface.menuHelperSearchWithBrave
  )

  open class var defaultHelper: MenuHelper {
    struct Singleton {
      static let instance = MenuHelper()
    }
    return Singleton.instance
  }

  /// Set custom menu items on `UIMenuController.shared`
  open func setItems() {
    let revealPasswordItem = UIMenuItem(
      title: Strings.menuItemRevealPasswordTitle,
      action: MenuHelper.selectorReveal
    )
    let hidePasswordItem = UIMenuItem(
      title: Strings.menuItemHidePasswordTitle,
      action: MenuHelper.selectorHide
    )
    let copyItem = UIMenuItem(title: Strings.menuItemCopyTitle, action: MenuHelper.selectorCopy)
    let openWebsiteItem = UIMenuItem(
      title: Strings.menuItemOpenWebsiteTitle,
      action: MenuHelper.selectorOpenWebsite
    )
    UIMenuController.shared.menuItems = [
      copyItem, revealPasswordItem, hidePasswordItem, openWebsiteItem,
    ]
  }
}
