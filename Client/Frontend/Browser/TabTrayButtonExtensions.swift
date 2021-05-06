/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

class PrivateModeButton: InsetButton {
    override var isSelected: Bool {
        didSet {
            accessibilityValue = isSelected ? Strings.tabPrivateModeToggleAccessibilityValueOn : Strings.tabPrivateModeToggleAccessibilityValueOff
            backgroundColor = isSelected ? selectedBackgroundColor : .clear
            setTitleColor(isSelected ? .white : .braveLabel, for: .normal)
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
        accessibilityLabel = Strings.tabPrivateModeToggleAccessibilityLabel
        accessibilityHint = Strings.tabPrivateModeToggleAccessibilityHint
        
        titleEdgeInsets = UIEdgeInsets(top: -3, left: 6, bottom: -3, right: 6)
        layer.cornerRadius = 4.0
        layer.cornerCurve = .continuous
        
        setTitleColor(.braveLabel, for: .normal)
        selectedBackgroundColor = .braveOrange
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

extension UIButton {
    static func newTabButton() -> UIButton {
        let newTab = UIButton()
        newTab.setImage(#imageLiteral(resourceName: "quick_action_new_tab").template, for: .normal)
        newTab.accessibilityLabel = Strings.tabTrayNewTabButtonAccessibilityLabel
        return newTab
    }
}
