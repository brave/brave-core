// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

extension ShieldsViewController {
    /// The custom loaded view for the `ShieldsViewController`
    class View: UIView, Themeable {
        private let scrollView = UIScrollView()
        
        let stackView: UIStackView = {
            let sv = UIStackView()
            sv.axis = .vertical
            sv.spacing = 15.0
            sv.layoutMargins = UIEdgeInsets(top: 20, left: 20, bottom: 20, right: 20)
            sv.isLayoutMarginsRelativeArrangement = true
            return sv
        }()
        
        // Global Shields Override
        let shieldOverrideControl: ToggleView = {
            let toggleView = ToggleView(title: Strings.Site_shield_settings, toggleSide: .right)
            toggleView.titleLabel.font = .systemFont(ofSize: 17.0, weight: .medium)
            return toggleView
        }()
        
        let overviewStackView: OverviewContainerStackView = {
            let sv = OverviewContainerStackView()
            sv.isHidden = true
            return sv
        }()
        
        let shieldsContainerStackView = ShieldsContainerStackView()
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            addSubview(scrollView)
            scrollView.addSubview(stackView)
            
            scrollView.snp.makeConstraints {
                $0.edges.equalTo(self)
            }
            
            scrollView.contentLayoutGuide.snp.makeConstraints {
                $0.width.equalTo(self)
            }
            
            stackView.snp.makeConstraints {
                $0.edges.equalTo(scrollView.contentLayoutGuide)
            }
            
            stackView.addArrangedSubview(shieldOverrideControl)
            stackView.addArrangedSubview(overviewStackView)
            stackView.addArrangedSubview(shieldsContainerStackView)
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
        }
        
        // MARK: - Themeable
        func applyTheme(_ theme: Theme) {
            styleChildren(theme: theme)
            
            backgroundColor = theme.colors.home
            
            // Overview
            overviewStackView.overviewFooterLabel.textColor = overviewStackView.overviewLabel.textColor.withAlphaComponent(0.6)
            
            // Normal shields panel
            shieldsContainerStackView.set(theme: theme)
        }
    }
    
    class OverviewContainerStackView: UIStackView {
        
        let overviewLabel: UILabel = {
            let label = UILabel()
            label.numberOfLines = 0
            label.font = .systemFont(ofSize: 15.0)
            label.text = Strings.Shields_Overview
            return label
        }()
        
        let overviewFooterLabel: UILabel = {
            let label = UILabel()
            label.numberOfLines = 0
            label.font = .systemFont(ofSize: 15.0)
            label.text = Strings.Shields_Overview_Footer
            return label
        }()
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            axis = .vertical
            spacing = 15.0
            
            addArrangedSubview(overviewLabel)
            addArrangedSubview(overviewFooterLabel)
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
    
    class ShieldsContainerStackView: UIStackView {
        /// Create a header label
        private class func headerLabel(title: String) -> UILabel {
            let label = UILabel()
            label.font = .systemFont(ofSize: 15.0)
            label.text = title
            return label
        }
        
        private class func dividerView() -> UIView {
            let divider = UIView()
            divider.backgroundColor = BraveUX.ColorForSidebarLineSeparators
            divider.snp.makeConstraints { $0.height.equalTo(1.0 / UIScreen.main.scale) }
            return divider
        }
        
        // Site Host Label
        let hostLabel: UILabel = {
            let label = UILabel()
            label.font = .systemFont(ofSize: 21.0, weight: .medium)
            label.lineBreakMode = .byTruncatingMiddle
            label.minimumScaleFactor = 0.75
            label.adjustsFontSizeToFitWidth = true
            return label
        }()
        
        // Stats
        let statsHeaderLabel = headerLabel(title: Strings.Blocking_Monitor)
        let adsTrackersStatView = StatView(title: Strings.Ads_and_Trackers)
        let httpsUpgradesStatView = StatView(title: Strings.HTTPS_Upgrades)
        let scriptsBlockedStatView = StatView(title: Strings.Scripts_Blocked)
        let fingerprintingStatView = StatView(title: Strings.Fingerprinting_Methods)
        
        // Settings
        let settingsDivider = dividerView()
        let settingsHeaderLabel = headerLabel(title: Strings.Individual_Controls)
        let adsTrackersControl = ToggleView(title: Strings.Block_Ads_and_Tracking)
        let httpsUpgradesControl = ToggleView(title: Strings.HTTPS_Everywhere)
        let blockMalwareControl = ToggleView(title: Strings.Block_Phishing)
        let blockScriptsControl = ToggleView(title: Strings.Block_Scripts)
        let fingerprintingControl = ToggleView(title: Strings.Fingerprinting_Protection_wrapped)
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            axis = .vertical
            spacing = 15.0
            
            // Stats
            addArrangedSubview(hostLabel)
            addArrangedSubview(statsHeaderLabel)
            setCustomSpacing(15.0, after: statsHeaderLabel)
            let statViews = [adsTrackersStatView, httpsUpgradesStatView, scriptsBlockedStatView, fingerprintingStatView]
            statViews.forEach {
                addArrangedSubview($0)
                if $0 !== statViews.last {
                    setCustomSpacing(3.0, after: $0)
                }
            }
            
            // Controls
            addArrangedSubview(settingsDivider)
            addArrangedSubview(settingsHeaderLabel)
            
            [adsTrackersControl, httpsUpgradesControl, blockMalwareControl, blockScriptsControl, fingerprintingControl].forEach {
                addArrangedSubview($0)
                setCustomSpacing(18.0, after: $0)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
        
        func set(theme: Theme) {
            let stats = theme.colors.stats
            [
                adsTrackersStatView: stats.ads,
                httpsUpgradesStatView: stats.httpse,
                scriptsBlockedStatView: stats.trackers
            ].forEach {
                $0.0.valueLabel.appearanceTextColor = $0.1
            }
            
            let faddedColor = theme.colors.tints.home.withAlphaComponent(0.8)
            statsHeaderLabel.appearanceTextColor = faddedColor
            settingsHeaderLabel.appearanceTextColor = faddedColor
        }
    }
    
    /// Displays some UI that displays the block count of a stat. Set `valueLabel.text` to the stat
    class StatView: UIView {
        /// The number the shield has blocked
        let valueLabel: UILabel = {
            let l = UILabel()
            l.font = .boldSystemFont(ofSize: 28.0)
            l.adjustsFontSizeToFitWidth = true
            l.textAlignment = .center
            l.text = "0"
            return l
        }()
        /// The stat being blocked (i.e. Ads and Trackers)
        let titleLabel: UILabel = {
            let l = UILabel()
            l.font = .systemFont(ofSize: 15.0)
            l.adjustsFontSizeToFitWidth = true
            l.numberOfLines = 0
            return l
        }()
        /// Create the stat view with a given title
        init(title: String) {
            super.init(frame: .zero)
            titleLabel.text = title
            
            addSubview(valueLabel)
            addSubview(titleLabel)
            
            titleLabel.setContentCompressionResistancePriority(.required, for: .horizontal)
            
            valueLabel.snp.makeConstraints {
                $0.width.equalTo(50.0)
                $0.top.bottom.equalTo(self)
                $0.left.equalTo(self)
            }
            titleLabel.snp.makeConstraints {
                $0.left.equalTo(valueLabel.snp.right).offset(12)
                $0.top.bottom.equalTo(self)
                $0.right.equalTo(self)
            }
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
        }
    }
    
    /// A container displaying a toggle for the user
    class ToggleView: UIView {
        /// Where the toggle resides
        enum ToggleSide {
            /// Resides on the left edge of the view
            case left
            /// Resides on the right edge of the view
            case right
        }
        
        let titleLabel: UILabel = {
            let l = UILabel()
            l.font = .systemFont(ofSize: 15.0)
            l.numberOfLines = 0
            return l
        }()
        
        let toggleSwitch = UISwitch()
        var valueToggled: ((Bool) -> Void)?
        
        init(title: String, toggleSide: ToggleSide = .left) {
            super.init(frame: .zero)
            
            let stackView = UIStackView()
            stackView.spacing = 12.0
            stackView.alignment = .center
            addSubview(stackView)
            stackView.snp.makeConstraints {
                $0.edges.equalTo(self)
            }
            
            if toggleSide == .left {
                stackView.addArrangedSubview(toggleSwitch)
                stackView.addArrangedSubview(titleLabel)
            } else {
                stackView.addArrangedSubview(titleLabel)
                stackView.addArrangedSubview(toggleSwitch)
            }
            
            titleLabel.text = title
            toggleSwitch.addTarget(self, action: #selector(switchValueChanged), for: .valueChanged)
            
            toggleSwitch.setContentHuggingPriority(.required, for: .horizontal)
            toggleSwitch.setContentCompressionResistancePriority(.required, for: .horizontal)
            titleLabel.setContentCompressionResistancePriority(.required, for: .vertical)
            
            snp.makeConstraints {
                $0.height.greaterThanOrEqualTo(toggleSwitch)
            }
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
        }
        
        @objc private func switchValueChanged() {
            valueToggled?(toggleSwitch.isOn)
        }
    }
}
