// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class ReportBrokenSiteView: UIStackView, Themeable {
    
    private let titleLabel = UILabel().then {
        $0.text = Strings.Shields.reportABrokenSite
        $0.font = .systemFont(ofSize: 24.0)
        $0.numberOfLines = 0
    }
    
    private let bodyLabelOne = UILabel().then {
        $0.text = Strings.Shields.reportBrokenSiteBody1
        $0.font = .systemFont(ofSize: 16.0)
        $0.numberOfLines = 0
    }
    
    private let bodyLabelTwo = UILabel().then {
        $0.text = Strings.Shields.reportBrokenSiteBody2
        $0.font = .systemFont(ofSize: 16.0)
        $0.numberOfLines = 0
    }
    
    let urlLabel = UILabel().then {
        $0.appearanceTextColor = BraveUX.braveOrange
        $0.font = .systemFont(ofSize: 16.0)
        $0.numberOfLines = 0
    }
    
    let cancelButton = ActionButton(type: .system).then {
        $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
        $0.titleEdgeInsets = UIEdgeInsets(top: 4, left: 20, bottom: 4, right: 20)
        $0.setTitle(Strings.cancelButtonTitle, for: .normal)
    }
    
    let submitButton = ActionButton(type: .system).then {
        $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
        $0.titleEdgeInsets = UIEdgeInsets(top: 4, left: 20, bottom: 4, right: 20)
        $0.backgroundColor = BraveUX.braveOrange
        $0.setTitleColor(.white, for: .normal)
        $0.setTitle(Strings.Shields.reportBrokenSubmitButtonTitle, for: .normal)
        $0.layer.borderWidth = 0
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        axis = .vertical
        layoutMargins = UIEdgeInsets(equalInset: 30)
        isLayoutMarginsRelativeArrangement = true
        spacing = 12
        
        addStackViewItems(
            .view(titleLabel),
            .customSpace(16),
            .view(bodyLabelOne),
            .view(urlLabel),
            .view(bodyLabelTwo),
            .view(UIStackView().then {
                $0.spacing = 10
                $0.addStackViewItems(
                    .view(UIView()), // spacer
                    .view(cancelButton),
                    .view(submitButton)
                )
            })
        )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func applyTheme(_ theme: Theme) {
        titleLabel.textColor = theme.colors.tints.home
        bodyLabelOne.textColor = theme.colors.tints.home
        bodyLabelTwo.textColor = theme.colors.tints.home
        cancelButton.tintColor = theme.isDark ? Colors.grey200 : Colors.grey800
    }
}
