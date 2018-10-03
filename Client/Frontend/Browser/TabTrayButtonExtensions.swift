/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class PrivateModeButton: InsetButton, Themeable {
    var light: Bool = false
    
    override var isSelected: Bool {
        didSet {
            backgroundColor = isSelected ? UIColor.Photon.Purple60 : .clear
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        accessibilityLabel = PrivateModeStrings.toggleAccessibilityLabel
        accessibilityHint = PrivateModeStrings.toggleAccessibilityHint
        
        titleEdgeInsets = UIEdgeInsets(top: -3, left: 6, bottom: -3, right: 6)
        layer.cornerRadius = 4.0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func applyTheme(_ theme: Theme) {
        setTitleColor(UIColor.TabTray.ToolbarButtonTint.colorFor(theme), for: .normal)
        imageView?.tintColor = tintColor
        isSelected = theme.isPrivate
        accessibilityValue = isSelected ? PrivateModeStrings.toggleAccessibilityValueOn : PrivateModeStrings.toggleAccessibilityValueOff
    }
}

extension UIButton {
    static func newTabButton() -> UIButton {
        let newTab = UIButton()
        newTab.setImage(#imageLiteral(resourceName: "quick_action_new_tab").template, for: .normal)
        newTab.accessibilityLabel = NSLocalizedString("New Tab", comment: "Accessibility label for the New Tab button in the tab toolbar.")
        return newTab
    }
}

extension TabsButton {
    static func tabTrayButton() -> TabsButton {
        let tabsButton = TabsButton()
        tabsButton.countLabel.text = "0"
        tabsButton.accessibilityLabel = NSLocalizedString("Toolbar.Show.Tabs.Button.Accessibility.Label", value: "Show Tabs", comment: "Accessibility Label for the tabs button in the tab toolbar")
        return tabsButton
    }
}
