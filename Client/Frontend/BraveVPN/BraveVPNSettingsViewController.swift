// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared

private let log = Logger.browserLogger

class BraveVPNSettingsViewController: TableViewController {
    
    var faqButtonTapped: (() -> Void)?
    
    init() {
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    private let manageSubcriptionURL = URL(string: "https://apps.apple.com/account/subscriptions")
    
    // Cell/section tags so we can update them dynamically.
    private let serverSectionId = "server"
    private let hostCellId = "host"
    private let locationCellId = "location"
    private let resetCellId = "reset"
    private let vpnStatusSectionCellId = "vpnStatus"
    
    private var vpnConnectionStatusSwitch: SwitchAccessoryView?
    
    private var vpnReconfigurationPending = false {
        didSet {
            DispatchQueue.main.async {
                self.vpnConnectionStatusSwitch?.isEnabled = !self.vpnReconfigurationPending
            }
        }
    }
    
    /// This is local variable only to prevents users from spamming the reset configuration button.
    private var lastTimeVPNWasResetted: Date?
    
    /// View to show when the vpn config reset is pending.
    private var overlayView: UIView?
    
    private var isLoading: Bool = false {
        didSet {
            overlayView?.removeFromSuperview()
            
            if !isLoading { return }
            
            let overlay = UIView().then {
                $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
                let activityIndicator = UIActivityIndicatorView().then { indicator in
                    indicator.startAnimating()
                    indicator.autoresizingMask = [.flexibleWidth, .flexibleHeight]
                }
                
                $0.addSubview(activityIndicator)
            }
            
            view.addSubview(overlay)
            overlay.frame = CGRect(size: tableView.contentSize)
            
            overlayView = overlay
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.VPN.vpnName
        NotificationCenter.default.addObserver(self, selector: #selector(vpnConfigChanged),
                                               name: .NEVPNStatusDidChange, object: nil)
        
        let switchView = SwitchAccessoryView(initialValue: BraveVPN.isConnected, valueChange: { vpnOn in
            if vpnOn {
                BraveVPN.reconnect()
            } else {
                BraveVPN.disconnect()
            }
        })
        
        self.vpnConnectionStatusSwitch = switchView
        
        let vpnStatusSection = Section(rows: [
            Row(text: Strings.VPN.settingsVPNEnabled,
                accessory: .view(switchView), uuid: vpnStatusSectionCellId)
        ], uuid: vpnStatusSectionCellId)

        let subscriptionStatus = BraveVPN.hasExpired == true ?
            Strings.VPN.subscriptionStatusExpired : BraveVPN.subscriptionName
        
        let expiration = BraveVPN.hasExpired == true ? "-" : expirationDate
        
        let subscriptionSection =
            Section(header: .title(Strings.VPN.settingsSubscriptionSection),
                    rows: [Row(text: Strings.VPN.settingsSubscriptionStatus,
                               detailText: subscriptionStatus),
                           Row(text: Strings.VPN.settingsSubscriptionExpiration, detailText: expiration),
                           Row(text: Strings.VPN.settingsManageSubscription,
                               selection: { [unowned self] in
                                guard let url = self.manageSubcriptionURL else { return }
                                if UIApplication.shared.canOpenURL(url) {
                                    // Opens Apple's 'manage subscription' screen.
                                    UIApplication.shared.open(url, options: [:])
                                }
                            }, cellClass: ButtonCell.self)])
        
        let location = BraveVPN.serverLocation ?? "-"
        
        let serverSection =
            Section(header: .title(Strings.VPN.settingsServerSection),
                    rows: [Row(text: Strings.VPN.settingsServerHost, detailText: hostname, uuid: hostCellId),
                           Row(text: Strings.VPN.settingsServerLocation, detailText: location,
                               uuid: locationCellId),
                           Row(text: Strings.VPN.settingsResetConfiguration,
                               selection: resetConfigurationTapped,
                               cellClass: ButtonCell.self, uuid: resetCellId)],
                    uuid: serverSectionId)
        
        let techSupportSection = Section(rows:
            [Row(text: Strings.VPN.settingsContactSupport, selection: { [unowned self] in
                self.sendContactSupportEmail()
                }, accessory: .disclosureIndicator, cellClass: ButtonCell.self)])
        
        let termsSection = Section(rows:
            [Row(text: Strings.VPN.settingsFAQ, selection: { [unowned self] in
                self.faqButtonTapped?()
                }, accessory: .disclosureIndicator, cellClass: ButtonCell.self)])

        dataSource.sections = [vpnStatusSection,
                               subscriptionSection,
                               serverSection,
                               techSupportSection,
                               termsSection]
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    private var hostname: String {
        BraveVPN.hostname?.components(separatedBy: ".").first ?? "-"
    }
    
    private var expirationDate: String {
        guard let expirationDate = Preferences.VPN.expirationDate.value else {
            return ""
        }
        
        let dateFormatter = DateFormatter().then {
            $0.locale = Locale.current
            $0.dateStyle = .short
        }
        
        return dateFormatter.string(from: expirationDate)
    }
    
    private func updateServerInfo() {
        guard let hostIndexPath = dataSource
            .indexPath(rowUUID: hostCellId, sectionUUID: serverSectionId) else { return }
        
        guard let locationIndexPath = dataSource
            .indexPath(rowUUID: locationCellId, sectionUUID: serverSectionId) else { return }
        
        dataSource.sections[hostIndexPath.section].rows[hostIndexPath.row]
            .detailText = hostname
        dataSource.sections[locationIndexPath.section].rows[locationIndexPath.row]
            .detailText = BraveVPN.serverLocation ?? "-"
    }
    
    private func sendContactSupportEmail() {
        navigationController?.pushViewController(BraveVPNContactFormViewController(), animated: true)
    }
    
    private func resetConfigurationTapped() {
        if BraveVPN.hasExpired == true { return }
        
        if let lastResetTime = lastTimeVPNWasResetted {
            let timeBetweenLastReset = Date().timeIntervalSince(lastResetTime)
            if timeBetweenLastReset < 60 { return }
        }
        
        let alert = UIAlertController(title: Strings.VPN.vpnResetAlertTitle,
                                      message: Strings.VPN.vpnResetAlertBody,
                                      preferredStyle: .actionSheet)
        
        let cancel = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)
        let reset = UIAlertAction(title: Strings.VPN.vpnResetButton, style: .destructive,
                                  handler: { [weak self] _ in
                                    guard let self = self else { return }
                                    self.vpnReconfigurationPending = true
                                    self.lastTimeVPNWasResetted = Date()
                                    self.isLoading = true
                                    log.debug("Reconfiguring the vpn")
                                    
                                    BraveVPN.reconfigureVPN() { success in
                                        log.debug("Reconfiguration suceedeed: \(success)")
                                        // Small delay before unlocking UI because enabling vpn
                                        // takes a moment after we call to connect to the vpn.
                                        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                                            self.isLoading = false
                                            self.vpnReconfigurationPending = false
                                            self.updateServerInfo()
                                            if !success {
                                                self.showVPNResetErrorAlert()
                                            }
                                        }
                                    }
        })
        
        alert.addAction(cancel)
        alert.addAction(reset)
        
        if UIDevice.current.userInterfaceIdiom == .pad {
            alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
            
            if let resetCellIndexPath = dataSource
                .indexPath(rowUUID: resetCellId, sectionUUID: serverSectionId) {
                let cell = tableView.cellForRow(at: resetCellIndexPath)
                
                alert.popoverPresentationController?.sourceView = cell
                alert.popoverPresentationController?.sourceRect = cell?.bounds ?? .zero
            } else {
                alert.popoverPresentationController?.sourceView = self.view
                alert.popoverPresentationController?.sourceRect = self.view.bounds
            }
        }
        
        present(alert, animated: true)
    }
    
    private func showVPNResetErrorAlert() {
        let alert = UIAlertController(title: Strings.VPN.resetVPNErrorTitle,
                                      message: Strings.VPN.resetVPNErrorBody, preferredStyle: .alert)
        
        let okAction = UIAlertAction(title: Strings.OKString, style: .default)
        alert.addAction(okAction)
        
        present(alert, animated: true)
    }
    
    @objc func vpnConfigChanged() {
        vpnConnectionStatusSwitch?.isOn = BraveVPN.isConnected
    }
}
