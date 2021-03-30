// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI
import Lottie

class BraveVPNRegionPickerViewController: UIViewController {
    private let regionList: [VPNRegion]
    
    private var overlayView: UIView?
    private let tableView: UITableView
    
    private enum Section: Int, CaseIterable {
        case automatic = 0
        case regionList
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    /// This group monitors vpn connection status.
    private var dispatchGroup: DispatchGroup?
    private var vpnRegionChangeSuccess = false
    
    private var isLoading: Bool = false {
        didSet {
            overlayView?.removeFromSuperview()
            
            navigationItem.hidesBackButton = isLoading
            
            // Prevent dismissing the modal by swipe when the VPN is being configured
            navigationController?.isModalInPresentation = isLoading
            
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
            overlay.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            
            overlayView = overlay
        }
    }
    
    init(serverList: [VPNRegion]) {
        self.regionList = serverList
            .sorted { $0.namePretty < $1.namePretty }
        
        if #available(iOS 14, *) {
            tableView = UITableView(frame: .zero, style: .insetGrouped)
        } else {
            tableView = UITableView(frame: .zero, style: .grouped)
        }
        
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init?(coder: NSCoder) { fatalError() }
    
    override func viewDidLoad() {
        title = Strings.VPN.regionPickerTitle
        
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(VPNRegionCell.self)
        
        NotificationCenter.default.addObserver(self, selector: #selector(vpnConfigChanged(notification:)),
                                               name: .NEVPNStatusDidChange, object: nil)
        
        view.addSubview(tableView)
        tableView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    @objc private func vpnConfigChanged(notification: NSNotification) {
        guard let connection = notification.object as? NEVPNConnection else { return }
        
        if connection.status == .connected {
            dispatchGroup?.leave()
            self.vpnRegionChangeSuccess = true
            dispatchGroup = nil
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        tableView.reloadData()
    }
}

// MARK: - UITableView Data Source & Delegate
extension BraveVPNRegionPickerViewController: UITableViewDelegate, UITableViewDataSource {
    func numberOfSections(in tableView: UITableView) -> Int {
        Section.allCases.count
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        section == Section.automatic.rawValue ? 1 : regionList.count
    }
    
    func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        if section == Section.automatic.rawValue {
            return Strings.VPN.regionPickerAutomaticDescription
        }
        
        return nil
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(for: indexPath) as VPNRegionCell
        cell.accessoryType = .none
        
        switch indexPath.section {
        case Section.automatic.rawValue:
            cell.textLabel?.text = Strings.VPN.regionPickerAutomaticModeCellText
            if Preferences.VPN.vpnRegionOverride.value == nil {
                cell.accessoryType = .checkmark
            }
        case Section.regionList.rawValue:
            guard let server = regionList[safe: indexPath.row] else { return cell }
            cell.textLabel?.text = server.namePretty
            if server.name == Preferences.VPN.vpnRegionOverride.value {
                cell.accessoryType = .checkmark
            }
        default:
            assertionFailure("Section count out of bounds")
        }
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        guard let region = regionList[safe: indexPath.row] else { return }
        
        // Tapped on the same cell, do nothing
        if (region.name == Preferences.VPN.vpnRegionOverride.value)
            || (indexPath.section == Section.automatic.rawValue && Preferences.VPN.vpnRegionOverride.value == nil) {
            return
        }
        
        tableView.reloadData()
        
        isLoading = true
        
        if indexPath.section == Section.automatic.rawValue {
            Preferences.VPN.vpnRegionOverride.value = nil
        } else {
            Preferences.VPN.vpnRegionOverride.value = region.name
        }
        
        self.dispatchGroup = DispatchGroup()
        
        BraveVPN.reconfigureVPN() { [weak self] success in
            guard let self = self else { return }
            
            func _showError() {
                DispatchQueue.main.async {
                    let alert = AlertController(title: Strings.VPN.regionPickerErrorTitle,
                                                message: Strings.VPN.regionPickerErrorMessage,
                                                preferredStyle: .alert)
                    let okAction = UIAlertAction(title: Strings.OKString, style: .default) { _ in
                        self.dismiss(animated: true)
                    }
                    alert.addAction(okAction)
                         
                    self.present(alert, animated: true)
                }
            }
            
            if !success {
                _showError()
            }
            
            // Changing vpn server settings takes lot of time,
            // and nothing we can do about it as it relies on Apple apis.
            // Here we observe vpn status and we show success alert if it connected,
            // otherwise an error alert is show if it did not manage to connect in 60 seconds.
            self.dispatchGroup?.enter()
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 60) {
                self.vpnRegionChangeSuccess = false
                self.dispatchGroup?.leave()
                self.dispatchGroup = nil
            }
            
            self.dispatchGroup?.notify(queue: .main) { [weak self] in
                guard let self = self else { return }
                if self.vpnRegionChangeSuccess {
                    
                    self.dismiss(animated: true) {
                        self.showSuccessAlert()
                    }
                } else {
                    _showError()
                }
            }
        }
    }
    
    private func showSuccessAlert() {
        let animation = AnimationView(name: "vpncheckmark").then {
            $0.bounds = CGRect(x: 0, y: 0, width: 300, height: 200)
            $0.contentMode = .scaleAspectFill
            $0.play()
        }
        
        let popup = AlertPopupView(imageView: animation,
                                   title: Strings.VPN.regionSwitchSuccessPopupText, message: "",
                                   titleWeight: .semibold, titleSize: 18,
                                   dismissHandler: { true })
        
        popup.showWithType(showType: .flyUp, autoDismissTime: 1.5)
    }
}

private class VPNRegionCell: UITableViewCell, TableViewReusable {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .value1, reuseIdentifier: reuseIdentifier)
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
