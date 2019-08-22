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
        static let topInset: CGFloat = 48
        static let contentInset: CGFloat = 25
        static let logoSizeAfterAnimation: CGFloat = 100
        static let logoSizeBeforeAnimation: CGFloat = 150
        
        struct SearchEngineCell {
            static let rowHeight: CGFloat = 54
            static let imageSize: CGFloat = 32
            static let cornerRadius: CGFloat = 8
            static let selectedBackgroundColor = #colorLiteral(red: 0.9411764706, green: 0.9450980392, blue: 1, alpha: 1)
            static let deselectedBackgroundColor: UIColor = .white
        }
    }
    
    class View: UIView {
        
        let searchEnginesTable = UITableView().then {
            $0.separatorStyle = .none
            $0.allowsMultipleSelection = false
            $0.alwaysBounceVertical = false
        }
        
        let continueButton = CommonViews.primaryButton(text: Strings.OBSaveButton).then {
            $0.accessibilityIdentifier = "OnboardingSearchEnginesViewController.SaveButton"
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
        }
        
        let skipButton = CommonViews.secondaryButton().then {
            $0.accessibilityIdentifier = "OnboardingSearchEnginesViewController.SkipButton"
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
        }
        
        let titleSecondary = CommonViews.secondaryText(Strings.OBSearchEngineDetail).then {
            $0.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.medium)
        }
        
        private let titleStackView = UIStackView().then { stackView in
            stackView.axis = .vertical
        }
        
        private let buttonsStackView = UIStackView().then {
            $0.distribution = .equalCentering
            $0.alignment = .center
        }
        
        private var logoCenterY: Constraint?
        
        init() {
            super.init(frame: .zero)
            backgroundColor = .white
            
            addSubview(braveLogo)
            
            [titlePrimary, titleSecondary].forEach(titleStackView.addArrangedSubview(_:))
            
            let spacer = UIView()
            
            [skipButton, continueButton, spacer]
                .forEach(buttonsStackView.addArrangedSubview(_:))
            
            [titleStackView, searchEnginesTable, buttonsStackView]
                .forEach(mainStackView.addArrangedSubview(_:))
            
            addSubview(mainStackView)
            
            braveLogo.snp.makeConstraints {
                $0.centerX.equalToSuperview()
                logoCenterY = $0.centerY.equalToSuperview().constraint
                $0.size.equalTo(UX.logoSizeBeforeAnimation)
            }

            mainStackView.snp.makeConstraints {
                $0.top.equalTo(braveLogo.snp.bottom).offset(30)
                $0.leading.equalTo(self.safeArea.leading).inset(UX.contentInset)
                $0.trailing.equalTo(self.safeArea.trailing).inset(UX.contentInset)
                $0.bottom.equalTo(self.safeArea.bottom).inset(UX.contentInset)
            }
            
            // Make width the same as skip button to make save button always centered.
            spacer.snp.makeConstraints {
                $0.width.equalTo(skipButton)
            }
            
            // Hiding views in prepration to animations.
            // Alpha is used instead of `isHidden` to make the views participate in auto-layout.
            [titlePrimary, titleSecondary, searchEnginesTable, buttonsStackView].forEach {
                $0.alpha = CGFloat.leastNormalMagnitude
            }
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
                self?.startAnimations()
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        // MARK: - Animations
        
        private func startAnimations() {
            UIView.animate(withDuration: 0.7, animations: braveLogoAnimation) { _ in
                self.braveLogoBounceEffect()
                
                UIView.animate(withDuration: 0.5, animations: self.showTitle) { _ in
                    UIView.animate(withDuration: 0.5, animations: self.showRemainingViews)
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
            set { textLabel?.text = newValue }
            get { return textLabel?.text }
        }
        
        var searchEngineImage: UIImage? {
            set { imageView?.image = newValue }
            get { return imageView?.image }
        }
        
        override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
            super.init(style: style, reuseIdentifier: reuseIdentifier)
            
            imageView?.contentMode = .scaleAspectFit
            layer.cornerRadius = UX.SearchEngineCell.cornerRadius
            selectionStyle = .none
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        override func setSelected(_ selected: Bool, animated: Bool) {
            super.setSelected(selected, animated: animated)
            
            backgroundColor = selected ?
                UX.SearchEngineCell.selectedBackgroundColor : UX.SearchEngineCell.deselectedBackgroundColor
            
            textLabel?.font = UIFont.systemFont(ofSize: 16, weight: UIFont.Weight.medium)
        }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            let size = UX.SearchEngineCell.imageSize
            imageView?.bounds = CGRect(x: 0, y: 0, width: size, height: size)
        }
    }
}
