// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
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
        
        let agreeButton = CommonViews.primaryButton(text: Strings.OBAgreeButton).then {
            $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.AgreeButton"
            $0.backgroundColor = BraveUX.BraveOrange.withAlphaComponent(0.7)
            $0.isEnabled = false
        }
        
        let cancelButton = CommonViews.secondaryButton(text: Strings.CancelButtonTitle).then {
            $0.accessibilityIdentifier = "OnboardingRewardsAgreementViewController.CancelButton"
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
        
        private let descriptionCheckbox = UIButton().then {
            $0.setImage(#imageLiteral(resourceName: "checkbox_off"), for: .normal)
            $0.setImage(#imageLiteral(resourceName: "checkbox_on"), for: .selected)
            $0.setImage(#imageLiteral(resourceName: "checkbox_on"), for: .highlighted)
            $0.adjustsImageWhenHighlighted = true
            
            $0.contentMode = .scaleAspectFit
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        }
        
        private let titleLabel = CommonViews.primaryText(Strings.OBRewardsAgreementTitle).then {
            $0.numberOfLines = 0
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
            let descriptionStackView =  UIStackView(arrangedSubviews: [descriptionLabel, descriptionCheckbox]).then {
                $0.alignment = .center
                $0.spacing = 65.0
            }
            
            [titleLabel, descriptionStackView].forEach(stackView.addArrangedSubview(_:))
        }
        
        private let buttonsStackView = UIStackView().then {
            $0.distribution = .equalCentering
        }
        
        private func updateDescriptionLabel() {
            descriptionLabel.attributedText = {
                let titleLabelColor = titleLabel.textColor ?? .black
                
                let text = NSMutableAttributedString(string: Strings.OBRewardsAgreementDetail, attributes: [
                    .font: UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ])
                
                text.append(NSAttributedString(string: " ", attributes: [
                    .font: UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ]))
                
                text.append(NSAttributedString(string: Strings.OBRewardsAgreementDetailLink, attributes: [
                    .font: UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular),
                    .foregroundColor: UX.linkColor,
                    .link: "brave_terms_of_service"
                ]))
                
                text.append(NSAttributedString(string: ".", attributes: [
                    .font: UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.regular),
                    .foregroundColor: titleLabelColor
                ]))
                
                let paragraphStyle = NSMutableParagraphStyle()
                paragraphStyle.lineBreakMode = .byWordWrapping
                
                text.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSRange(location: 0, length: text.length))
                
                return text
            }()
            
            descriptionLabel.accessibilityLabel = descriptionLabel.text
            descriptionLabel.accessibilityTraits = [.staticText, .link]
            descriptionLabel.accessibilityValue = nil
            descriptionLabel.isAccessibilityElement = true
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
                $0.leading.equalTo(self.safeArea.leading)
                $0.trailing.equalTo(self.safeArea.trailing)
                $0.bottom.equalTo(self.safeArea.bottom)
            }
            
            descriptionView.addSubview(descriptionStackView)
            descriptionStackView.snp.makeConstraints {
                $0.edges.equalToSuperview().inset(UX.descriptionContentInset)
            }
            
            mainStackView.addArrangedSubview(descriptionView)

            [cancelButton, agreeButton, UIView.spacer(.horizontal, amount: 0)]
                .forEach(buttonsStackView.addArrangedSubview(_:))
            
            [textStackView, buttonsStackView].forEach(descriptionStackView.addArrangedSubview(_:))
            
            descriptionCheckbox.addTarget(self, action: #selector(onTermsAccepted(_:)), for: .touchUpInside)
        }
        
        func applyTheme(_ theme: Theme) {
            descriptionView.backgroundColor = OnboardingViewController.colorForTheme(theme)
            titleLabel.appearanceTextColor = theme.colors.tints.home
            updateDescriptionLabel()
            descriptionCheckbox.setImage(theme.isDark ? #imageLiteral(resourceName: "checkbox_off_dark") : #imageLiteral(resourceName: "checkbox_off"), for: .normal)
        }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            
            let size = imageView.intrinsicContentSize
            let scaleFactor = bounds.width / size.width
            let newSize = CGSize(width: size.width * scaleFactor, height: size.height * scaleFactor)
            
            imageView.frame = CGRect(x: 0.0, y: UX.animationContentInset, width: newSize.width, height: newSize.height)
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        @objc
        private func onTermsAccepted(_ button: UIButton) {
            button.isSelected.toggle()
            
            agreeButton.backgroundColor = button.isSelected ? BraveUX.BraveOrange : BraveUX.BraveOrange.withAlphaComponent(0.7)
            agreeButton.isEnabled = button.isSelected
        }
        
        override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
            if !descriptionCheckbox.isHidden &&
                descriptionCheckbox.isUserInteractionEnabled &&
                descriptionCheckbox.alpha >= 0.01,
                let frame = descriptionCheckbox.superview?.convert(
                    descriptionCheckbox.frame,
                    to: self
                ) {
                
                if frame.inset(by: UIEdgeInsets(equalInset: UX.checkboxInsets)).contains(point) {
                    return descriptionCheckbox
                }
            }
            
            return super.hitTest(point, with: event)
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
    func boldFirstWord(with font: UIFont) -> NSMutableAttributedString {
        let mutableDescriptionText = NSMutableAttributedString(string: self)
        
        if let firstWord = self.components(separatedBy: " ").first {
            if let range = self.range(of: firstWord) {
                let nsRange = NSRange(range, in: self)
                let font = UIFont.systemFont(ofSize: font.pointSize, weight: UIFont.Weight.bold)
                
                mutableDescriptionText.addAttribute(NSAttributedString.Key.font, value: font, range: nsRange)
            }
        }
        
        return mutableDescriptionText
    }
}
