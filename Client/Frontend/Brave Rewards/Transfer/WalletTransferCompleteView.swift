// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

extension WalletTransferCompleteViewController {
    class WalletTransferCompleteView: UIView, Themeable {
        
        private let scrollView = UIScrollView()
        private let stackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 4
            $0.layoutMargins = UIEdgeInsets(equalInset: 16)
            $0.isLayoutMarginsRelativeArrangement = true
        }
        let titleLabel = UILabel().then {
            $0.text = Strings.Rewards.walletTransferCompleteTitle
            $0.numberOfLines = 0
            $0.font = .systemFont(ofSize: 17, weight: .semibold)
            $0.appearanceTextColor = UIColor(rgb: 0x339AF0)
        }
        let bodyLabel = UILabel().then {
            $0.text = Strings.Rewards.walletTransferCompleteBody
            $0.numberOfLines = 0
            $0.font = .systemFont(ofSize: 17)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            addSubview(scrollView)
            scrollView.addSubview(stackView)
            stackView.addStackViewItems(
                .view(titleLabel),
                .view(bodyLabel)
            )
            
            scrollView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            scrollView.contentLayoutGuide.snp.makeConstraints {
                $0.width.equalToSuperview()
                $0.top.bottom.equalTo(stackView)
            }
            stackView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
        
        func applyTheme(_ theme: Theme) {
            backgroundColor = theme.colors.home
            titleLabel.textColor = theme.colors.tints.home
            bodyLabel.textColor = theme.colors.tints.home
        }
    }
}
