/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

class PrivateModeButton: InsetButton, Themeable {
    override var isSelected: Bool {
        didSet {
            accessibilityValue = isSelected ? Strings.TabPrivateModeToggleAccessibilityValueOn : Strings.TabPrivateModeToggleAccessibilityValueOff
            backgroundColor = isSelected ? selectedBackgroundColor : .clear
        }
    }
    
    var selectedBackgroundColor: UIColor? {
        didSet {
            if isSelected {
                backgroundColor = selectedBackgroundColor
            }
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        accessibilityLabel = Strings.TabPrivateModeToggleAccessibilityLabel
        accessibilityHint = Strings.TabPrivateModeToggleAccessibilityHint
        
        titleEdgeInsets = UIEdgeInsets(top: -3, left: 6, bottom: -3, right: 6)
        layer.cornerRadius = 4.0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        
        setTitleColor(theme.colors.tints.footer, for: .normal)
        imageView?.tintColor = tintColor
        selectedBackgroundColor = theme.colors.accent
    }
}

extension UIButton {
    static func newTabButton() -> UIButton {
        let newTab = UIButton()
        newTab.setImage(#imageLiteral(resourceName: "quick_action_new_tab").template, for: .normal)
        newTab.accessibilityLabel = Strings.TabTrayNewTabButtonAccessibilityLabel
        return newTab
    }
}
