// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class RewardsButton: UIButton {
    
    enum IconState {
        case initial
        case enabled
        case disabled
    }
    
    var iconState: IconState = .initial {
        didSet {
            updateView()
        }
    }
    
    private let lookAtMeBadge = UIImageView(image: #imageLiteral(resourceName: "rewards-look-at-me")).then {
        $0.isHidden = true
        $0.isUserInteractionEnabled = false
        $0.frame = CGRect(x: 17, y: 7, width: 16, height: 16)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        adjustsImageWhenHighlighted = false
        imageView?.contentMode = .center
        
        accessibilityLabel = Strings.rewardsPanel
        accessibilityIdentifier = "urlBar-rewardsButton"
        
        addSubview(lookAtMeBadge)
        
        updateView()
    }
    
    private func updateView() {
        switch iconState {
        case .initial:
            setImage(#imageLiteral(resourceName: "brave_rewards_button_enabled"), for: .normal)
            lookAtMeBadge.isHidden = false
        case .enabled:
            setImage(#imageLiteral(resourceName: "brave_rewards_button_enabled"), for: .normal)
            lookAtMeBadge.isHidden = true
        case .disabled:
            setImage(#imageLiteral(resourceName: "brave_rewards_button_disabled"), for: .normal)
            lookAtMeBadge.isHidden = true
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) { fatalError() }
}
