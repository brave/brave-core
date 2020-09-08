// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import pop
import SnapKit

extension OnboardingPrivacyConsentViewController {
    
    class View: UIView {
        
        let yesConsentButton = CommonViews.primaryButton(text: Strings.OBPrivacyConsentYesButton).then {
            $0.accessibilityIdentifier = "OnboardingPrivacyConsentViewController.YesButton"
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
        }
        
        let noConsentButton = CommonViews.secondaryButton(text: Strings.OBPrivacyConsentNoButton).then {
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
        
        private let titleLabel = CommonViews.primaryText(Strings.OBPrivacyConsentTitle).then {
            $0.font = .systemFont(ofSize: 20, weight: .semibold)
            $0.numberOfLines = 0
        }
        
        private let refProgramLabel = UILabel().then {
            $0.text = Strings.OBPrivacyConsentDetail
            $0.font = .systemFont(ofSize: 16, weight: .regular)
            $0.numberOfLines = 0
            $0.textAlignment = .left
            $0.minimumScaleFactor = 0.7
        }
        
        init(theme: Theme) {
            super.init(frame: .zero)
            
            mainStackView.tag = OnboardingViewAnimationID.detailsContent.rawValue
            braveLogo.tag = OnboardingViewAnimationID.background.rawValue
            
            applyTheme(theme)
            
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
        
        func applyTheme(_ theme: Theme) {
            backgroundColor = OnboardingViewController.colorForTheme(theme)
            titleLabel.appearanceTextColor = theme.colors.tints.home
            refProgramLabel.appearanceTextColor = theme.colors.tints.home
        }
    }
}
