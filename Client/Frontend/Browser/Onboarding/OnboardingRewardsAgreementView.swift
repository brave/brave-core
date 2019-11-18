// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveRewards
import Lottie

extension OnboardingRewardsAgreementViewController {
    
    private struct UX {
        /// A negative spacing is needed to make rounded corners for details view visible.
        static let negativeSpacing: CGFloat = -16
        static let descriptionContentInset: CGFloat = 32
        static let linkColor: UIColor = BraveUX.BraveOrange
        static let animationContentInset: CGFloat = 50.0
        static let checkboxInsets: CGFloat = -44.0
    }
    
    class View: UIView {
        
        var onTermsOfServicePressed: (() -> Void)?
        
        let turnOnButton = CommonViews.primaryButton(text: Strings.OBTurnOnButton).then {
            $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.OBTurnOnButton"
            $0.backgroundColor = BraveUX.BraveOrange
        }
        
        let skipButton = CommonViews.secondaryButton(text: Strings.OBSkipButton).then {
            $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.OBSkipButton"
        }
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = UX.negativeSpacing
        }
        
        let imageView = AnimationView(name: "onboarding-rewards").then {
            $0.contentMode = .scaleAspectFit
            $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
            $0.backgroundBehavior = .pauseAndRestore
            $0.loopMode = .loop
            $0.play()
        }
        
        private let descriptionView = UIView().then {
            $0.layer.cornerRadius = 12
            $0.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
        }
        
        private let descriptionStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 32
        }
        
        private let titleLabel = CommonViews.primaryText(Strings.OBRewardsAgreementTitle).then {
            $0.numberOfLines = 0
        }
        
        private let subtitleLabel = CommonViews.secondaryText("").then {
            let adSupportedRegionText = Preferences.Rewards.isUsingBAP.value == true ? Strings.OBRewardsDetailInAdRegionJapan : Strings.OBRewardsDetailInAdRegion
            $0.attributedText = BraveAds.isCurrentLocaleSupported() ?  adSupportedRegionText.boldWords(with: $0.font, amount: 2) : Strings.OBRewardsDetailOutsideAdRegion.boldWords(with: $0.font, amount: 1)
        }
        
        private lazy var descriptionLabel = UITextView().then {
            $0.delaysContentTouches = false
            $0.isEditable = false
            $0.isScrollEnabled = false
            $0.isSelectable = true
            $0.backgroundColor = .clear
            $0.textDragInteraction?.isEnabled = false
            $0.textContainerInset = .zero
            $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular)
            $0.delegate = self
            
            $0.linkTextAttributes = [
              .font: $0.font!,
              .foregroundColor: UX.linkColor,
              .underlineStyle: 0
            ]
            
            $0.textAlignment = .center
        }
        
        private lazy var textStackView = UIStackView().then { stackView in
            stackView.axis = .vertical
            stackView.spacing = 8
            stackView.layoutMargins = UIEdgeInsets(top: 0.0, left: 20.0, bottom: 0.0, right: 20.0)
            stackView.isLayoutMarginsRelativeArrangement = true

            [titleLabel, subtitleLabel, descriptionLabel].forEach {
                stackView.addArrangedSubview($0)
            }
        }
        
        private let buttonsStackView = UIStackView().then {
            $0.distribution = .equalCentering
        }
        
        private func updateDescriptionLabel() {
            descriptionLabel.attributedText = {
                let fontSize: CGFloat = 14.0
                let titleLabelColor = titleLabel.textColor ?? .black
                
                let text = NSMutableAttributedString(string: Strings.OBRewardsAgreementDetail, attributes: [
                    .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ])
                
                text.append(NSAttributedString(string: " ", attributes: [
                    .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ]))
                
                text.append(NSAttributedString(string: Strings.OBRewardsAgreementDetailLink, attributes: [
                    .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
                    .foregroundColor: UX.linkColor,
                    .link: "brave_terms_of_service"
                ]))
                
                text.append(NSAttributedString(string: ".", attributes: [
                    .font: UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ]))
                
                let paragraphStyle = NSMutableParagraphStyle()
                paragraphStyle.lineBreakMode = .byWordWrapping
                paragraphStyle.alignment = .center
                
                text.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSRange(location: 0, length: text.length))
                
                return text
            }()
            
            descriptionLabel.accessibilityLabel = descriptionLabel.text
            descriptionLabel.accessibilityTraits = [.staticText, .link]
            descriptionLabel.accessibilityValue = nil
            descriptionLabel.isAccessibilityElement = true
        }
        
        func updateSubtitleText(_ text: String, boldWords: Int) {
            self.subtitleLabel.attributedText = text.boldWords(with: self.subtitleLabel.font, amount: boldWords)
        }
        
        init(theme: Theme) {
            super.init(frame: .zero)
            
            applyTheme(theme)
            mainStackView.tag = OnboardingViewAnimationID.details.rawValue
            descriptionStackView.tag = OnboardingViewAnimationID.detailsContent.rawValue
            imageView.tag = OnboardingViewAnimationID.background.rawValue
            
            addSubview(imageView)
            addSubview(mainStackView)
            mainStackView.snp.makeConstraints {
                $0.leading.trailing.bottom.equalToSuperview()
            }
            
            descriptionView.addSubview(descriptionStackView)
            descriptionStackView.snp.makeConstraints {
                $0.edges.equalTo(descriptionView.safeArea.edges).inset(UX.descriptionContentInset)
            }
            
            mainStackView.addArrangedSubview(descriptionView)

            [skipButton, turnOnButton, UIView.spacer(.horizontal, amount: 0)]
                .forEach(buttonsStackView.addArrangedSubview(_:))
            
            [textStackView, buttonsStackView].forEach(descriptionStackView.addArrangedSubview(_:))
            
            turnOnButton.snp.makeConstraints {
                $0.centerX.equalTo(self.snp.centerX)
            }
        }
        
        func applyTheme(_ theme: Theme) {
            descriptionView.backgroundColor = OnboardingViewController.colorForTheme(theme)
            titleLabel.appearanceTextColor = theme.colors.tints.home
            updateDescriptionLabel()
        }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            
            let size = imageView.intrinsicContentSize
            let scaleFactor = bounds.width / size.width
            let newSize = CGSize(width: size.width * scaleFactor, height: size.height * scaleFactor)
            
            imageView.frame = CGRect(x: 0.0, y: UX.animationContentInset, width: newSize.width, height: newSize.height)
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
}

extension OnboardingRewardsAgreementViewController.View: UITextViewDelegate {
    @objc
    func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange, interaction: UITextItemInteraction) -> Bool {
        
        onTermsOfServicePressed?()
        return false
    }
}

private extension String {
    func boldWords(with font: UIFont, amount: Int) -> NSMutableAttributedString {
        let mutableDescriptionText = NSMutableAttributedString(string: self)
        
        let components = self.components(separatedBy: " ")
        for i in 0..<min(amount, components.count) {
            if let range = self.range(of: components[i]) {
                let nsRange = NSRange(range, in: self)
                let font = UIFont.systemFont(ofSize: font.pointSize, weight: UIFont.Weight.bold)
                
                mutableDescriptionText.addAttribute(NSAttributedString.Key.font, value: font, range: nsRange)
            }
        }
        
        return mutableDescriptionText
    }
}

