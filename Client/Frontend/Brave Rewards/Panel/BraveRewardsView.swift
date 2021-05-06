// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import BraveUI
import Shared

extension BraveRewardsViewController {
    class BraveRewardsView: UIView {
        
        private let stackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 20
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(top: 20, left: 16, bottom: 20, right: 16)
        }
        let rewardsToggle = UISwitch().then {
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.onTintColor = .braveOrange
        }
        private let titleLabel = UILabel().then {
            $0.text = Strings.braveRewardsTitle
            $0.textColor = .braveLabel
            $0.font = .systemFont(ofSize: 20)
        }
        let subtitleLabel = UILabel().then {
            $0.text = Strings.Rewards.disabledBody
            $0.textColor = .secondaryBraveLabel
            $0.font = .systemFont(ofSize: 12)
        }
        
        let publisherView = BraveRewardsPublisherView().then {
            $0.isHidden = true
        }
        let statusView = BraveRewardsStatusView()
        let legacyWalletTransferButton = LegacyWalletTransferButton().then {
            $0.isHidden = true
        }
        let legacyWalletTransferStatusButton = LegacyWalletTransferStatusButton().then {
            $0.isHidden = true
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .braveBackground
            
            addSubview(stackView)
            stackView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            stackView.addStackViewItems(
                .view(UIStackView().then {
                    $0.alignment = .center
                    $0.spacing = 12
                    $0.addStackViewItems(
                        .view(UIStackView().then {
                            $0.axis = .vertical
                            $0.spacing = 4
                            $0.addStackViewItems(
                                .view(titleLabel),
                                .view(subtitleLabel)
                            )
                            $0.setContentHuggingPriority(.required, for: .vertical)
                        }),
                        .view(rewardsToggle)
                    )
                }),
                .view(UIStackView().then {
                    $0.axis = .vertical
                    $0.spacing = 8
                    $0.addStackViewItems(
                        .view(legacyWalletTransferButton),
                        .view(legacyWalletTransferStatusButton),
                        .view(statusView)
                    )
                }),
                .view(publisherView)
            )
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
}
