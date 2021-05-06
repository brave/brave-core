// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

extension WalletTransferViewController {
    class WalletTransferView: UIView {
        let cameraView = SyncCameraView().then {
            $0.backgroundColor = .black
            $0.layer.cornerRadius = 4
            $0.layer.cornerCurve = .continuous
        }
        
        private let scrollView = UIScrollView()
        private let stackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 6
            $0.alignment = .leading
        }
        private let titleLabel = UILabel().then {
            $0.text = Strings.Rewards.walletTransferStepsTitle
            $0.font = .systemFont(ofSize: 17, weight: .semibold)
            $0.numberOfLines = 0
            $0.textColor = .braveLabel
        }
        private let bodyLabel = UILabel().then {
            $0.text = Strings.Rewards.walletTransferStepsBody
            $0.font = .systemFont(ofSize: 17)
            $0.numberOfLines = 0
            $0.textColor = .braveLabel
        }
        let learnMoreButton = UIButton(type: .system).then {
            $0.setTitle(Strings.learnMore, for: .normal)
            $0.setTitleColor(.braveBlurple, for: .normal)
            $0.titleLabel?.font = .systemFont(ofSize: 17)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .secondaryBraveBackground
            
            addSubview(cameraView)
            addSubview(scrollView)
            scrollView.addSubview(stackView)
            stackView.addStackViewItems(
                .view(titleLabel),
                .view(bodyLabel),
                .customSpace(20),
                .view(learnMoreButton)
            )
            
            cameraView.snp.makeConstraints {
                $0.top.equalTo(self.safeAreaLayoutGuide).inset(10)
                $0.leading.greaterThanOrEqualTo(self.safeAreaLayoutGuide).inset(10)
                $0.trailing.lessThanOrEqualTo(self.safeAreaLayoutGuide).inset(10)
                $0.centerX.equalToSuperview()
                $0.height.equalTo(cameraView.snp.width)
                $0.width.lessThanOrEqualTo(375)
            }
            
            scrollView.snp.makeConstraints {
                $0.top.equalTo(cameraView.snp.bottom).offset(10)
                $0.leading.trailing.bottom.equalToSuperview()
            }
            scrollView.contentLayoutGuide.snp.makeConstraints {
                $0.top.bottom.equalTo(stackView)
                $0.width.equalToSuperview()
            }
            stackView.snp.makeConstraints {
                $0.edges.equalToSuperview().inset(10)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
}
