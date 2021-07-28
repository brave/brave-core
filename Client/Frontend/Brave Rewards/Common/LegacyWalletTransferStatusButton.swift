// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveUI

class LegacyWalletTransferStatusButton: UIControl {
    
    let dismissButton = BraveButton(type: .system).then {
        $0.setImage(UIImage(imageLiteralResourceName: "close-medium").template, for: .normal)
        $0.tintColor = .white
        $0.hitTestSlop = UIEdgeInsets(equalInset: -10)
        $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    let titleLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 18, weight: .medium)
        $0.numberOfLines = 0
        $0.textColor = .white
        $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        isAccessibilityElement = true
        accessibilityTraits.insert(.button)
        
        layer.cornerRadius = 8
        layer.cornerCurve = .continuous
        
        backgroundColor = .braveInfoLabel
        
        addSubview(titleLabel)
        addSubview(dismissButton)
        
        accessibilityLabel = titleLabel.text
        
        titleLabel.snp.makeConstraints {
            $0.top.leading.bottom.equalToSuperview().inset(UIEdgeInsets(top: 14, left: 20, bottom: 14, right: 20))
            $0.trailing.equalTo(dismissButton.snp.leading).offset(-14)
        }
        dismissButton.snp.makeConstraints {
            $0.top.trailing.equalToSuperview().inset(5)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override var isHighlighted: Bool {
        didSet {
            UIView.animate(withDuration: 0.15) {
                self.titleLabel.alpha = self.isHighlighted ? 0.4 : 1.0
            }
        }
    }
}
