// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import Shared

private let log = Logger.browserLogger

class OnboardingPrivacyConsentViewController: UIViewController {
    
    private struct UX {
        /// The onboarding screens are showing as a modal on iPads.
        static let preferredModalSize = CGSize(width: 375, height: 667)
    }
    
    var handleReferralLookup: ((_ urp: UserReferralProgram, _ checkClipboard: Bool) -> Void)?
    var onPrivacyConsentCompleted: (() -> Void)?

    private let contentView = View().then {
        $0.layer.cornerCurve = .continuous
        $0.layer.cornerRadius = 10.0
        $0.layer.masksToBounds = true
    }
    
    init() {
        super.init(nibName: nil, bundle: nil)
        modalPresentationStyle = .fullScreen
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.backgroundColor = #colorLiteral(red: 0.1176470588, green: 0.1254901961, blue: 0.1607843137, alpha: 1)
        view.addSubview(contentView)
        
        if UIDevice.current.userInterfaceIdiom == .phone {
            contentView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
        } else {
            contentView.snp.makeConstraints {
                $0.leading.greaterThanOrEqualToSuperview()
                $0.trailing.lessThanOrEqualToSuperview()
                $0.top.greaterThanOrEqualToSuperview()
                $0.bottom.lessThanOrEqualToSuperview()
                $0.center.equalToSuperview()
                $0.width.equalTo(UX.preferredModalSize.width)
                $0.height.equalTo(UX.preferredModalSize.height)
            }
        }
        
        contentView.yesConsentButton.addTarget(self, action: #selector(yesConsentTapped), for: .touchUpInside)
        contentView.noConsentButton.addTarget(self, action: #selector(noConsentTaapped), for: .touchUpInside)
    }
    
    @objc func yesConsentTapped() {
        presentNextScreen(withPrivacyConsent: true)
    }
    
    @objc func noConsentTaapped() {
        presentNextScreen(withPrivacyConsent: false)
    }
    
    private func presentNextScreen(withPrivacyConsent: Bool) {
        Preferences.General.basicOnboardingProgress.value = OnboardingProgress.privacyConsent.rawValue
        if let urp = UserReferralProgram.shared {
            handleReferralLookup?(urp, withPrivacyConsent)
        }
        
        onPrivacyConsentCompleted?()
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
            return .default
        }
        
        override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
            return .portrait
        }
        
        override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
            return .portrait
        }
        
        override var shouldAutorotate: Bool {
            return false
        }
}

extension OnboardingPrivacyConsentViewController {
    class View: UIView {
        let yesConsentButton = OnboardingCommon.Views.primaryButton(text: Strings.OBPrivacyConsentYesButton).then {
            $0.accessibilityIdentifier = "OnboardingPrivacyConsentViewController.YesButton"
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
        }
        
        let noConsentButton = OnboardingCommon.Views.secondaryButton(text: Strings.OBPrivacyConsentNoButton).then {
            $0.accessibilityIdentifier = "OnboardingPrivacyConsentViewController.NoButton"
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
        }
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.distribution = .equalSpacing
        }
        
        private let braveLogo = UIImageView(image: #imageLiteral(resourceName: "browser_lock_popup")).then {
            $0.contentMode = .scaleAspectFit
        }
        
        private let titleLabel = OnboardingCommon.Views.primaryText(Strings.OBPrivacyConsentTitle).then {
            $0.font = .systemFont(ofSize: 20, weight: .semibold)
            $0.textColor = .braveLabel
            $0.numberOfLines = 0
        }
        
        private let refProgramLabel = UILabel().then {
            $0.text = Strings.OBPrivacyConsentDetail
            $0.textColor = .braveLabel
            $0.font = .systemFont(ofSize: 16, weight: .regular)
            $0.numberOfLines = 0
            $0.textAlignment = .left
            $0.minimumScaleFactor = 0.7
        }
        
        init() {
            super.init(frame: .zero)
            
            backgroundColor = .braveBackground

            mainStackView.addStackViewItems(
                .view(.spacer(.vertical, amount: 1)),
                .view(braveLogo),
                .view(
                    UIStackView(arrangedSubviews: [titleLabel, refProgramLabel]).then {
                        $0.axis = .vertical
                        $0.spacing = 24
                }),
                .view(UIStackView(arrangedSubviews: [yesConsentButton, noConsentButton]).then {
                    $0.axis = .vertical
                    $0.spacing = 8
                })
            )
            
            addSubview(mainStackView)
            
            mainStackView.snp.makeConstraints {
                $0.top.equalTo(safeArea.top).inset(24)
                $0.bottom.equalTo(safeArea.bottom).inset(16)
                $0.leading.trailing.equalToSuperview().inset(25)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
    }
}
