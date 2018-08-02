// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Storage
import SnapKit
import Shared
import BraveShared
import Data

struct ShieldBlockedStats {
    var adsAndTrackers = 0
    var httpsUpgrades = 0
    var blockedScripts = 0
    var fingerprintingProtection = 0
}

/// Displays shield settings and shield stats for a given URL
class ShieldsViewController: UIViewController, PopoverContentComponent {
    /// The url loaded currently
    var url: URL? {
        didSet {
            updateToggleStatus()
            updateOverviewState()
        }
    }
    /// The blocked stats
    var shieldBlockStats: ShieldBlockedStats? {
        didSet {
            updateShieldBlockStats()
        }
    }
    
    init(url: URL?, shieldBlockStats: ShieldBlockedStats?) {
        self.url = url
        self.shieldBlockStats = shieldBlockStats
        
        super.init(nibName: nil, bundle: nil)
        
        updateToggleStatus()
        updateShieldBlockStats()
        updateOverviewState()
    }
    
    // MARK: - State
    
    private func updateOverviewState() {
        if let url = url, let domain = url.normalizedHost, !url.isLocal {
            shieldsView.hostLabel.text = domain
            // Swap out the overview for the host label
            shieldsView.overviewLabel.removeFromSuperview()
            shieldsView.overviewFooterLabel.removeFromSuperview()
            shieldsView.stackView.insertArrangedSubview(shieldsView.hostLabel, at: 1)
        } else {
            // Swap out the host label for the overview
            shieldsView.hostLabel.removeFromSuperview()
            shieldsView.stackView.insertArrangedSubview(shieldsView.overviewLabel, at: 1)
            shieldsView.stackView.insertArrangedSubview(shieldsView.overviewFooterLabel, at: 2)
            shieldsView.stackView.setCustomSpacing(30.0, after: shieldsView.overviewFooterLabel)
        }
    }
    
    private func updateToggleStatus() {
        var isEnabled = false
        if let url = url, !url.isLocal {
            isEnabled = true
        }
        shieldControlMapping.forEach { shield, view, option in
            view.toggleSwitch.isEnabled = isEnabled
            if let option = option {
                // Sets the default setting
                view.toggleSwitch.isOn = option.value
            } else {
                // The only "shield" that doesn't actually have a global preference is the option to disable all shields
                // Therefore its the only shield we have to give a default value of true
                view.toggleSwitch.isOn = shield == .AllOff
            }
            if isEnabled, let host = url?.normalizedHost, let shieldState = BraveShieldState.getStateForDomain(host) {
                // Sets the site-specific setting
                view.toggleSwitch.isOn = shieldState.isShieldEnabled(shield)
            }
        }
    }
    
    private func updateShieldBlockStats() {
        guard let stats = shieldBlockStats else {
            // All 0
            shieldsView.adsTrackersStatView.valueLabel.text = "0"
            shieldsView.httpsUpgradesStatView.valueLabel.text = "0"
            shieldsView.scriptsBlockedStatView.valueLabel.text = "0"
            shieldsView.fingerprintingStatView.valueLabel.text = "0"
            return
        }
        shieldsView.adsTrackersStatView.valueLabel.text = String(stats.adsAndTrackers)
        shieldsView.httpsUpgradesStatView.valueLabel.text = String(stats.httpsUpgrades)
        shieldsView.scriptsBlockedStatView.valueLabel.text = String(stats.blockedScripts)
        shieldsView.fingerprintingStatView.valueLabel.text = String(stats.fingerprintingProtection)
    }
    
    private func updateBraveShieldState(shield: BraveShieldState.Shield, on: Bool) {
        guard let url = self.url, let domain = url.normalizedHost, !url.isLocal else { return }
        if shield == .AllOff {
            // Technically we set "all off" when the switch is OFF, unlike all the others
            BraveShieldState.set(forDomain: domain, state: (.AllOff, !on))
        } else {
            BraveShieldState.set(forDomain: domain, state: (shield, on))
        }
    }
    
    // MARK: -
    
    /// Groups the shield types with their control and global preference
    private lazy var shieldControlMapping: [(BraveShieldState.Shield, ToggleView, Preferences.Option<Bool>?)] = [
        (.AllOff, shieldsView.shieldOverrideControl, nil),
        (.AdblockAndTp, shieldsView.adsTrackersControl, Preferences.Shields.blockAdsAndTracking),
        (.SafeBrowsing, shieldsView.blockMalwareControl, Preferences.Shields.blockPhishingAndMalware),
        (.NoScript, shieldsView.blockScriptsControl, Preferences.Shields.blockScripts),
        (.HTTPSE, shieldsView.httpsUpgradesControl, Preferences.Shields.httpsEverywhere),
        (.FpProtection, shieldsView.fingerprintingControl, Preferences.Shields.fingerprintingProtection),
    ]
    
    var shieldsView: View {
        return view as! View
    }
    
    override func loadView() {
        view = View()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        shieldControlMapping.forEach { shield, toggle, _ in
            toggle.valueToggled = { [unowned self] on in
                self.updateBraveShieldState(shield: shield, on: on)
            }
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}

extension ShieldsViewController {
    /// The custom loaded view for the `ShieldsViewController`
    class View: UIView {
        /// Create a header label
        private class func headerLabel(title: String) -> UILabel {
            let label = UILabel()
            label.textColor = UIColor(white: 0.4, alpha: 1.0)
            label.font = .systemFont(ofSize: 15.0)
            label.text = title
            return label
        }
        
        private func dividerView() -> UIView {
            let divider = UIView()
            divider.backgroundColor = BraveUX.ColorForSidebarLineSeparators
            divider.snp.makeConstraints { $0.height.equalTo(1.0 / UIScreen.main.scale) }
            return divider
        }
        
        private let scrollView = UIScrollView()
        
        let stackView: UIStackView = {
            let sv = UIStackView()
            sv.axis = .vertical
            sv.spacing = 15.0
            return sv
        }()
        
        // Global Shields Override
        let shieldOverrideControl: ToggleView = {
            let toggleView = ToggleView(title: Strings.Site_shield_settings, toggleSide: .right)
            toggleView.titleLabel.textColor = BraveUX.GreyJ
            toggleView.titleLabel.font = .systemFont(ofSize: 17.0, weight: .medium)
            return toggleView
        }()
        
        // Site Host Label
        let hostLabel: UILabel = {
            let label = UILabel()
            label.font = .systemFont(ofSize: 21.0, weight: .medium)
            label.lineBreakMode = .byTruncatingMiddle
            label.minimumScaleFactor = 0.75
            label.adjustsFontSizeToFitWidth = true
            return label
        }()
        
        // Shields Overview Labels
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
            label.textColor = .lightGray
            label.text = Strings.Shields_Overview_Footer
            return label
        }()
        
        // Stats
        let statsHeaderLabel = headerLabel(title: Strings.Blocking_Monitor)
        let adsTrackersStatView = StatView(title: Strings.Ads_and_Trackers, valueColor: BraveUX.BraveOrange)
        let httpsUpgradesStatView = StatView(title: Strings.HTTPS_Upgrades, valueColor: BraveUX.Green)
        let scriptsBlockedStatView = StatView(title: Strings.Scripts_Blocked, valueColor: BraveUX.Purple)
        let fingerprintingStatView = StatView(title: Strings.Fingerprinting_Methods, valueColor: BraveUX.GreyG)
        
        // Settings
        let settingsHeaderLabel = headerLabel(title: Strings.Individual_Controls)
        let adsTrackersControl = ToggleView(title: Strings.Block_Ads_and_Tracking)
        let httpsUpgradesControl = ToggleView(title: Strings.HTTPS_Everywhere)
        let blockMalwareControl = ToggleView(title: Strings.Block_Phishing)
        let blockScriptsControl = ToggleView(title: Strings.Block_Scripts)
        let fingerprintingControl = ToggleView(title: Strings.Fingerprinting_Protection_wrapped)
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            addSubview(scrollView)
            scrollView.addSubview(stackView)
            
            scrollView.snp.makeConstraints {
                $0.edges.equalTo(self)
            }
            
            stackView.snp.makeConstraints {
                $0.left.right.equalTo(scrollView.frameLayoutGuide).inset(20.0)
                $0.top.bottom.equalTo(scrollView.contentLayoutGuide).inset(20.0)
            }
            
            stackView.addArrangedSubview(shieldOverrideControl)
            
            // Stats
            stackView.addArrangedSubview(statsHeaderLabel)
            stackView.setCustomSpacing(15.0, after: statsHeaderLabel)
            let statViews = [adsTrackersStatView, httpsUpgradesStatView, scriptsBlockedStatView, fingerprintingStatView]
            statViews.forEach {
                stackView.addArrangedSubview($0)
                if $0 !== statViews.last {
                    stackView.setCustomSpacing(3.0, after: $0)
                }
            }
            
            // Controls
            stackView.addArrangedSubview(dividerView())
            stackView.addArrangedSubview(settingsHeaderLabel)
            
            [adsTrackersControl, httpsUpgradesControl, blockMalwareControl, blockScriptsControl, fingerprintingControl].forEach {
                stackView.addArrangedSubview($0)
                stackView.setCustomSpacing(18.0, after: $0)
            }
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError()
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
            return l
        }()
        /// Create the stat view with a given title and color
        init(title: String, valueColor: UIColor) {
            super.init(frame: .zero)
            
            valueLabel.textColor = valueColor
            titleLabel.text = title
            
            addSubview(valueLabel)
            addSubview(titleLabel)
            
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
            titleLabel.setContentCompressionResistancePriority(.required, for: .horizontal)
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
