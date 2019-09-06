// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class ToolbarButton: UIButton {
    fileprivate var selectedTintColor: UIColor?
    fileprivate var primaryTintColor: UIColor?
    fileprivate var disabledTintColor: UIColor?
    
    let top: Bool
    
    required init(top: Bool) {
        self.top = top
        super.init(frame: .zero)
        adjustsImageWhenHighlighted = false
        imageView?.contentMode = .scaleAspectFit
    }

    override init(frame: CGRect) {
        fatalError("init(coder:) has not been implemented")
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override open var isHighlighted: Bool {
        didSet {
            self.tintColor = isHighlighted ? selectedTintColor : primaryTintColor
        }
    }
    
    override open var isEnabled: Bool {
        didSet {
            self.tintColor = isEnabled ? primaryTintColor : disabledTintColor
        }
    }
    
    override var tintColor: UIColor! {
        didSet {
            self.imageView?.tintColor = self.tintColor
        }
    }
    
}

extension ToolbarButton: Themeable {
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        
        selectedTintColor = theme.colors.accent
        primaryTintColor = top ? theme.colors.tints.header : theme.colors.tints.footer
        disabledTintColor = primaryTintColor?.withAlphaComponent(0.4)
        
        // Logic is slightly weird, but necessary for proper styling at launch
        tintColor = isEnabled ? primaryTintColor : disabledTintColor
    }
}
