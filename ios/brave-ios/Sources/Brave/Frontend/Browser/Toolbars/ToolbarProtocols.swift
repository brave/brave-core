// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

protocol ToolbarProtocol: AnyObject {

  var tabToolbarDelegate: ToolbarDelegate? { get set }
  var tabsButton: TabsButton { get }
  var backButton: ToolbarButton { get }
  var forwardButton: ToolbarButton { get }
  var shareButton: ToolbarButton { get }
  var addTabButton: ToolbarButton { get }
  var searchButton: ToolbarButton { get }
  var menuButton: MenuButton { get }
  var actionButtons: [UIButton] { get }

  func updateBackStatus(_ canGoBack: Bool)
  func updateForwardStatus(_ canGoForward: Bool)
  func updatePageStatus(_ isWebPage: Bool)
  func updateTabCount(_ count: Int)
}

extension ToolbarProtocol {
  func updatePageStatus(_ isWebPage: Bool) {
    shareButton.isEnabled = isWebPage
  }

  func updateBackStatus(_ canGoBack: Bool) {
    backButton.isEnabled = canGoBack
  }

  func updateForwardStatus(_ canGoForward: Bool) {
    forwardButton.isEnabled = canGoForward
  }

  func updateTabCount(_ count: Int) {
    tabsButton.updateTabCount(count)
  }
}

protocol ToolbarUrlActionsProtocol where Self: UIViewController {
  var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate? { get }
}

protocol ToolbarDelegate: AnyObject {
  func tabToolbarDidPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidLongPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidPressShare()
  func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressSearch(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidSwipeToChangeTabs(_ tabToolbar: ToolbarProtocol, direction: UISwipeGestureRecognizer.Direction)
}
