// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Storage
import SnapKit
import Shared
import BraveShared
import Data
import BraveUI

/// Displays shield settings and shield stats for a given URL
class ShieldsViewController: UIViewController, PopoverContentComponent, Themeable {
    
    let tab: Tab
    private lazy var url: URL? = {
        guard let _url = tab.url else { return nil }
        
        if _url.isErrorPageURL {
            return _url.originalURLFromErrorURL
        }
        
        return _url
    }()
    
    var shieldsSettingsChanged: ((ShieldsViewController) -> Void)?
    var showGlobalShieldsSettings: ((ShieldsViewController) -> Void)?
    
    private var statsUpdateObservable: AnyObject?
    
    /// Create with an initial URL and block stats (or nil if you are not on any web page)
    init(tab: Tab) {
        self.tab = tab
        
        super.init(nibName: nil, bundle: nil)
        
        tab.contentBlocker.statsDidChange = { [weak self] _ in
            self?.updateShieldBlockStats()
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        navigationController?.setNavigationBarHidden(true, animated: animated)
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        navigationController?.setNavigationBarHidden(false, animated: animated)
    }
    
    private var shieldsUpSwitch: ShieldsSwitch {
        return shieldsView.simpleShieldView.shieldsSwitch
    }
    
    // MARK: - State
    
    private func updateToggleStatus() {
        var domain: Domain?
        if let url = url {
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
        }

        if let domain = domain {
            shieldsUpSwitch.isOn = !domain.isShieldExpected(.AllOff, considerAllShieldsOption: false)
        } else {
            shieldsUpSwitch.isOn = true
        }
        
        shieldControlMapping.forEach { shield, view, option in
            // Updating based on global settings
            if let option = option {
                // Sets the default setting
                view.toggleSwitch.isOn = option.value
            }
            // Domain specific overrides after defaults have already been setup
            
            if let domain = domain {
                // site-specific shield has been overridden, update
                view.toggleSwitch.isOn = domain.isShieldExpected(shield, considerAllShieldsOption: false)
            }
        }
        updateGlobalShieldState(shieldsUpSwitch.isOn)
    }
    
    private func updateShieldBlockStats() {
        shieldsView.simpleShieldView.blockCountView.countLabel.text = String(
            tab.contentBlocker.stats.adCount +
            tab.contentBlocker.stats.trackerCount +
            tab.contentBlocker.stats.httpsCount +
            tab.contentBlocker.stats.scriptCount +
            tab.contentBlocker.stats.fingerprintingCount
        )
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
        shieldsView.simpleShieldView.statusLabel.text = on ?
            Strings.Shields.statusValueUp.uppercased() :
            Strings.Shields.statusValueDown.uppercased()
        
        // Whether or not shields are available for this URL.
        let isShieldsAvailable = url?.isLocal == false
        // If shields aren't available, we don't show the switch and show the "off" state
        let shieldsEnabled = isShieldsAvailable ? on : false
        if animated {
            var partOneViews: [UIView]
            var partTwoViews: [UIView]
            if shieldsEnabled {
                partOneViews = [self.shieldsView.simpleShieldView.shieldsDownStackView]
                partTwoViews = [
                    self.shieldsView.simpleShieldView.blockCountView,
                    self.shieldsView.simpleShieldView.footerLabel,
                    self.shieldsView.advancedControlsBar
                ]
                if advancedControlsShowing {
                    partTwoViews.append(self.shieldsView.advancedShieldView)
                }
            } else {
                partOneViews = [
                    self.shieldsView.simpleShieldView.blockCountView,
                    self.shieldsView.simpleShieldView.footerLabel,
                    self.shieldsView.advancedControlsBar,
                ]
                if advancedControlsShowing {
                    partOneViews.append(self.shieldsView.advancedShieldView)
                }
                partTwoViews = [self.shieldsView.simpleShieldView.shieldsDownStackView]
            }
            // Step 1, hide
            UIView.animate(withDuration: 0.1, animations: {
                partOneViews.forEach { $0.alpha = 0.0 }
            }, completion: { _ in
                partOneViews.forEach {
                    $0.alpha = 1.0
                    $0.isHidden = true
                }
                partTwoViews.forEach {
                    $0.alpha = 0.0
                    $0.isHidden = false
                }
                UIView.animate(withDuration: 0.15, animations: {
                    partTwoViews.forEach { $0.alpha = 1.0 }
                })
                
                self.updatePreferredContentSize()
            })
        } else {
            shieldsView.simpleShieldView.blockCountView.isHidden = !shieldsEnabled
            shieldsView.simpleShieldView.footerLabel.isHidden = !shieldsEnabled
            shieldsView.simpleShieldView.shieldsDownStackView.isHidden = shieldsEnabled
            shieldsView.advancedControlsBar.isHidden = !shieldsEnabled
            
            updatePreferredContentSize()
        }
    }
    
    private func updateContentView(to view: UIView, animated: Bool) {
        if animated {
            UIView.animate(withDuration: shieldsView.contentView == nil ? 0 : 0.1, animations: {
                self.shieldsView.contentView?.alpha = 0.0
            }, completion: { _ in
                self.shieldsView.contentView = view
                view.alpha = 0
                self.updatePreferredContentSize()
                UIView.animate(withDuration: 0.1) {
                    view.alpha = 1.0
                }
            })
        } else {
            shieldsView.contentView = view
        }
    }
    
    private func updatePreferredContentSize() {
        guard let visibleView = shieldsView.contentView else { return }
        let width = min(360, UIScreen.main.bounds.width - 20)
        // Ensure the a static width is given to the main view so we can calculate the height
        // correctly when we force a layout
        shieldsView.frame = CGRect(origin: .zero, size: .init(width: width, height: 0))
        shieldsView.setNeedsLayout()
        shieldsView.layoutIfNeeded()
        
        preferredContentSize = CGSize(
            width: width,
            height: visibleView.bounds.height
        )
    }
    
    // MARK: -
    
    /// Groups the shield types with their control and global preference
    private lazy var shieldControlMapping: [(BraveShield, AdvancedShieldsView.ToggleView, Preferences.Option<Bool>?)] = [
        (.AdblockAndTp, shieldsView.advancedShieldView.adsTrackersControl, Preferences.Shields.blockAdsAndTracking),
        (.SafeBrowsing, shieldsView.advancedShieldView.blockMalwareControl, Preferences.Shields.blockPhishingAndMalware),
        (.NoScript, shieldsView.advancedShieldView.blockScriptsControl, Preferences.Shields.blockScripts),
        (.HTTPSE, shieldsView.advancedShieldView.httpsUpgradesControl, Preferences.Shields.httpsEverywhere),
        (.FpProtection, shieldsView.advancedShieldView.fingerprintingControl, Preferences.Shields.fingerprintingProtection),
    ]
    
    var shieldsView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View()
        applyTheme(Theme.of(nil))
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if #available(iOS 13.0, *) {
            if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
                applyTheme(Theme.of(nil))
            }
        }
    }
    
    func applyTheme(_ theme: Theme) {
        shieldsView.applyTheme(theme)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        if let url = url {
            shieldsView.simpleShieldView.faviconImageView.loadFavicon(for: url)
        } else {
            shieldsView.simpleShieldView.faviconImageView.isHidden = true
        }
        shieldsView.simpleShieldView.hostLabel.text = url?.normalizedHost()
        shieldsView.reportBrokenSiteView.urlLabel.text = url?.domainURL.absoluteString
        shieldsView.simpleShieldView.shieldsSwitch.addTarget(self, action: #selector(shieldsOverrideSwitchValueChanged), for: .valueChanged)
        shieldsView.advancedShieldView.siteTitle.titleLabel.text = url?.normalizedHost()?.uppercased()
        shieldsView.advancedShieldView.globalControlsButton.addTarget(self, action: #selector(tappedGlobalShieldsButton), for: .touchUpInside)
        
        shieldsView.advancedControlsBar.addTarget(self, action: #selector(tappedAdvancedControlsBar), for: .touchUpInside)
        shieldsView.simpleShieldView.blockCountView.infoButton.addTarget(self, action: #selector(tappedAboutShieldsButton), for: .touchUpInside)
        
        shieldsView.simpleShieldView.reportSiteButton.addTarget(self, action: #selector(tappedReportSiteButton), for: .touchUpInside)
        shieldsView.reportBrokenSiteView.cancelButton.addTarget(self, action: #selector(tappedCancelReportingButton), for: .touchUpInside)
        shieldsView.reportBrokenSiteView.submitButton.addTarget(self, action: #selector(tappedSubmitReportingButton), for: .touchUpInside)
        
        updateShieldBlockStats()
        
        navigationController?.setNavigationBarHidden(true, animated: false)
        
        updateToggleStatus()
        
        if advancedControlsShowing && shieldsUpSwitch.isOn {
            shieldsView.advancedShieldView.isHidden = false
            shieldsView.advancedControlsBar.isShowingAdvancedControls = true
            updatePreferredContentSize()
        }
        
        shieldControlMapping.forEach { shield, toggle, option in
            toggle.valueToggled = { [unowned self] on in
                // Localized / per domain toggles triggered here
                self.updateBraveShieldState(shield: shield, on: on, option: option)
                self.shieldsSettingsChanged?(self)
            }
        }
    }
    
    @objc private func shieldsOverrideSwitchValueChanged() {
        let isOn = shieldsUpSwitch.isOn
        self.updateGlobalShieldState(isOn, animated: true)
        self.updateBraveShieldState(shield: .AllOff, on: isOn, option: nil)
        self.shieldsSettingsChanged?(self)
    }
    
    private var advancedControlsShowing: Bool {
        Preferences.Shields.advancedControlsVisible.value
    }
    
    @objc private func tappedAdvancedControlsBar() {
        Preferences.Shields.advancedControlsVisible.value.toggle()
        UIView.animate(withDuration: 0.25) {
            self.shieldsView.advancedShieldView.isHidden.toggle()
        }
        updatePreferredContentSize()
    }
    
    @objc private func tappedAboutShieldsButton() {
        let aboutShields = AboutShieldsViewController(tab: tab)
        aboutShields.applyTheme(Theme.of(tab))
        aboutShields.preferredContentSize = preferredContentSize
        navigationController?.pushViewController(aboutShields, animated: true)
    }
    
    @objc private func tappedReportSiteButton() {
        updateContentView(to: shieldsView.reportBrokenSiteView, animated: true)
    }
    
    @objc private func tappedCancelReportingButton() {
        updateContentView(to: shieldsView.stackView, animated: true)
    }
    
    @objc private func tappedSubmitReportingButton() {
        if let url = url {
            WebcompatReporter.reportIssue(on: url)
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { [weak self] in
                guard let self = self, !self.isBeingDismissed else { return }
                self.dismiss(animated: true)
            }
        }
        updateContentView(to: shieldsView.siteReportedView, animated: true)
    }
    
    @objc private func tappedGlobalShieldsButton() {
        showGlobalShieldsSettings?(self)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
}
