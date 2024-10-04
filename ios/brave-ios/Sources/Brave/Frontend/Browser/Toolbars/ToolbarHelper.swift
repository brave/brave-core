// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Shared
import UIKit

@objcMembers
class ToolbarHelper: NSObject {
  let toolbar: ToolbarProtocol

  init(toolbar: ToolbarProtocol) {
    self.toolbar = toolbar
    super.init()

    toolbar.backButton.setImage(UIImage(braveSystemNamed: "leo.browser.back"), for: .normal)
    toolbar.backButton.accessibilityLabel = Strings.tabToolbarBackButtonAccessibilityLabel
    let longPressGestureBackButton = UILongPressGestureRecognizer(
      target: self,
      action: #selector(didLongPressBack)
    )
    toolbar.backButton.addGestureRecognizer(longPressGestureBackButton)
    toolbar.backButton.addTarget(self, action: #selector(didClickBack), for: .touchUpInside)

    toolbar.shareButton.setImage(UIImage(braveSystemNamed: "leo.share.macos"), for: .normal)
    toolbar.shareButton.accessibilityLabel = Strings.tabToolbarShareButtonAccessibilityLabel
    toolbar.shareButton.addTarget(
      self,
      action: #selector(didClickShare),
      for: UIControl.Event.touchUpInside
    )

    toolbar.tabsButton.addTarget(self, action: #selector(didClickTabs), for: .touchUpInside)

    toolbar.addTabButton.setImage(UIImage(braveSystemNamed: "leo.plus.add"), for: .normal)
    toolbar.addTabButton.accessibilityLabel = Strings.tabToolbarAddTabButtonAccessibilityLabel
    toolbar.addTabButton.addTarget(
      self,
      action: #selector(didClickAddTab),
      for: UIControl.Event.touchUpInside
    )

    toolbar.searchButton.setImage(UIImage(braveSystemNamed: "leo.search"), for: .normal)
    // Accessibility label not needed, since overriden in the bottom tool bar class.
    toolbar.searchButton.addTarget(
      self,
      action: #selector(didClickSearch),
      for: UIControl.Event.touchUpInside
    )

    toolbar.menuButton.setImage(UIImage(braveSystemNamed: "leo.more.horizontal"), for: .normal)
    toolbar.menuButton.accessibilityLabel = Strings.tabToolbarMenuButtonAccessibilityLabel
    toolbar.menuButton.addTarget(
      self,
      action: #selector(didClickMenu),
      for: UIControl.Event.touchUpInside
    )

    toolbar.forwardButton.setImage(UIImage(braveSystemNamed: "leo.browser.forward"), for: .normal)
    toolbar.forwardButton.accessibilityLabel = Strings.tabToolbarForwardButtonAccessibilityLabel
    let longPressGestureForwardButton = UILongPressGestureRecognizer(
      target: self,
      action: #selector(didLongPressForward)
    )
    toolbar.forwardButton.addGestureRecognizer(longPressGestureForwardButton)
    toolbar.forwardButton.addTarget(self, action: #selector(didClickForward), for: .touchUpInside)
  }

  func didClickMenu() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressMenu(toolbar)
  }

  func didClickBack() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressBack(toolbar, button: toolbar.backButton)
  }

  func didLongPressBack(_ recognizer: UILongPressGestureRecognizer) {
    if recognizer.state == .began {
      toolbar.tabToolbarDelegate?.tabToolbarDidLongPressBack(toolbar, button: toolbar.backButton)
    }
  }

  func didClickTabs() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressTabs(toolbar, button: toolbar.tabsButton)
  }

  func didClickForward() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressForward(toolbar, button: toolbar.forwardButton)
  }

  func didLongPressForward(_ recognizer: UILongPressGestureRecognizer) {
    if recognizer.state == .began {
      toolbar.tabToolbarDelegate?.tabToolbarDidLongPressForward(
        toolbar,
        button: toolbar.forwardButton
      )
    }
  }

  func didClickShare() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressShare()
  }

  func didClickAddTab() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressAddTab(toolbar, button: toolbar.shareButton)
  }

  func didClickSearch() {
    toolbar.tabToolbarDelegate?.tabToolbarDidPressSearch(toolbar, button: toolbar.searchButton)
  }

  func updateForTraitCollection(
    _ traitCollection: UITraitCollection,
    browserColors: some BrowserColors,
    isBottomToolbar: Bool,
    additionalButtons: [UIButton] = []
  ) {
    let toolbarTraitCollection = UITraitCollection(
      preferredContentSizeCategory: traitCollection.toolbarButtonContentSizeCategory
    )
    let config = UIImage.SymbolConfiguration(
      pointSize: isBottomToolbar
        ? UIFont.preferredFont(forTextStyle: .body, compatibleWith: toolbarTraitCollection)
          .pointSize
        : UIFontMetrics.default.scaledValue(for: 14, compatibleWith: toolbarTraitCollection),
      weight: .regular,
      scale: .large
    )
    let buttons: [UIButton] =
      [
        toolbar.backButton,
        toolbar.forwardButton,
        toolbar.addTabButton,
        toolbar.menuButton,
        toolbar.searchButton,
        toolbar.shareButton,
      ] + additionalButtons
    for button in buttons {
      button.setPreferredSymbolConfiguration(config, forImageIn: .normal)
      button.tintColor = browserColors.iconDefault
      if let button = button as? ToolbarButton {
        button.primaryTintColor = browserColors.iconDefault
        button.selectedTintColor = browserColors.iconActive
        button.disabledTintColor = browserColors.iconDisabled
      }
    }
    toolbar.tabsButton.browserColors = browserColors
  }
}
