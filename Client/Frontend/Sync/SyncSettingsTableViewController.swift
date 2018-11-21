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
    
    private struct ButtonsSection {
        enum Position: Int, CaseIterable { case addAnotherDevice, removeDevice }
        
        static let count = Position.allCases.count
    }
    
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
        
        // Hiding empty rows from table view.
        tableView.tableFooterView = UIView()
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
        // Only button section is tappable.
        if indexPath.section != Sections.buttons.rawValue { return }
        
        tableView.deselectRow(at: indexPath, animated: true)
        
        switch indexPath.row {
        case ButtonsSection.Position.addAnotherDevice.rawValue:
            addAnotherDeviceAction()
        case ButtonsSection.Position.removeDevice.rawValue:
            removeDeviceAction()
        default:
            log.error("IndexPath row for SyncDeviceList buttons out of range")
        }
    }
    
    override func tableView(_ tableView: UITableView, shouldHighlightRowAt indexPath: IndexPath) -> Bool {
        return indexPath.section != Sections.deviceList.rawValue
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 2
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        let deviceCount = frc?.fetchedObjects?.count ?? 0
        
        return section == Sections.buttons.rawValue ? ButtonsSection.count : deviceCount
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
            cell.textLabel?.text = device.name
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
                [NSAttributedStringKey.foregroundColor: color,
                 NSAttributedStringKey.font: UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.regular)])
        }
        
        var decoratedText: NSAttributedString?
        
        switch buttonIndex {
        case ButtonsSection.Position.addAnotherDevice.rawValue:
            decoratedText = attributedString(for: Strings.SyncAddAnotherDevice, color: BraveUX.Blue)
        case ButtonsSection.Position.removeDevice.rawValue:
            decoratedText = attributedString(for: Strings.SyncRemoveThisDevice, color: BraveUX.Red)
        default:
            log.error("IndexPath row for SyncDeviceList buttons out of range")
        }
        
        cell.textLabel?.attributedText = decoratedText
        cell.textLabel?.textAlignment = .center
    }
    
    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        
        switch section {
        case Sections.deviceList.rawValue:
            return Strings.SyncDeviceSettingsFooter
        default:
            return nil
        }
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        
        switch section {
        case Sections.deviceList.rawValue:
            return Strings.Devices.uppercased()
        default:
            return nil
        }
    }
    
     override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        if indexPath.section != Sections.deviceList.rawValue { return false }
        
        // First cell is our own device, we don't want to allow swipe to delete it.
        return indexPath.row != 0
     }
 
    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
        
        guard let deviceToDelete = frc?.object(at: indexPath), editingStyle == .delete else { return }
        deviceToDelete.remove(save: true)
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
