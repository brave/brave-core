/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

class SyncSettingsTableViewController: UITableViewController {
    private var frc: NSFetchedResultsController<Device>?
    
    private enum Sections: Int { case deviceList, buttons }
    
    /// A different logic and UI is used depending on what device was selected to remove and if it is the only device
    /// left in the Sync Chain.
    enum DeviceRemovalType { case lastDeviceLeft, currentDevice, otherDevice }
    
    /// Handles dismissing parent view controller.
    var dismissHandler: (() -> Void)?
    
    /// After synchronization is completed, user needs to tap on `Done` to go back.
    /// Standard navigation is disabled then.
    var disableBackButton = false
    
    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        title = Strings.Sync
        
        frc = Device.frc()
        frc?.delegate = self
        
        do {
            try frc?.performFetch()
        } catch {
            log.error("frc fetch error: \(error)")
        }
        
        let text = UITextView().then {
            $0.text = Strings.SyncSettingsHeader
            $0.textContainerInset = UIEdgeInsets(top: 36, left: 16, bottom: 16, right: 16)
            $0.isEditable = false
            $0.isSelectable = false
            $0.textColor = BraveUX.GreyH
            $0.textAlignment = .center
            $0.font = UIFont.systemFont(ofSize: 15)
            $0.sizeToFit()
            $0.backgroundColor = UIColor.clear
        }
        
        tableView.tableHeaderView = text
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        if disableBackButton {
            navigationController?.interactivePopGestureRecognizer?.isEnabled = false
            
            navigationItem.setHidesBackButton(true, animated: false)
            navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self,
                                                                action: #selector(doneTapped))
        }
    }
    
    // MARK: - Button actions.
    
    @objc func doneTapped() {
        dismissHandler?()
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
        let alert = UIAlertController(title: Strings.SyncRemoveThisDeviceQuestion, message: Strings.SyncRemoveThisDeviceQuestionDesc, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
        alert.addAction(UIAlertAction(title: Strings.RemoveDevice, style: .destructive) { action in
            Sync.shared.leaveSyncGroup()
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
        
        guard let frc = frc, let deviceCount = frc.fetchedObjects?.count else { return }
        let device = frc.object(at: indexPath)
        
        let actionShet = UIAlertController(title: device.name, message: nil, preferredStyle: .actionSheet)
        
        let removeAction = UIAlertAction(title: Strings.SyncRemoveDeviceAction, style: .destructive) { _ in
            if !DeviceInfo.hasConnectivity() {
                self.present(SyncAlerts.noConnection, animated: true)
                return
            }
            
            var alertType = DeviceRemovalType.otherDevice
            
            if deviceCount == 1 {
                alertType = .lastDeviceLeft
            } else if device.isCurrentDevice {
                alertType = .currentDevice
            }
            
            self.presentAlertPopup(for: alertType, device: device)
        }
        
        let cancelAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil)
        
        actionShet.addAction(removeAction)
        actionShet.addAction(cancelAction)
        
        present(actionShet, animated: true)
    }
    
    private func presentAlertPopup(for type: DeviceRemovalType, device: Device) {
        var title: String?
        var message: String?
        var removeButtonName: String?
        let deviceName = device.name ?? Strings.SyncRemoveDeviceDefaultName
        
        switch type {
        case .lastDeviceLeft:
            title = String(format: Strings.SyncRemoveLastDeviceTitle, deviceName)
            message = Strings.SyncRemoveLastDeviceMessage
            removeButtonName = Strings.SyncRemoveLastDeviceRemoveButtonName
        case .currentDevice:
            title = String(format: Strings.SyncRemoveCurrentDeviceTitle, "\(deviceName) (\(Strings.SyncThisDevice))")
            message = Strings.SyncRemoveCurrentDeviceMessage
            removeButtonName = Strings.RemoveDevice
        case .otherDevice:
            title = String(format: Strings.SyncRemoveOtherDeviceTitle, deviceName)
            message = Strings.SyncRemoveOtherDeviceMessage
            removeButtonName = Strings.RemoveDevice
        }
        
        guard let popupTitle = title, let popupMessage = message, let popupButtonName = removeButtonName else { fatalError() }
        
        let popup = AlertPopupView(image: nil, title: popupTitle, message: popupMessage)
        let fontSize: CGFloat = 15
        
        popup.addButton(title: Strings.CancelButtonTitle, fontSize: fontSize) { return .flyDown }
        popup.addButton(title: popupButtonName, type: .destructive, fontSize: fontSize) {
            switch type {
            case .lastDeviceLeft, .currentDevice:
                Sync.shared.leaveSyncGroup()
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
        let deviceCount = frc?.fetchedObjects?.count ?? 0
        let buttonsCount = 1
        
        return section == Sections.buttons.rawValue ? buttonsCount : deviceCount
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        
        guard let frc = frc else {
            log.error("FetchedResultsController is nil.")
            return UITableViewCell()
        }
        
        switch indexPath.section {
        case Sections.deviceList.rawValue:
            let device = frc.object(at: indexPath)
            guard let name = device.name else { break }
            let deviceName = device.isCurrentDevice ? "\(name) (\(Strings.SyncThisDevice))" : name
            
            cell.textLabel?.text = deviceName
        case Sections.buttons.rawValue:
            // By default all cells have separators with left inset. Our buttons are based on table cells,
            // we need to style them so they look more like buttons with full width separator between them.
            setFullWidthSeparator(for: cell)
            configureButtonCell(cell, buttonIndex: indexPath.row)
        default:
            log.error("Section index out of bounds.")
        }
        
        return cell
    }
    
    private func setFullWidthSeparator(for cell: UITableViewCell) {
        cell.preservesSuperviewLayoutMargins = false
        cell.separatorInset = UIEdgeInsets.zero
        cell.layoutMargins = UIEdgeInsets.zero
    }
    
    private func configureButtonCell(_ cell: UITableViewCell, buttonIndex: Int) {
        func attributedString(for text: String, color: UIColor) -> NSAttributedString {
            return NSAttributedString(string: text, attributes:
                [NSAttributedString.Key.foregroundColor: color,
                 NSAttributedString.Key.font: UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.regular)])
        }
        
        let decoratedText = attributedString(for: Strings.SyncAddAnotherDevice, color: BraveUX.BraveOrange)
        
        cell.textLabel?.attributedText = decoratedText
        cell.textLabel?.textAlignment = .center
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        
        switch section {
        case Sections.deviceList.rawValue:
            return Strings.Devices.uppercased()
        default:
            return nil
        }
    }
}

// MARK: - NSFetchedResultsControllerDelegate

extension SyncSettingsTableViewController: NSFetchedResultsControllerDelegate {
    func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        tableView.beginUpdates()
    }
    
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        tableView.endUpdates()
    }
    
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any,
                    at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        
        switch type {
        case .insert:
            guard let newIndexPath = newIndexPath else { return }
            tableView.insertRows(at: [newIndexPath], with: .fade)
        case .delete:
            guard let indexPath = indexPath else { return }
            tableView.deleteRows(at: [indexPath], with: .fade)
        default:
            log.info("Operation type: \(type) is not handled.")
        }
    }
}
