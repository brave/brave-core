// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Storage
import SnapKit
import Shared
import BraveShared
import Data

class ShieldBlockedStats: NSObject {
    var adsAndTrackers = 0
    var httpsUpgrades = 0
    var blockedScripts = 0
    var fingerprintingProtection = 0
}

/// Displays shield settings and shield stats for a given URL
class ShieldsViewController: UIViewController, PopoverContentComponent {
    /// The url loaded currently. Update this as the pages url changes
    var url: URL? {
        didSet {
            updateToggleStatus()
        }
    }
    /// The blocked stats. Update this as the pages block stats change
    var shieldBlockStats: ShieldBlockedStats {
        didSet {
            updateShieldBlockStats()
        }
    }
    
    var shieldsSettingsChanged: ((ShieldsViewController) -> Void)?
    
    /// Create with an initial URL and block stats (or nil if you are not on any web page)
    init(url: URL?, shieldBlockStats: ShieldBlockedStats) {
        self.url = url
        self.shieldBlockStats = shieldBlockStats
        
        super.init(nibName: nil, bundle: nil)
        
        shieldsView.shieldsContainerStackView.hostLabel.text = url?.normalizedHost
        
        updateToggleStatus()
        updateShieldBlockStats()
    }
    
    // MARK: - State
    
    private func updateToggleStatus() {
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
            
            if let host = url?.domainURL.absoluteString, let shieldState = BraveShieldState.getStateForDomain(host) {
                // Sets the site-specific setting
                if var shieldOverrideEnabled = shieldState.isShieldOverrideEnabled(shield) {
                    if shield == .AllOff {
                        // Reversed...
                        shieldOverrideEnabled = !shieldOverrideEnabled
                        // TODO: Switch ^ to `isEnabled.toggle()` when we support Swift 4.2+
                    }
                    view.toggleSwitch.isOn = shieldOverrideEnabled
                }
            }
        }
        updateGlobalShieldState(shieldsView.shieldOverrideControl.toggleSwitch.isOn)
    }
    
    private func updateShieldBlockStats() {
        shieldsView.shieldsContainerStackView.adsTrackersStatView.valueLabel.text = String(shieldBlockStats.adsAndTrackers)
        shieldsView.shieldsContainerStackView.httpsUpgradesStatView.valueLabel.text = String(shieldBlockStats.httpsUpgrades)
        shieldsView.shieldsContainerStackView.scriptsBlockedStatView.valueLabel.text = String(shieldBlockStats.blockedScripts)
        shieldsView.shieldsContainerStackView.fingerprintingStatView.valueLabel.text = String(shieldBlockStats.fingerprintingProtection)
    }
    
    private func updateBraveShieldState(shield: BraveShieldState.Shield, on: Bool) {
        guard let url = url else { return }
        let allOff = shield == .AllOff
        // `.AllOff` uses inverse logic
        // Technically we set "all off" when the switch is OFF, unlike all the others
        let isOn = allOff ? !on : on
        Domain.setBraveShield(forUrl: url, shield: shield, isOn: isOn)
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
        preferredContentSize = CGSize(width: PopoverController.preferredPopoverWidth, height: shieldsView.stackView.bounds.height)
    }
    
    // MARK: -
    
    /// Groups the shield types with their control and global preference
    private lazy var shieldControlMapping: [(BraveShieldState.Shield, ToggleView, Preferences.Option<Bool>?)] = [
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
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        shieldControlMapping.forEach { shield, toggle, _ in
            toggle.valueToggled = { [unowned self] on in
                if shield == .AllOff {
                    // Update the content size
                    self.updateGlobalShieldState(on, animated: true)
                }
                // Localized / per domain toggles triggered here
                self.updateBraveShieldState(shield: shield, on: on)
                self.shieldsSettingsChanged?(self)
            }
        }
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        shieldsView.stackView.layoutIfNeeded()
        preferredContentSize = CGSize(width: PopoverController.preferredPopoverWidth, height: shieldsView.stackView.bounds.height)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}
