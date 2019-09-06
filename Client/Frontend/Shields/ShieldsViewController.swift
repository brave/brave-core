// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Storage
import SnapKit
import Shared
import BraveShared
import Data

/// Displays shield settings and shield stats for a given URL
class ShieldsViewController: UIViewController, PopoverContentComponent {
    let tab: Tab
    private lazy var url: URL? = {
        guard let _url = tab.url else { return nil }
        
        if _url.isErrorPageURL {
            return _url.originalURLFromErrorURL
        }
        
        return _url
    }()
    
    var shieldsSettingsChanged: ((ShieldsViewController) -> Void)?
    
    private var statsUpdateObservable: AnyObject?
    
    /// Create with an initial URL and block stats (or nil if you are not on any web page)
    init(tab: Tab) {
        self.tab = tab
        
        super.init(nibName: nil, bundle: nil)
        
        shieldsView.shieldsContainerStackView.hostLabel.text = url?.normalizedHost
        
        updateToggleStatus()
        updateShieldBlockStats()
        
        tab.contentBlocker.statsDidChange = { [weak self] _ in
            self?.updateShieldBlockStats()
        }
    }
    
    // MARK: - State
    
    private func updateToggleStatus() {
        var domain: Domain?
        if let url = url {
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
        }
        
        shieldControlMapping.forEach { shield, view, option in
            
            // Updating based on global settings
            
            if let option = option {
                // Sets the default setting
                view.toggleSwitch.isOn = option.value
            } else {
                // The only "shield" that doesn't actually have a global preference is the option to disable all shields
                // Therefore its the only shield we have to give a default value of true
                view.toggleSwitch.isOn = shield == .AllOff
            }
            
            // Domain specific overrides after defaults have already been setup
            
            if let domain = domain {
                // site-specific shield has been overridden, update
                view.toggleSwitch.isOn = domain.isShieldExpected(shield)
                if shield == .AllOff {
                    // Reverse, as logic is inverted
                    view.toggleSwitch.isOn.toggle()
                }
            }
        }
        updateGlobalShieldState(shieldsView.shieldOverrideControl.toggleSwitch.isOn)
    }
    
    private func updateShieldBlockStats() {
        shieldsView.shieldsContainerStackView.adsTrackersStatView.valueLabel.text = String(tab.contentBlocker.stats.adCount + tab.contentBlocker.stats.trackerCount)
        shieldsView.shieldsContainerStackView.httpsUpgradesStatView.valueLabel.text = String(tab.contentBlocker.stats.httpsCount)
        shieldsView.shieldsContainerStackView.scriptsBlockedStatView.valueLabel.text = String(tab.contentBlocker.stats.scriptCount)
        shieldsView.shieldsContainerStackView.fingerprintingStatView.valueLabel.text = String(tab.contentBlocker.stats.fingerprintingCount)
    }
    
    private func updateBraveShieldState(shield: BraveShield, on: Bool, option: Preferences.Option<Bool>?) {
        guard let url = url else { return }
        let allOff = shield == .AllOff
        // `.AllOff` uses inverse logic. Technically we set "all off" when the switch is OFF, unlike all the others
        // If the new state is the same as the global preference, reset it to nil so future shield state queries
        // respect the global preference rather than the overridden value. (Prevents toggling domain state from
        // affecting future changes to the global pref)
        let isOn = allOff ? !on : on
        Domain.setBraveShield(forUrl: url, shield: shield, isOn: isOn,
                              isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }
    
    private func updateGlobalShieldState(_ on: Bool, animated: Bool = false) {
        // Whether or not shields are available for this URL.
        let isShieldsAvailable = url?.isLocal == false
        // If shields aren't available, we don't show the switch and show the "off" state
        let shieldsEnabled = isShieldsAvailable ? on : false
        let updateBlock = {
            self.shieldsView.shieldOverrideControl.isHidden = !isShieldsAvailable
            self.shieldsView.shieldsContainerStackView.isHidden = !shieldsEnabled
            self.shieldsView.shieldsContainerStackView.alpha = shieldsEnabled ? 1.0 : 0.0
            self.shieldsView.overviewStackView.isHidden = shieldsEnabled
            self.shieldsView.overviewStackView.alpha = shieldsEnabled ? 0.0 : 1.0
        }
        if animated {
            UIView.animate(withDuration: 0.25) {
                updateBlock()
            }
        } else {
            updateBlock()
        }
        shieldsView.stackView.setNeedsLayout()
        shieldsView.stackView.layoutIfNeeded()
        
        let size = CGSize(width: PopoverController.preferredPopoverWidth, height: UIScreen.main.bounds.height)
        preferredContentSize = shieldsView.stackView.systemLayoutSizeFitting(
            size,
            withHorizontalFittingPriority: .required,
            verticalFittingPriority: .fittingSizeLevel
        )
    }
    
    // MARK: -
    
    /// Groups the shield types with their control and global preference
    private lazy var shieldControlMapping: [(BraveShield, ToggleView, Preferences.Option<Bool>?)] = [
        (.AllOff, shieldsView.shieldOverrideControl, nil),
        (.AdblockAndTp, shieldsView.shieldsContainerStackView.adsTrackersControl, Preferences.Shields.blockAdsAndTracking),
        (.SafeBrowsing, shieldsView.shieldsContainerStackView.blockMalwareControl, Preferences.Shields.blockPhishingAndMalware),
        (.NoScript, shieldsView.shieldsContainerStackView.blockScriptsControl, Preferences.Shields.blockScripts),
        (.HTTPSE, shieldsView.shieldsContainerStackView.httpsUpgradesControl, Preferences.Shields.httpsEverywhere),
        (.FpProtection, shieldsView.shieldsContainerStackView.fingerprintingControl, Preferences.Shields.fingerprintingProtection),
    ]
    
    var shieldsView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View()
        shieldsView.applyTheme(Theme.of(tab))
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        shieldControlMapping.forEach { shield, toggle, option in
            toggle.valueToggled = { [unowned self] on in
                if shield == .AllOff {
                    // Update the content size
                    self.updateGlobalShieldState(on, animated: true)
                }
                // Localized / per domain toggles triggered here
                self.updateBraveShieldState(shield: shield, on: on, option: option)
                self.shieldsSettingsChanged?(self)
            }
        }
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}
