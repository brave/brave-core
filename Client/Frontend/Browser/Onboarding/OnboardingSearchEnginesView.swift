// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import pop
import SnapKit

extension OnboardingSearchEnginesViewController {
    
    private struct UX {
        static var topInset: CGFloat = 48
        static let contentInset: CGFloat = 25
        static var logoSizeAfterAnimation: CGFloat = 100
        static let logoSizeBeforeAnimation: CGFloat = 150
        
        struct SearchEngineCell {
            static let rowHeight: CGFloat = 54
            static let imageSize: CGFloat = 32
            static let cornerRadius: CGFloat = 8
        }
    }
    
    class View: UIView {
        
        let searchEnginesTable = UITableView().then {
            $0.separatorStyle = .none
            $0.allowsMultipleSelection = false
            $0.alwaysBounceVertical = false
            $0.showsVerticalScrollIndicator = true
            $0.backgroundColor = .braveBackground
            #if swift(>=5.5)
            if #available(iOS 15.0, *) {
                $0.sectionHeaderTopPadding = 0
            }
            #endif
        }
        
        let continueButton = CommonViews.primaryButton(text: Strings.OBSaveButton).then {
            $0.accessibilityIdentifier = "OnboardingSearchEnginesViewController.SaveButton"
            $0.titleLabel?.minimumScaleFactor = 0.75
        }
        
        let skipButton = CommonViews.secondaryButton().then {
            $0.accessibilityIdentifier = "OnboardingSearchEnginesViewController.SkipButton"
            $0.titleLabel?.minimumScaleFactor = 0.75
        }
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.alignment = .fill
            $0.spacing = 20
            $0.translatesAutoresizingMaskIntoConstraints = false
        }
        
        private let braveLogo = UIImageView(image: #imageLiteral(resourceName: "browser_lock_popup")).then { logo in
            logo.contentMode = .scaleAspectFit
        }
        
        let titlePrimary = CommonViews.primaryText(Strings.OBSearchEngineTitle).then {
            $0.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
            $0.textColor = .braveLabel
        }
        
        let titleSecondary = CommonViews.secondaryText(Strings.OBSearchEngineDetail).then {
            $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.medium)
            $0.textColor = .braveLabel
        }
        
        private let titleStackView = UIStackView().then { stackView in
            stackView.spacing = 10
            stackView.axis = .vertical
        }
        
        private let buttonsStackView = UIStackView().then {
            $0.axis = .horizontal
            $0.alignment = .center
            $0.spacing = 15.0
        }
        
        private let containerView = UIView().then {
            $0.backgroundColor = .braveBackground
        }
        
        private var logoCenterY: Constraint?
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            containerView.tag = OnboardingViewAnimationID.details.rawValue
            mainStackView.tag = OnboardingViewAnimationID.detailsContent.rawValue
            braveLogo.tag = OnboardingViewAnimationID.background.rawValue
            
            addSubview(containerView)
            containerView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            
            containerView.addSubview(braveLogo)
            [titlePrimary, titleSecondary].forEach(titleStackView.addArrangedSubview(_:))
            
            [skipButton, continueButton]
                .forEach(buttonsStackView.addArrangedSubview(_:))
            
            [titleStackView, searchEnginesTable, buttonsStackView]
                .forEach(mainStackView.addArrangedSubview(_:))
            
            containerView.addSubview(mainStackView)
            
            braveLogo.snp.makeConstraints {
                $0.centerX.equalToSuperview()
                logoCenterY = $0.centerY.equalToSuperview().constraint
                $0.size.equalTo(UX.logoSizeBeforeAnimation)
            }

            mainStackView.snp.makeConstraints {
                $0.top.equalTo(braveLogo.snp.bottom).offset(30)
                $0.leading.equalTo(containerView.safeArea.leading).inset(UX.contentInset)
                $0.trailing.equalTo(containerView.safeArea.trailing).inset(UX.contentInset)
                $0.bottom.equalTo(containerView.safeArea.bottom).inset(UX.contentInset)
            }
            
            // Hiding views in prepration to animations.
            // Alpha is used instead of `isHidden` to make the views participate in auto-layout.
            [titlePrimary, titleSecondary, searchEnginesTable, buttonsStackView].forEach {
                $0.alpha = CGFloat.leastNormalMagnitude
            }
            
            skipButton.snp.makeConstraints {
                $0.width.equalTo(continueButton.snp.width).priority(.low)
            }
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                if self.bounds.height < 660.0 {
                    UX.topInset = 10.0
                    UX.logoSizeAfterAnimation = 50.0
                }
                
                print(UX.topInset)
                self.startAnimations()
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        // MARK: - Animations
        
        private func startAnimations() {
            UIView.animate(withDuration: 0.7, animations: braveLogoAnimation) { _ in
                self.braveLogoBounceEffect()
                
                UIView.animate(withDuration: 0.5, animations: self.showTitle) { _ in
                    UIView.animate(withDuration: 0.5, animations: self.showRemainingViews) { _ in
                        self.searchEnginesTable.flashScrollIndicators()
                    }
                }
            }
        }
        
        private func braveLogoAnimation() {
            // This is how many points until we reach top of the view.
            let logoY = -self.braveLogo.frame.minY
            // The logo changes size when animating, we need to account for that.
            let sizeDelta = min(0, (UX.logoSizeBeforeAnimation - UX.logoSizeAfterAnimation)) / 2
            
            self.logoCenterY?.update(offset: logoY + UX.topInset - sizeDelta)
            
            self.braveLogo.snp.updateConstraints {
                $0.height.equalTo(UX.logoSizeAfterAnimation)
            }
            self.layoutIfNeeded()
        }
        
        private func braveLogoBounceEffect() {
            POPBasicAnimation(propertyNamed: kPOPLayerTranslationY)?.do {
                $0.fromValue = -5
                $0.toValue = 5
                $0.repeatForever = true
                $0.autoreverses = true
                $0.duration = 2
                self.braveLogo.layer.pop_add($0, forKey: "braveLogoTranslateY")
            }
        }
        
        private func showTitle() {
            self.titlePrimary.alpha = 1
        }
        
        private func showRemainingViews() {
            [self.titleSecondary, self.searchEnginesTable, self.buttonsStackView].forEach {
                $0.alpha = 1
            }
        }
    }
    
    // MARK: - SearchEngineCell
    
    class SearchEngineCell: UITableViewCell {
        
        static let preferredHeight = UX.SearchEngineCell.rowHeight
        
        var searchEngineName: String? {
            get { return textLabel?.text }
            set { textLabel?.text = newValue }
        }
        
        var searchEngineImage: UIImage? {
            get { return imageView?.image }
            set { imageView?.image = newValue }
        }
        
        var selectedBackgroundColor: UIColor? {
            didSet {
                selectedBackgroundView?.backgroundColor = selectedBackgroundColor
            }
        }
        
        override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
            super.init(style: style, reuseIdentifier: reuseIdentifier)
            
            imageView?.contentMode = .scaleAspectFit
            layer.cornerRadius = UX.SearchEngineCell.cornerRadius
            layer.cornerCurve = .continuous
            
            selectedBackgroundView = UIView().then {
                $0.layer.cornerRadius = UX.SearchEngineCell.cornerRadius
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            let size = UX.SearchEngineCell.imageSize
            imageView?.bounds = CGRect(x: 0, y: 0, width: size, height: size)
        }
    }
}
