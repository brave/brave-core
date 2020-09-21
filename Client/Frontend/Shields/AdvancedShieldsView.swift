// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class AdvancedShieldsView: UIStackView, Themeable {
    
    var themeableChildren: [Themeable?]?
    
    let siteTitle = HeaderTitleView()
    let adsTrackersControl = ToggleView(title: Strings.blockAdsAndTracking)
    let httpsUpgradesControl = ToggleView(title: Strings.HTTPSEverywhere)
    let blockMalwareControl = ToggleView(title: Strings.blockPhishing)
    let blockScriptsControl = ToggleView(title: Strings.blockScripts)
    let fingerprintingControl = ToggleView(title: Strings.fingerprintingProtectionWrapped)
    let globalControlsTitleView = HeaderTitleView().then {
        $0.titleLabel.text = Strings.Shields.globalControls.uppercased()
    }
    let globalControlsButton = ChangeGlobalDefaultsView()
    
    private func dividerView() -> UIView {
        let divider = UIView()
        divider.backgroundColor = BraveUX.colorForSidebarLineSeparators
        divider.snp.makeConstraints { $0.height.equalTo(1.0 / UIScreen.main.scale) }
        return divider
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        axis = .vertical
        
        let rows: [UIView & Themeable] = [
            siteTitle,
            adsTrackersControl,
            httpsUpgradesControl,
            blockMalwareControl,
            blockScriptsControl,
            fingerprintingControl,
            globalControlsTitleView,
            globalControlsButton
        ]
        let separators = (0..<rows.count).map { _ in SeparatorView() }
        let n = zip(rows, separators)
        for (sep, row) in n {
            addArrangedSubview(sep)
            addArrangedSubview(row)
        }
        
        themeableChildren = separators + rows
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

extension AdvancedShieldsView {
    
    class HeaderTitleView: UIView, Themeable {
        
        let titleLabel = UILabel().then {
            $0.font = .systemFont(ofSize: 13.0)
            $0.numberOfLines = 0
            $0.appearanceTextColor = Colors.grey700
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            addSubview(titleLabel)
            titleLabel.snp.makeConstraints {
                $0.top.leading.trailing.equalTo(self).inset(16)
                $0.bottom.equalTo(self).inset(6)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
    
    /// A container displaying a toggle for the user
    class ToggleView: UIView, Themeable {
        
        let titleLabel: UILabel = {
            let l = UILabel()
            l.font = .systemFont(ofSize: 15.0)
            l.numberOfLines = 0
            return l
        }()
        
        let toggleSwitch = UISwitch()
        var valueToggled: ((Bool) -> Void)?
        
        init(title: String) {
            super.init(frame: .zero)
            
            let stackView = UIStackView()
            stackView.spacing = 12.0
            stackView.alignment = .center
            addSubview(stackView)
            self.snp.makeConstraints {
                $0.height.greaterThanOrEqualTo(44)
            }
            stackView.snp.makeConstraints {
                $0.leading.trailing.equalTo(self).inset(16)
                $0.top.bottom.equalTo(self)
            }
            
            stackView.addArrangedSubview(titleLabel)
            stackView.addArrangedSubview(toggleSwitch)
            
            titleLabel.text = title
            toggleSwitch.addTarget(self, action: #selector(switchValueChanged), for: .valueChanged)
            
            toggleSwitch.setContentHuggingPriority(.required, for: .horizontal)
            
            snp.makeConstraints {
                $0.height.greaterThanOrEqualTo(toggleSwitch)
            }
            
            isAccessibilityElement = true
            accessibilityTraits.insert(.button)
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
        }
        
        override func accessibilityActivate() -> Bool {
            toggleSwitch.setOn(!toggleSwitch.isOn, animated: true)
            toggleSwitch.sendActions(for: .valueChanged)
            return true
        }
        
        override var accessibilityLabel: String? {
            get { titleLabel.accessibilityLabel }
            set { assertionFailure() } // swiftlint:disable:this unused_setter_value
        }
        
        override var accessibilityValue: String? {
            get { toggleSwitch.accessibilityValue }
            set { assertionFailure() } // swiftlint:disable:this unused_setter_value
        }
        
        @objc private func switchValueChanged() {
            valueToggled?(toggleSwitch.isOn)
        }
        
        func applyTheme(_ theme: Theme) {
            titleLabel.textColor = theme.colors.tints.home
        }
    }
    
    class SeparatorView: UIView, Themeable {
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            self.snp.makeConstraints {
                $0.height.equalTo(1.0 / UIScreen.main.scale)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
        
        func applyTheme(_ theme: Theme) {
            backgroundColor = theme.isDark ?
                UIColor(white: 1.0, alpha: 0.2) :
                UIColor(white: 0.0, alpha: 0.2)
        }
    }
}

final class ChangeGlobalDefaultsView: UIControl, Themeable {
    
    private let highlightedBackgroundView = UIView().then {
        $0.isUserInteractionEnabled = false
        $0.alpha = 0.0
    }
    private let imageView = UIImageView(image: UIImage(imageLiteralResourceName: "internet-block").template).then {
        $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    private let textLabel = UILabel().then {
        $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
        $0.font = .systemFont(ofSize: 15.0)
        $0.text = Strings.Shields.globalChangeButton
    }
    private let chevron = UIImageView(image: UIImage(imageLiteralResourceName: "chevron").template).then {
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.tintColor = UIColor(rgb: 0xD1D1D6)
    }
    
    override var isHighlighted: Bool {
        didSet {
            UIView.animate(withDuration: 0.15, delay: 0, options: [.beginFromCurrentState], animations: {
                self.highlightedBackgroundView.alpha = self.isHighlighted ? 1.0 : 0.0
            }, completion: nil)
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        isAccessibilityElement = true
        accessibilityTraits.insert(.button)
        accessibilityLabel = textLabel.text
        
        let stackView = UIStackView().then {
            $0.spacing = 10
            $0.alignment = .center
            $0.isUserInteractionEnabled = false
        }
        
        addSubview(highlightedBackgroundView)
        addSubview(stackView)
        stackView.addStackViewItems(
            .view(imageView),
            .view(textLabel),
            .view(chevron)
        )
        
        highlightedBackgroundView.snp.makeConstraints {
            $0.edges.equalTo(self)
        }
        stackView.snp.makeConstraints {
            $0.leading.trailing.equalTo(self).inset(16)
            $0.top.greaterThanOrEqualTo(self)
            $0.bottom.lessThanOrEqualTo(self)
            $0.centerY.equalTo(self)
            $0.height.greaterThanOrEqualTo(44)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func applyTheme(_ theme: Theme) {
        highlightedBackgroundView.backgroundColor = theme.isDark ?
            UIColor(white: 1.0, alpha: 0.1) :
            UIColor(white: 0.0, alpha: 0.1)
        imageView.tintColor = theme.isDark ? Colors.grey500 : Colors.grey700
        textLabel.textColor = theme.colors.tints.home
    }
}
