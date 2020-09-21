// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class SiteReportedView: UIStackView, Themeable {
    
    let titleLabel = UILabel().then {
        $0.text = Strings.Shields.siteReportedTitle
        $0.font = .systemFont(ofSize: 24)
        $0.numberOfLines = 0
    }
    
    let bodyLabel = UILabel().then {
        $0.text = Strings.Shields.siteReportedBody
        $0.font = .systemFont(ofSize: 17)
        $0.numberOfLines = 0
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        axis = .vertical
        spacing = 16
        
        layoutMargins = UIEdgeInsets(equalInset: 30)
        isLayoutMarginsRelativeArrangement = true
        
        addStackViewItems(
            .view(UIStackView().then {
                $0.spacing = 16
                $0.addStackViewItems(
                    .view(UIImageView(image: UIImage(imageLiteralResourceName: "check-circle")).then {
                        $0.setContentHuggingPriority(.required, for: .horizontal)
                        $0.setContentHuggingPriority(.required, for: .vertical)
                    }),
                    .view(titleLabel)
                )
            }),
            .view(bodyLabel)
        )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func applyTheme(_ theme: Theme) {
        titleLabel.textColor = theme.colors.tints.home
        bodyLabel.textColor = theme.colors.tints.home
    }
}
