// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

extension InstallVPNViewController {
    class View: UIView {
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.translatesAutoresizingMaskIntoConstraints = false
            $0.spacing = 30
        }
        
        private let imageView = UIView().then {
            $0.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor
            
            let image = UIImageView(image: #imageLiteral(resourceName: "install_vpn_image")).then { img in
                img.contentMode = .scaleAspectFill
                img.setContentHuggingPriority(.required, for: .vertical)
                img.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
            }
            
            $0.snp.makeConstraints { make in
                make.height.greaterThanOrEqualTo(250)
            }
            
            $0.addSubview(image)
            image.snp.makeConstraints { make in
                make.edges.equalToSuperview()
            }
            
            image.clipsToBounds = true
            
            $0.setContentHuggingPriority(.required, for: .vertical)
            $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        }
        
        private lazy var infoStackView = UIStackView().then {
            
            let contentStackView = UIStackView().then { stackView in
                stackView.axis = .vertical
                stackView.distribution = .equalSpacing
                stackView.spacing = 16
                stackView.alignment = .center
            }
            
            let textStackView = UIStackView().then { stackView in
                stackView.axis = .vertical
                stackView.spacing = 8
                
                let titleLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then { label in
                    label.text = Strings.VPN.installProfileTitle
                    label.textAlignment = .center
                    label.font = .systemFont(ofSize: 20, weight: .medium)
                    label.appearanceTextColor = .black
                    label.minimumScaleFactor = 0.5
                    label.adjustsFontSizeToFitWidth = true
                }
                
                let bodyLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then { label in
                    label.text = Strings.VPN.installProfileBody
                    label.numberOfLines = 0
                    label.font = .systemFont(ofSize: 18, weight: .medium)
                    label.appearanceTextColor = #colorLiteral(red: 0.4745098039, green: 0.4745098039, blue: 0.4745098039, alpha: 1)
                }
                
                [titleLabel, bodyLabel].forEach(stackView.addArrangedSubview(_:))
            }
            
            [textStackView, installVPNButton].forEach(contentStackView.addArrangedSubview(_:))
            
            [UIView.spacer(.horizontal, amount: 30),
             contentStackView,
             UIView.spacer(.horizontal, amount: 30)]
                .forEach($0.addArrangedSubview(_:))
        }
        
        let installVPNButton = Button().then {
            $0.setTitle(Strings.VPN.installProfileButtonText, for: .normal)
            $0.backgroundColor = BraveUX.braveOrange
            $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
            $0.appearanceTextColor = .white
            $0.snp.makeConstraints { make in
                make.height.equalTo(44)
            }
            $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 25, bottom: 0, right: 25)
            $0.layer.cornerRadius = 22
            $0.loaderView = LoaderView(size: .small)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            backgroundColor = .white
            addSubview(mainStackView)
            
            [imageView, infoStackView].forEach(mainStackView.addArrangedSubview(_:))
            
            mainStackView.snp.makeConstraints {
                $0.top.leading.trailing.equalTo(self.safeAreaLayoutGuide)
                $0.bottom.equalTo(self.safeAreaLayoutGuide).inset(30)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
    }
}
