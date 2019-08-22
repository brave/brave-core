// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import Lottie

extension OnboardingShieldsViewController {
    
    private struct UX {
        /// A negative spacing is needed to make rounded corners for details view visible.
        static let negativeSpacing: CGFloat = -16
        static let descriptionContentInset: CGFloat = 32
    }
    
    class View: UIView {
        
        let finishButton = CommonViews.primaryButton(text: Strings.OBFinishButton).then {
            $0.accessibilityIdentifier = "OnboardingShieldsViewController.FinishButton"
        }
        
        let skipButton = CommonViews.secondaryButton().then {
            $0.accessibilityIdentifier = "OnboardingShieldsViewController.SkipButton"
        }
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = UX.negativeSpacing
        }
        
        let imageView = AnimationView(name: "onboarding-shields").then {
            $0.contentMode = .scaleAspectFit
            $0.backgroundColor = #colorLiteral(red: 0.1176470588, green: 0.1254901961, blue: 0.1607843137, alpha: 1)
            $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
            $0.play()
            $0.loopMode = .loop
        }
        
        private let descriptionView = UIView().then {
            $0.backgroundColor = .white
            $0.layer.cornerRadius = 12
            $0.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
        }
        
        private let descriptionStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 32
        }
        
        private let textStackView = UIStackView().then { stackView in
            stackView.axis = .vertical
            stackView.spacing = 8
            
            let titleLabel = CommonViews.primaryText(Strings.OBShieldsTitle)
            
            let descriptionLabel = CommonViews.secondaryText("").then {
                $0.attributedText = Strings.OBShieldsDetail.boldFirstWord(with: $0.font)
            }
            
            [titleLabel, descriptionLabel].forEach {
                stackView.addArrangedSubview($0)
            }
        }
        
        private let buttonsStackView = UIStackView().then {
            $0.distribution = .equalCentering
        }
        
        init() {
            super.init(frame: .zero)
            
            [imageView, descriptionView].forEach(mainStackView.addArrangedSubview(_:))
            
            [UIView.spacer(.horizontal, amount: 0),
             finishButton,
             UIView.spacer(.horizontal, amount: 0)]
                .forEach(buttonsStackView.addArrangedSubview(_:))
            
            [textStackView, buttonsStackView].forEach(descriptionStackView.addArrangedSubview(_:))
            
            addSubview(mainStackView)
            descriptionView.addSubview(descriptionStackView)
            
            mainStackView.snp.makeConstraints {
                $0.leading.equalTo(self.safeArea.leading)
                $0.trailing.equalTo(self.safeArea.trailing)
                $0.bottom.equalTo(self.safeArea.bottom)
                $0.top.equalTo(self) // extend the view undeneath the safe area/notch
            }
            
            descriptionStackView.snp.makeConstraints {
                $0.edges.equalToSuperview().inset(UX.descriptionContentInset)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
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
