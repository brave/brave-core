// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class RewardsButton: UIButton {
    
    var isVerified = false {
        didSet { updateView() }
    }
    
    var notificationCount = 0 {
        didSet { updateView() }
    }
    
    var forceShowBadge = false {
        didSet { updateView() }
    }
    
    private let notificationsBadgeView = UIView().then {
        $0.backgroundColor = BraveUX.BraveOrange
        $0.frame = CGRect(x: 19, y: 5, width: 12, height: 12)
        $0.layer.cornerRadius = 6
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
        addSubview(notificationsBadgeView)
        updateView()
    }
    
    private func updateView() {
        checkmarkView.isHidden = true
        notificationsBadgeView.isHidden = true
        
        setImage(#imageLiteral(resourceName: "brave_rewards_button_enabled"), for: .normal)
        
        if notificationCount > 0 || forceShowBadge {
            notificationsBadgeView.isHidden = false
        } else if isVerified {
            checkmarkView.isHidden = false
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) { fatalError() }
}
