// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import IntentsUI

// MARK: - ShortcutSettingsViewController

class ShortcutSettingsViewController: TableViewController {
        
    // MARK: Lifecycle
    
    init() {
        super.init(style: .insetGrouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.Shortcuts.shortcutSettingsTitle
        
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsOpenNewTabTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .newTab)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shortcutSettingsOpenNewTabDescription))
        )
        
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .newPrivateTab)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabDescription))
        )
        
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsClearBrowserHistoryTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .clearBrowsingHistory)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shortcutSettingsClearBrowserHistoryDescription))
        )
            
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsEnableVPNTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .enableBraveVPN)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shortcutSettingsEnableVPNDescription))
        )
        
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsOpenBraveNewsTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .openBraveNews)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shortcutSettingsOpenBraveNewsDescription))
        )
        
        dataSource.sections.append(
            Section(rows: [
                        Row(text: Strings.Shortcuts.shortcutSettingsOpenPlaylistTitle, selection: { [unowned self] in
                            manageShortcutActivity(for: .openPlayList)
                        }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)],
                    footer: .title(Strings.Shortcuts.shorcutSettingsOpenPlaylistDescription))
        )
    }
    
    private func manageShortcutActivity(for type: ActivityType) {
        INVoiceShortcutCenter.shared.getAllVoiceShortcuts { [unowned self] (shortcuts, error) in
            DispatchQueue.main.async {
                guard let shortcuts = shortcuts else { return }
                
                guard let shortcut = shortcuts.first(where: { $0.shortcut.userActivity?.activityType == type.identifier }) else {
                    presentAddShorcutActivity(for: type)
                    return
                }
                
                self.presentEditShorcutActivity(for: shortcut)
            }
        }
    }
    
    private func presentAddShorcutActivity(for type: ActivityType) {
        let userActivity = ActivityShortcutManager.shared.createShortcutActivity(type: type)
                        
        let addShorcutViewController = INUIAddVoiceShortcutViewController(shortcut: INShortcut(userActivity: userActivity))
        addShorcutViewController.delegate = self

        present(addShorcutViewController, animated: true, completion: nil)
    }
    
    private func presentEditShorcutActivity(for voiceShortcut: INVoiceShortcut) {
        let addShorcutViewController = INUIEditVoiceShortcutViewController(voiceShortcut: voiceShortcut)
        addShorcutViewController.delegate = self

        present(addShorcutViewController, animated: true, completion: nil)
    }
}

extension ShortcutSettingsViewController: INUIAddVoiceShortcutViewControllerDelegate {
    func addVoiceShortcutViewController(_ controller: INUIAddVoiceShortcutViewController,
                                        didFinishWith voiceShortcut: INVoiceShortcut?, error: Error?) {
        controller.dismiss(animated: true, completion: nil)
    }

    func addVoiceShortcutViewControllerDidCancel(_ controller: INUIAddVoiceShortcutViewController) {
        controller.dismiss(animated: true, completion: nil)
    }
}

extension ShortcutSettingsViewController: INUIEditVoiceShortcutViewControllerDelegate {
    func editVoiceShortcutViewController(_ controller: INUIEditVoiceShortcutViewController,
                                         didUpdate voiceShortcut: INVoiceShortcut?, error: Error?) {
        controller.dismiss(animated: true, completion: nil)
    }

    func editVoiceShortcutViewController(_ controller: INUIEditVoiceShortcutViewController,
                                         didDeleteVoiceShortcutWithIdentifier deletedVoiceShortcutIdentifier: UUID) {
        controller.dismiss(animated: true, completion: nil)
    }

    func editVoiceShortcutViewControllerDidCancel(_ controller: INUIEditVoiceShortcutViewController) {
        controller.dismiss(animated: true, completion: nil)
    }
}
