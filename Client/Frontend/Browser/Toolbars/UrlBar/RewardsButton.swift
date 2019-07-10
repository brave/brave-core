// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

class RewardsButton: UIButton {
    
    var isVerified = false {
        didSet { updateView() }
    }
    
    var notificationCount = 0 {
        didSet { updateView() }
    }
    
    /// Disabled state shows grayscale icon but still allows interaction.
    var isDisabled: Bool = false {
        didSet { updateView() }
    }
    
    private var checkmarkView = UIImageView().then {
        let checkmark = #imageLiteral(resourceName: "brave_rewards_verified_badge")
        $0.image = checkmark
        $0.frame = CGRect(x: 22, y: 2, width: checkmark.size.width, height: checkmark.size.height)
        $0.isHidden = true
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        adjustsImageWhenHighlighted = false
        imageView?.contentMode = .center
        
        accessibilityLabel = Strings.RewardsPanel
        accessibilityIdentifier = "urlBar-rewardsButton"
        
        addSubview(checkmarkView)
        updateView()
    }
    
    private func updateView() {
        checkmarkView.isHidden = true
        // notificationView.isHidden = true
        
        if isDisabled {
            setImage(#imageLiteral(resourceName: "brave_rewards_button_disabled"), for: .normal)
            return
        }
        
        setImage(#imageLiteral(resourceName: "brave_rewards_button_enabled"), for: .normal)
        
        // if notificationCount > 0 { show badge with number }
        
        if isVerified {
            checkmarkView.isHidden = false
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) { fatalError() }
}
