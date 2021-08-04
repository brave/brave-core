/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import Data
import BraveShared
import BraveRewards

private let log = Logger.browserLogger

class SyncSettingsTableViewController: UITableViewController {
    
    // MARK: Lifecycle
    
    init(showDoneButton: Bool = false) {
        self.showDoneButton = showDoneButton
        super.init(style: .grouped)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
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
        
        tableView.tableHeaderView = makeInformationTextView(with: Strings.syncSettingsHeader)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        tableView.tableHeaderView?.sizeToFit()
        
        if showDoneButton {
            navigationController?.interactivePopGestureRecognizer?.isEnabled = false
            navigationItem.setHidesBackButton(true, animated: false)
            navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self,
                                                                action: #selector(doneTapped))
        }
    }
    
    // MARK: Private
    
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
    
    private var syncDeviceObserver: AnyObject?
    private var devices = [BraveSyncDevice]()
    
    private enum Sections: Int {
        case deviceList, deviceActions, syncTypes
    }
    
    private enum SyncDataTypes: Int {
        case bookmarks = 0, history
    }
    
    /// A different logic and UI is used depending on what device was selected to remove and if it is the only device
    /// left in the Sync Chain.
    private enum DeviceRemovalType {
        case lastDeviceLeft, currentDevice, otherDevice
    }
    
    /// After synchronization is completed, user needs to tap on `Done` to go back.
    /// Standard navigation is disabled then.
    private var showDoneButton = false
    
    // MARK: Actions
    
    @objc private func doneTapped() {
        navigationController?.popToRootViewController(animated: true)
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
    
    @objc private func didToggleSyncType(_ toggle: UISwitch) {
        switch toggle.tag {
            case SyncDataTypes.bookmarks.rawValue:
                Preferences.Chromium.syncBookmarksEnabled.value = toggle.isOn
            case SyncDataTypes.history.rawValue:
                Preferences.Chromium.syncHistoryEnabled.value = toggle.isOn
            default:
                return
        }
        
        BraveSyncAPI.shared.enableSyncTypes()
    }
}

// MARK: - UITableViewDelegate, UITableViewDataSource

extension SyncSettingsTableViewController {
        
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        defer { tableView.deselectRow(at: indexPath, animated: true) }
        
        if indexPath.section == Sections.deviceActions.rawValue {
            addAnotherDevice()
            return
        }
        
        guard !devices.isEmpty, let device = devices[safe: indexPath.row] else {
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
            
            if self.devices.count == 1 {
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
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 3
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch section {
            case Sections.deviceList.rawValue:
                return devices.count
            case Sections.deviceActions.rawValue:
                return 1
            case Sections.syncTypes.rawValue:
                return 2
            default:
                return 0
        }
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        configureCell(cell, atIndexPath: indexPath)
        
        return cell
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch section {
            case Sections.deviceList.rawValue:
                return Strings.devices
            case Sections.syncTypes.rawValue:
                return Strings.Sync.syncSettingsTitle
            default:
                return nil
        }
    }
    
    override func tableView(_ tableView: UITableView, viewForFooterInSection sectionIndex: Int) -> UIView? {
        switch sectionIndex {
            case Sections.syncTypes.rawValue:
                return makeInformationTextView(with: Strings.Sync.syncConfigurationInformationText)
            default:
                return nil
        }
    }

    override func tableView(_ tableView: UITableView, heightForFooterInSection sectionIndex: Int) -> CGFloat {
        switch sectionIndex {
            case Sections.syncTypes.rawValue:
                return UITableView.automaticDimension
            default:
                return 0
        }
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
                cell.textLabel?.textColor = .braveLabel
            case Sections.deviceActions.rawValue:
                configureButtonCell(cell)
            case Sections.syncTypes.rawValue:
                configureToggleCell(cell, for: SyncDataTypes(rawValue: indexPath.row) ?? .bookmarks)
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
            $0.textColor = .braveOrange
            $0.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.regular)
        }
    }
    
    private func configureToggleCell(_ cell: UITableViewCell, for syncType: SyncDataTypes) {
        let toggle = UISwitch().then {
            $0.addTarget(self, action: #selector(didToggleSyncType), for: .valueChanged)
            $0.isOn = isOn(syncType: syncType)
            $0.tag = syncType.rawValue
        }
        
        cell.do {
            $0.textLabel?.text = syncType == .bookmarks ? Strings.bookmarks : Strings.historyMenuItem
            $0.accessoryView = toggle
            $0.selectionStyle = .none
        }
        
        cell.separatorInset = .zero
        
        func isOn(syncType: SyncDataTypes) -> Bool {
            switch syncType {
                case .bookmarks:
                    return Preferences.Chromium.syncBookmarksEnabled.value
                case .history:
                    return Preferences.Chromium.syncHistoryEnabled.value
            }
        }
    }
    
    private func makeInformationTextView(with info: String) -> UITextView {
        return UITextView().then {
            $0.text = info
            $0.textContainerInset = UIEdgeInsets(top: 16, left: 16, bottom: 0, right: 16)
            $0.isEditable = false
            $0.isSelectable = false
            $0.textColor = .secondaryBraveLabel
            $0.textAlignment = .center
            $0.font = UIFont.systemFont(ofSize: 15)
            $0.isScrollEnabled = false
            $0.backgroundColor = .clear
        }
    }
}

// MARK: - Sync Device Functionality

extension SyncSettingsTableViewController {
    
    private func updateDeviceList() {
        if let json = BraveSyncAPI.shared.getDeviceListJSON(), let data = json.data(using: .utf8) {
            do {
                let devices = try JSONDecoder().decode([BraveSyncDevice].self, from: data)
                self.devices = devices
                
                if devices.count <= 0 {
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
    
    private func addAnotherDevice() {
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
        let alert = UIAlertController(title: Strings.syncRemoveThisDeviceQuestion,
                                      message: Strings.syncRemoveThisDeviceQuestionDesc,
                                      preferredStyle: .alert)
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
}

