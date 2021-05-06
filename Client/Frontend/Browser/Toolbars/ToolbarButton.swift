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
        
        selectedTintColor = .braveOrange
        primaryTintColor = .braveLabel
        tintColor = primaryTintColor
        imageView?.tintColor = tintColor
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
            self.tintColor = primaryTintColor?.withAlphaComponent(isEnabled ? 1.0 : 0.4)
        }
    }
    
    override var tintColor: UIColor! {
        didSet {
            self.imageView?.tintColor = self.tintColor
        }
    }
    
}
