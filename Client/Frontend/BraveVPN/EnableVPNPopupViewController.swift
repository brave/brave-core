// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class EnableVPNPopupViewController: UIViewController {
    
    var enableVPNTapped: (() -> Void)?
    
    private let contentView = ContentView()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let backgroundView = UIView().then {
            $0.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        }
        
        view.insertSubview(backgroundView, at: 0)
        backgroundView.snp.makeConstraints { $0.edges.equalToSuperview() }
        
        view.addSubview(contentView)
        contentView.enableButton.addTarget(self, action: #selector(enableVPNAction), for: .touchUpInside)
        contentView.closeButton.addTarget(self, action: #selector(closeView), for: .touchUpInside)
        
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(backgroundTapped))
        backgroundView.addGestureRecognizer(tapGesture)
    }
    
    override func viewDidLayoutSubviews() {
        contentView.snp.remakeConstraints {
            if traitCollection.horizontalSizeClass == .compact
                && UIApplication.shared.statusBarOrientation.isPortrait {
                $0.leading.trailing.greaterThanOrEqualTo(view).inset(16)
            } else {
                $0.width.lessThanOrEqualTo(400)
            }
            
            $0.centerX.centerY.equalToSuperview()
        }
    }
    
    @objc func enableVPNAction() {
        dismiss(animated: false)
        enableVPNTapped?()
    }
    
    @objc func backgroundTapped() {
        dismiss(animated: false)
    }
    
    @objc func closeView() {
        dismiss(animated: false, completion: nil)
    }
}

private class ContentView: UIView {
    
    private let mainStackView = UIStackView().then {
        $0.axis = .vertical
        $0.distribution = .equalSpacing
        $0.spacing = 24
    }
    
    private let image = UIImageView(image: #imageLiteral(resourceName: "vpn_popup_shield")).then {
        $0.contentMode = .scaleAspectFit
    }
    
    private let titleStackView = UIStackView().then { stackView in
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.alignment = .center
        let titleLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then {
            $0.text = Strings.VPN.vpnName
            $0.appearanceTextColor = .black
            $0.textAlignment = .center
            $0.font = .systemFont(ofSize: 33, weight: .heavy)
        }
        
        let poweredByStackView = BraveVPNCommonUI.Views.poweredByView(textColor: .black, imageColor: .black)
        [titleLabel, poweredByStackView].forEach(stackView.addArrangedSubview(_:))
    }
    
    private lazy var enableButtonStackView = UIStackView().then {
        $0.distribution = .equalSpacing
        
        [UIView.spacer(.horizontal, amount: 1),
         enableButton,
         UIView.spacer(.horizontal, amount: 1)].forEach($0.addArrangedSubview(_:))
    }
    
    let enableButton = RoundInterfaceButton(type: .roundedRect).then {
        $0.setTitle(Strings.learnMore, for: .normal)
        $0.backgroundColor = BraveUX.braveOrange
        $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
        $0.appearanceTextColor = .white
        $0.snp.makeConstraints { make in
            make.height.equalTo(44)
        }
        $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 25, bottom: 0, right: 25)
        $0.setContentCompressionResistancePriority(.required, for: .vertical)
    }
    
    private lazy var checkboxesStackView = UIStackView().then { stackView in
        stackView.axis = .vertical
        stackView.spacing = 4
        
        let checkboxes = [Strings.VPN.checkboxBlockAds,
                          Strings.VPN.popupCheckmarkSecureConnections,
                          Strings.VPN.checkboxFast,
                          Strings.VPN.popupCheckmark247Support]
        
        checkboxes.forEach { checkbox in
            stackView.addArrangedSubview(
                BraveVPNCommonUI.Views.checkmarkView(string: checkbox,
                                                     textColor: .black,
                                                     font: .systemFont(ofSize: 16, weight: .regular),
                                                     useShieldAsCheckmark: false))
        }
    }
    
    let closeButton = UIButton().then {
        $0.setImage(#imageLiteral(resourceName: "close_popup").template, for: .normal)
        $0.appearanceTintColor = .lightGray
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        translatesAutoresizingMaskIntoConstraints = false
        
        backgroundColor = .white
        
        [image, titleStackView, checkboxesStackView, enableButtonStackView]
            .forEach(mainStackView.addArrangedSubview(_:))
        addSubview(mainStackView)
        
        clipsToBounds = true
        layer.cornerRadius = 8
        
        mainStackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(36)
        }
        
        addSubview(closeButton)
        closeButton.snp.makeConstraints {
            $0.top.trailing.equalToSuperview().inset(8)
            $0.size.equalTo(44)
        }
    }
    
    @available(*, unavailable)
    required init?(coder: NSCoder) { fatalError() }
}

