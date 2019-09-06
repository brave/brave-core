// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

@objcMembers
class ToolbarHelper: NSObject {
    let toolbar: ToolbarProtocol
    
    init(toolbar: ToolbarProtocol) {
        self.toolbar = toolbar
        super.init()
        
        toolbar.backButton.setImage(#imageLiteral(resourceName: "nav-back").template, for: .normal)
        toolbar.backButton.accessibilityLabel = Strings.TabToolbarBackButtonAccessibilityLabel
        let longPressGestureBackButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressBack))
        toolbar.backButton.addGestureRecognizer(longPressGestureBackButton)
        toolbar.backButton.addTarget(self, action: #selector(didClickBack), for: .touchUpInside)
        
        toolbar.tabsButton.addTarget(self, action: #selector(didClickTabs), for: .touchUpInside)
        let longPressGestureTabsButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressTabs))
        toolbar.tabsButton.addGestureRecognizer(longPressGestureTabsButton)
        
        toolbar.shareButton.setImage(#imageLiteral(resourceName: "nav-share").template, for: .normal)
        toolbar.shareButton.accessibilityLabel = Strings.TabToolbarShareButtonAccessibilityLabel
        toolbar.shareButton.addTarget(self, action: #selector(didClickShare), for: UIControl.Event.touchUpInside)
        
        toolbar.addTabButton.setImage(#imageLiteral(resourceName: "add_tab"), for: .normal)
        toolbar.addTabButton.accessibilityLabel = Strings.TabToolbarAddTabButtonAccessibilityLabel
        toolbar.addTabButton.addTarget(self, action: #selector(didClickAddTab), for: UIControl.Event.touchUpInside)
        toolbar.addTabButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(didLongPressAddTab(_:))))
        
        toolbar.menuButton.setImage(#imageLiteral(resourceName: "menu_more").template, for: .normal)
        toolbar.menuButton.accessibilityLabel = Strings.TabToolbarMenuButtonAccessibilityLabel
        toolbar.menuButton.addTarget(self, action: #selector(didClickMenu), for: UIControl.Event.touchUpInside)
        
        toolbar.forwardButton.setImage(#imageLiteral(resourceName: "nav-forward").template, for: .normal)
        toolbar.forwardButton.accessibilityLabel = Strings.TabToolbarForwardButtonAccessibilityLabel
        let longPressGestureForwardButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressForward))
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
    
    func didLongPressTabs(_ recognizer: UILongPressGestureRecognizer) {
        toolbar.tabToolbarDelegate?.tabToolbarDidLongPressTabs(toolbar, button: toolbar.tabsButton)
    }
    
    func didClickForward() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressForward(toolbar, button: toolbar.forwardButton)
    }
    
    func didLongPressForward(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began {
            toolbar.tabToolbarDelegate?.tabToolbarDidLongPressForward(toolbar, button: toolbar.forwardButton)
        }
    }
    
    func didClickShare() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressShare()
    }
    
    func didClickAddTab() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressAddTab(toolbar, button: toolbar.shareButton)
    }
    
    func didLongPressAddTab(_ longPress: UILongPressGestureRecognizer) {
        if longPress.state == .began {
            toolbar.tabToolbarDelegate?.tabToolbarDidLongPressAddTab(toolbar, button: toolbar.shareButton)
        }
    }
}
