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
    var url: URL {
        didSet {
            updateToggleStatus()
        }
    }
    /// The blocked stats
    var shieldBlockStats: ShieldBlockedStats? {
        didSet {
            updateShieldBlockStats()
        }
    }
    
    init(url: URL, shieldBlockStats: ShieldBlockedStats?) {
        self.url = url
        self.shieldBlockStats = shieldBlockStats
        
        super.init(nibName: nil, bundle: nil)
        
        shieldsView.shieldsContainerStackView.hostLabel.text = url.normalizedHost
        
        updateToggleStatus()
        updateShieldBlockStats()
    }
    
    // MARK: - State
    
    private func updateToggleStatus() {
        shieldControlMapping.forEach { shield, view, option in
            if let option = option {
                // Sets the default setting
                view.toggleSwitch.isOn = option.value
            } else {
                // The only "shield" that doesn't actually have a global preference is the option to disable all shields
                // Therefore its the only shield we have to give a default value of true
                view.toggleSwitch.isOn = shield == .AllOff
            }
            if let host = url.normalizedHost, let shieldState = BraveShieldState.getStateForDomain(host) {
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
        guard let stats = shieldBlockStats else {
            // All 0
            shieldsView.shieldsContainerStackView.adsTrackersStatView.valueLabel.text = "0"
            shieldsView.shieldsContainerStackView.httpsUpgradesStatView.valueLabel.text = "0"
            shieldsView.shieldsContainerStackView.scriptsBlockedStatView.valueLabel.text = "0"
            shieldsView.shieldsContainerStackView.fingerprintingStatView.valueLabel.text = "0"
            return
        }
        shieldsView.shieldsContainerStackView.adsTrackersStatView.valueLabel.text = String(stats.adsAndTrackers)
        shieldsView.shieldsContainerStackView.httpsUpgradesStatView.valueLabel.text = String(stats.httpsUpgrades)
        shieldsView.shieldsContainerStackView.scriptsBlockedStatView.valueLabel.text = String(stats.blockedScripts)
        shieldsView.shieldsContainerStackView.fingerprintingStatView.valueLabel.text = String(stats.fingerprintingProtection)
    }
    
    private func updateBraveShieldState(shield: BraveShieldState.Shield, on: Bool) {
        guard let domain = url.normalizedHost else { return }
        if shield == .AllOff {
            // Technically we set "all off" when the switch is OFF, unlike all the others
            BraveShieldState.set(forDomain: domain, state: (.AllOff, !on))
        } else {
            BraveShieldState.set(forDomain: domain, state: (shield, on))
        }
    }
    
    private func updateGlobalShieldState(_ on: Bool, animated: Bool = false) {
        let updateBlock = {
            self.shieldsView.shieldsContainerStackView.isHidden = !on
            self.shieldsView.shieldsContainerStackView.alpha = on ? 1.0 : 0.0
            self.shieldsView.overviewStackView.isHidden = on
            self.shieldsView.overviewStackView.alpha = on ? 0.0 : 1.0
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
        preferredContentSize = CGSize(width: 320.0, height: shieldsView.stackView.bounds.height)
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
        return view as! View
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
                self.updateBraveShieldState(shield: shield, on: on)
            }
        }
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        shieldsView.stackView.layoutIfNeeded()
        preferredContentSize = CGSize(width: 320.0, height: shieldsView.stackView.bounds.height)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}
