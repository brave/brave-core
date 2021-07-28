// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

class LegacyWalletTransferButton: UIControl {
    
    let dismissButton = BraveButton(type: .system).then {
        $0.setImage(UIImage(imageLiteralResourceName: "close-medium").template, for: .normal)
        $0.tintColor = .white
        $0.hitTestSlop = UIEdgeInsets(equalInset: -10)
    }
    
    private let imageView = UIImageView(image: UIImage(imageLiteralResourceName: "rewards-qr-code").template).then {
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.tintColor = .white
    }
    
    private let titleLabel = UILabel().then {
        $0.text = Strings.Rewards.legacyWalletTransfer
        $0.font = .systemFont(ofSize: 18, weight: .medium)
        $0.numberOfLines = 0
        $0.textColor = .white
    }
    
    private let subtitleLabel = UILabel().then {
        $0.text = Strings.Rewards.legacyWalletTransferSubtitle
        $0.font = .systemFont(ofSize: 13)
        $0.numberOfLines = 0
        $0.textColor = .white
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        let stackView = UIStackView().then {
            $0.alignment = .center
            $0.spacing = 14
            $0.isUserInteractionEnabled = false
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(top: 14, left: 20, bottom: 14, right: 20)
        }
        
        isAccessibilityElement = true
        accessibilityTraits.insert(.button)
        
        layer.cornerRadius = 8
        layer.cornerCurve = .continuous
        
        backgroundColor = .braveInfoLabel
        
        addSubview(stackView)
        addSubview(dismissButton)
        
        stackView.addStackViewItems(
            .view(imageView),
            .view(UIStackView().then {
                $0.axis = .vertical
                $0.spacing = 4
                $0.addStackViewItems(
                    .view(titleLabel),
                    .view(subtitleLabel)
                )
            })
        )
        
        accessibilityLabel = titleLabel.text
        
        stackView.snp.makeConstraints {
            $0.top.leading.bottom.equalToSuperview()
            $0.trailing.equalTo(dismissButton)
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
                self.subtitleLabel.alpha = self.isHighlighted ? 0.4 : 1.0
                self.imageView.alpha = self.isHighlighted ? 0.4 : 1.0
            }
        }
    }
}
