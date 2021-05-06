/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import Data
import BraveShared
import BraveRewards

private let log = Logger.browserLogger

class SyncSettingsTableViewController: UITableViewController {
    private var syncDeviceObserver: AnyObject?
    private var devices = [BraveSyncDevice]()
    
    private enum Sections: Int { case deviceList, buttons }
    
    /// A different logic and UI is used depending on what device was selected to remove and if it is the only device
    /// left in the Sync Chain.
    enum DeviceRemovalType { case lastDeviceLeft, currentDevice, otherDevice }
    
    /// After synchronization is completed, user needs to tap on `Done` to go back.
    /// Standard navigation is disabled then.
    var disableBackButton = false
    
    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        title = Strings.sync
        
        syncDeviceObserver = BraveSyncAPI.addDeviceStateObserver { [weak self] in
            self?.updateDeviceList()
        }
        
        let codeWords = BraveSyncAPI.shared.getSyncCode()
        BraveSyncAPI.shared.joinSyncGroup(codeWords: codeWords)
        BraveSyncAPI.shared.syncEnabled = true
        
        self.updateDeviceList()
        
        let text = UITextView().then {
            $0.text = Strings.syncSettingsHeader
            $0.textContainerInset = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 16)
            $0.isEditable = false
            $0.isSelectable = false
            $0.textColor = BraveUX.greyH
            $0.textAlignment = .center
            $0.font = UIFont.systemFont(ofSize: 15)
            $0.isScrollEnabled = false
            $0.backgroundColor = UIColor.clear
        }
        
        tableView.tableHeaderView = text
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        tableView.tableHeaderView?.sizeToFit()
        
        if disableBackButton {
            navigationController?.interactivePopGestureRecognizer?.isEnabled = false
            
            navigationItem.setHidesBackButton(true, animated: false)
            navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self,
                                                                action: #selector(doneTapped))
        }
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            updateThemeForUserInterfaceStyleChange()
        }
    }
    
    // MARK: - Button actions.
    
    @objc func doneTapped() {
        navigationController?.popToRootViewController(animated: true)
    }
    
    private func addAnotherDeviceAction() {
        let view = SyncSelectDeviceTypeViewController()
        view.syncInitHandler = { title, type in
            let view = SyncAddDeviceViewController(title: title, type: type)
            view.doneHandler = {
                self.navigationController?.popToViewController(self, animated: true)
            }
            self.navigationController?.pushViewController(view, animated: true)
        }
        navigationController?.pushViewController(view, animated: true)
    }
    
    private func removeDeviceAction() {
        let alert = UIAlertController(title: Strings.syncRemoveThisDeviceQuestion, message: Strings.syncRemoveThisDeviceQuestionDesc, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        alert.addAction(UIAlertAction(title: Strings.removeDevice, style: .destructive) { action in
            if !DeviceInfo.hasConnectivity() {
                self.present(SyncAlerts.noConnection, animated: true)
                return
            }
            
            BraveSyncAPI.shared.leaveSyncGroup()
            self.navigationController?.popToRootViewController(animated: true)
        })
        
        navigationController?.present(alert, animated: true, completion: nil)
    }
    
    // MARK: - Table view methods.
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        defer { tableView.deselectRow(at: indexPath, animated: true) }
        
        if indexPath.section == Sections.buttons.rawValue {
            addAnotherDeviceAction()
            return
        }
        
        let devices = self.devices
        if devices.isEmpty {
            return
        }
        
        guard let device = devices[safe: indexPath.row] else {
            return
        }
        
        guard device.isCurrentDevice || (!device.isCurrentDevice && device.supportsSelfDelete) else {
            // See: `BraveSyncDevice.remove()` for more info.
            return
        }
        
        let actionSheet = UIAlertController(title: device.name, message: nil, preferredStyle: .actionSheet)
        if UIDevice.current.userInterfaceIdiom == .pad {
            let cell = tableView.cellForRow(at: indexPath)
            actionSheet.popoverPresentationController?.sourceView = cell
            actionSheet.popoverPresentationController?.sourceRect = cell?.bounds ?? .zero
            actionSheet.popoverPresentationController?.permittedArrowDirections = [.up, .down]
        }
        
        let removeAction = UIAlertAction(title: Strings.syncRemoveDeviceAction, style: .destructive) { _ in
            if !DeviceInfo.hasConnectivity() {
                self.present(SyncAlerts.noConnection, animated: true)
                return
            }
            
            var alertType = DeviceRemovalType.otherDevice
            
            if devices.count == 1 {
                alertType = .lastDeviceLeft
            } else if device.isCurrentDevice {
                alertType = .currentDevice
            }
            
            self.presentAlertPopup(for: alertType, device: device)
        }
        
        let cancelAction = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
        
        actionSheet.addAction(removeAction)
        actionSheet.addAction(cancelAction)
        
        present(actionSheet, animated: true)
    }
    
    private func presentAlertPopup(for type: DeviceRemovalType, device: BraveSyncDevice) {
        var title: String?
        var message: String?
        var removeButtonName: String?
        let deviceName = device.name ?? Strings.syncRemoveDeviceDefaultName
        
        switch type {
        case .lastDeviceLeft:
            title = String(format: Strings.syncRemoveLastDeviceTitle, deviceName)
            message = Strings.syncRemoveLastDeviceMessage
            removeButtonName = Strings.syncRemoveLastDeviceRemoveButtonName
        case .currentDevice:
            title = String(format: Strings.syncRemoveCurrentDeviceTitle, "\(deviceName) (\(Strings.syncThisDevice))")
            message = Strings.syncRemoveCurrentDeviceMessage
            removeButtonName = Strings.removeDevice
        case .otherDevice:
            title = String(format: Strings.syncRemoveOtherDeviceTitle, deviceName)
            message = Strings.syncRemoveOtherDeviceMessage
            removeButtonName = Strings.removeDevice
        }
        
        guard let popupTitle = title, let popupMessage = message, let popupButtonName = removeButtonName else { fatalError() }
        
        let popup = AlertPopupView(imageView: nil, title: popupTitle, message: popupMessage)
        let fontSize: CGFloat = 15
        
        popup.addButton(title: Strings.cancelButtonTitle, fontSize: fontSize) { return .flyDown }
        popup.addButton(title: popupButtonName, type: .destructive, fontSize: fontSize) {
            switch type {
            case .lastDeviceLeft, .currentDevice:
                BraveSyncAPI.shared.leaveSyncGroup()
                self.navigationController?.popToRootViewController(animated: true)
            case .otherDevice:
                device.remove()
            }
            return .flyDown
        }
        
        popup.showWithType(showType: .flyUp)
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 2
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        let deviceCount = devices.count
        let buttonsCount = 1
        
        return section == Sections.buttons.rawValue ? buttonsCount : deviceCount
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        configureCell(cell, atIndexPath: indexPath)
        
        return cell
    }
    
    private func configureCell(_ cell: UITableViewCell, atIndexPath indexPath: IndexPath) {
        if devices.isEmpty {
            log.error("No sync devices to configure.")
            return
        }
        
        switch indexPath.section {
        case Sections.deviceList.rawValue:
            guard let device = devices[safe: indexPath.row] else {
                log.error("Invalid device to configure.")
                return
            }
            
            guard let name = device.name else { break }
            let deviceName = device.isCurrentDevice ? "\(name) (\(Strings.syncThisDevice))" : name
            
            cell.textLabel?.text = deviceName
        case Sections.buttons.rawValue:
            configureButtonCell(cell)
        default:
            log.error("Section index out of bounds.")
        }
    }
    
    private func configureButtonCell(_ cell: UITableViewCell) {
        // By default all cells have separators with left inset. Our buttons are based on table cells,
        // we need to style them so they look more like buttons with full width separator between them.
        cell.preservesSuperviewLayoutMargins = false
        cell.separatorInset = UIEdgeInsets.zero
        cell.layoutMargins = UIEdgeInsets.zero
        
        cell.textLabel?.do {
            $0.text = Strings.syncAddAnotherDevice
            $0.textAlignment = .center
            $0.appearanceTextColor = BraveUX.braveOrange
            $0.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.regular)
        }
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        
        switch section {
        case Sections.deviceList.rawValue:
            return Strings.devices.uppercased()
        default:
            return nil
        }
    }
}

extension SyncSettingsTableViewController {
    private func updateDeviceList() {
        if let json = BraveSyncAPI.shared.getDeviceListJSON(), let data = json.data(using: .utf8) {
            do {
                let devices = try JSONDecoder().decode([BraveSyncDevice].self, from: data)
                self.devices = devices
                
                if self.devices.count <= 0 {
                    // Technically we shouldn't be calling this function..
                    // If Desktop deletes an iOS device from the chain, we're already removed..
                    // We should be just updating our UI and calling
                    // `Preferences.Chromium.syncEnabled.value = false`
                    // But I don't have a TRUE way to tell if the current device
                    // was removed from the sync chain because `OnSelfDeviceInfoDeleted` has no callback.
                    // So instead, we call `reset_sync` which won't hurt.
                    BraveSyncAPI.shared.leaveSyncGroup()
                    self.navigationController?.popToRootViewController(animated: true)
                } else {
                    self.tableView.reloadData()
                }
            } catch {
                log.error(error)
            }
        } else {
            log.error("Something went wrong while retrieving Sync Devices..")
        }
    }
}

private struct BraveSyncDevice: Codable {
    let chromeVersion: String
    let hasSharingInfo: Bool
    let id: String
    let guid: String
    let isCurrentDevice: Bool
    let supportsSelfDelete: Bool
    let lastUpdatedTimestamp: TimeInterval
    let name: String?
    let os: String
    let sendTabToSelfReceivingEnabled: Bool
    let type: String
    
    func remove() {
        // Devices other than `isCurrentDevice` can only be deleted if
        // `supportsSelfDelete` is true. Attempting to delete another device
        // from the sync chain that does not support it, will crash or
        // have undefined behaviour as the other device won't know that
        // it was deleted.
        BraveSyncAPI.shared.removeDeviceFromSyncGroup(deviceGuid: self.guid)
    }
}
