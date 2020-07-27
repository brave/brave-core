// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class EnableVPNSettingHeader: UIView {
    
    var enableVPNTapped: (() -> Void)?
    var dismissHeaderTapped: (() -> Void)?
    
    private let mainStackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 6
    }
    
    private let titleLabel = UILabel().then {
        $0.text = Strings.VPN.vpnName
        $0.appearanceTextColor = .white
        $0.font = UIFont.systemFont(ofSize: 19, weight: .semibold)
    }
    
    private let bodyLabel = UILabel().then {
        $0.text = Strings.VPN.settingHeaderBody
        $0.numberOfLines = 0
        $0.textAlignment = .center
        $0.appearanceTextColor = .white
        $0.font = UIFont.systemFont(ofSize: 15, weight: .regular)
        $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    }
    
    private let enableButton = RoundInterfaceButton(type: .roundedRect).then {
        
        let title = { () -> String in
            switch BraveVPN.vpnState {
            case .notPurchased, .expired:
                return Strings.learnMore
            case .purchased, .installed:
                return Strings.VPN.enableButton
            }
        }()
        
        $0.setTitle(title, for: .normal)
        $0.backgroundColor = BraveUX.braveOrange
        $0.titleLabel?.font = UIFont.systemFont(ofSize: 16, weight: .semibold)
        $0.appearanceTextColor = .white
        $0.snp.makeConstraints { make in
            make.height.equalTo(44)
            make.width.greaterThanOrEqualTo(120)
        }
        $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 25, bottom: 0, right: 25)
        $0.setContentCompressionResistancePriority(.required, for: .vertical)
    }
    
    private let poweredByStackView = BraveVPNCommonUI.Views.poweredByView(textColor: .white, imageColor: .white)
    
    private let contentView = UIView().then {
        $0.clipsToBounds = true
        $0.layer.cornerRadius = 16
        $0.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor
    }
    
    private let backgroundImage = UIImageView(image: #imageLiteral(resourceName: "enable_vpn_settings_banner")).then {
        $0.contentMode = .scaleAspectFill
    }
    
    private let closeButton = UIButton().then {
        $0.setImage(#imageLiteral(resourceName: "close_popup").template, for: .normal)
        $0.appearanceTintColor = .lightGray
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        contentView.addSubview(backgroundImage)
        backgroundImage.snp.makeConstraints { $0.edges.equalToSuperview() }
        
        addSubview(contentView)
        
        [titleLabel, bodyLabel, enableButton, poweredByStackView].forEach(mainStackView.addArrangedSubview(_:))
        
        contentView.addSubview(mainStackView)
        mainStackView.snp.makeConstraints {
            $0.top.bottom.equalToSuperview().inset(16)
            $0.centerX.equalToSuperview()
            $0.width.equalTo(250)
        }
        
        contentView.snp.makeConstraints {
            $0.top.leading.trailing.equalToSuperview().inset(8)
            $0.bottom.equalToSuperview()
        }
        
        addSubview(closeButton)
        closeButton.snp.makeConstraints {
            $0.top.trailing.equalToSuperview().inset(8)
            $0.size.equalTo(44)
        }
        
        enableButton.addTarget(self, action: #selector(enableVPNAction), for: .touchUpInside)
        closeButton.addTarget(self, action: #selector(closeView), for: .touchUpInside)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    @objc func enableVPNAction() {
        enableVPNTapped?()
    }
    
    @objc func closeView() {
        dismissHeaderTapped?()
    }
}
