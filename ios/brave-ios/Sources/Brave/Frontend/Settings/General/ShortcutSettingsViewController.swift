// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import IntentsUI
import BraveUI

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
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenNewTabTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .newTab)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenNewTabDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .newPrivateTab)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabDescription))
    )
    
    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenBookmarksTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .openBookmarks)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenBookmarksDescription))
    )
    
    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenHistoryListTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .openHistoryList)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenHistoryListDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsClearBrowserHistoryTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .clearBrowsingHistory)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsClearBrowserHistoryDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsEnableVPNTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .enableBraveVPN)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsEnableVPNDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenBraveNewsTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .openBraveNews)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenBraveNewsDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenPlaylistTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .openPlayList)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenPlaylistDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutSettingsOpenSyncedTabsTitle,
            selection: { [unowned self] in
              manageShortcutActivity(for: .openSyncedTabs)
            }, accessory: .disclosureIndicator, cellClass: MultilineValue1Cell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutSettingsOpenSyncedTabsDescription))
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.Shortcuts.shortcutOpenApplicationSettingsTitle,
            selection: { [unowned self] in
              let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
              let alert = UIAlertController(
                title: Strings.Shortcuts.shortcutOpenApplicationSettingsTitle,
                message: Strings.Shortcuts.shortcutOpenApplicationSettingsDescription,
                preferredStyle: style)

              alert.addAction(
                UIAlertAction(
                  title: Strings.Shortcuts.shortcutOpenApplicationSettingsTitle, style: .default,
                  handler: { _ in
                    if let settingsURL = URL(string: UIApplication.openSettingsURLString) {
                      UIApplication.shared.open(settingsURL)
                    }
                  }))
              alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
              present(alert, animated: true, completion: nil)
            }, cellClass: ButtonCell.self)
        ],
        footer: .title(Strings.Shortcuts.shortcutOpenApplicationSettingsDescription))
    )
  }

  private func manageShortcutActivity(for type: ActivityType) {
    INVoiceShortcutCenter.shared.getAllVoiceShortcuts { [self] (shortcuts, error) in
      DispatchQueue.main.async { [self] in
        guard let shortcut = shortcuts?.first(where: { $0.shortcut.userActivity?.activityType == type.identifier }) else {
          presentAddShortcutActivity(for: type)
          return
        }

        self.presentEditShortcutActivity(for: shortcut)
      }
    }
  }

  private func presentAddShortcutActivity(for type: ActivityType) {
    let userActivity = ActivityShortcutManager.shared.createShortcutActivity(type: type)

    let addShortcutViewController = INUIAddVoiceShortcutViewController(shortcut: INShortcut(userActivity: userActivity))
    addShortcutViewController.delegate = self

    present(addShortcutViewController, animated: true, completion: nil)
  }

  private func presentEditShortcutActivity(for voiceShortcut: INVoiceShortcut) {
    let addShortcutViewController = INUIEditVoiceShortcutViewController(voiceShortcut: voiceShortcut)
    addShortcutViewController.delegate = self

    present(addShortcutViewController, animated: true, completion: nil)
  }
}

extension ShortcutSettingsViewController: INUIAddVoiceShortcutViewControllerDelegate {
  func addVoiceShortcutViewController(
    _ controller: INUIAddVoiceShortcutViewController,
    didFinishWith voiceShortcut: INVoiceShortcut?, error: Error?
  ) {
    controller.dismiss(animated: true, completion: nil)
  }

  func addVoiceShortcutViewControllerDidCancel(_ controller: INUIAddVoiceShortcutViewController) {
    controller.dismiss(animated: true, completion: nil)
  }
}

extension ShortcutSettingsViewController: INUIEditVoiceShortcutViewControllerDelegate {
  func editVoiceShortcutViewController(
    _ controller: INUIEditVoiceShortcutViewController,
    didUpdate voiceShortcut: INVoiceShortcut?, error: Error?
  ) {
    controller.dismiss(animated: true, completion: nil)
  }

  func editVoiceShortcutViewController(
    _ controller: INUIEditVoiceShortcutViewController,
    didDeleteVoiceShortcutWithIdentifier deletedVoiceShortcutIdentifier: UUID
  ) {
    controller.dismiss(animated: true, completion: nil)
  }

  func editVoiceShortcutViewControllerDidCancel(_ controller: INUIEditVoiceShortcutViewController) {
    controller.dismiss(animated: true, completion: nil)
  }
}
