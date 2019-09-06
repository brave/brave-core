// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class ReaderModeButton: UIButton {
    var selectedTintColor: UIColor?
    var unselectedTintColor: UIColor?
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        adjustsImageWhenHighlighted = false
        setImage(#imageLiteral(resourceName: "reader").template, for: .normal)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override var isSelected: Bool {
        didSet {
            updateAppearance()
        }
    }
    
    override open var isHighlighted: Bool {
        didSet {
            updateAppearance()
        }
    }
    
    override var tintColor: UIColor! {
        didSet {
            self.imageView?.tintColor = self.tintColor
        }
    }
    
    private func updateAppearance() {
        self.tintColor = (isHighlighted || isSelected) ? selectedTintColor : unselectedTintColor
    }
    
    private var _readerModeState: ReaderModeState = .unavailable
    
    var readerModeState: ReaderModeState {
        get {
            return _readerModeState
        }
        set (newReaderModeState) {
            _readerModeState = newReaderModeState
            switch _readerModeState {
            case .available:
                self.isEnabled = true
                self.isSelected = false
            case .unavailable:
                self.isEnabled = false
                self.isSelected = false
            case .active:
                self.isEnabled = true
                self.isSelected = true
            }
        }
    }
}
