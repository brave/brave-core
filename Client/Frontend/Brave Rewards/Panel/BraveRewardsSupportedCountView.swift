// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

class BraveRewardsSupportedCountView: UIStackView, Themeable {
    
    let countLabel = UILabel().then {
        $0.text = "0"
        $0.font = .systemFont(ofSize: 36)
        $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    private let bodyLabel = UILabel().then {
        $0.text = Strings.Rewards.totalSupportedCount
        $0.numberOfLines = 0
        $0.font = .systemFont(ofSize: 15)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        spacing = 16
        alignment = .center
        
        addStackViewItems(
            .view(countLabel),
            .view(bodyLabel)
        )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func applyTheme(_ theme: Theme) {
        let isDark = theme.isDark
        countLabel.appearanceTextColor = isDark ? UIColor.white : UIColor.black
        bodyLabel.appearanceTextColor = isDark ? UIColor.white : UIColor.black
    }
}
