/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import UIKit

extension TabTrayController {
    override var keyCommands: [UIKeyCommand]? {
        let toggleText = privateMode ? Strings.SwitchToNonPBMKeyCodeTitle: Strings.SwitchToPBMKeyCodeTitle
        return [
            UIKeyCommand(input: "`", modifierFlags: .command, action: #selector(didTogglePrivateModeKeyCommand), discoverabilityTitle: toggleText),
            UIKeyCommand(input: "w", modifierFlags: .command, action: #selector(didCloseTabKeyCommand)),
            UIKeyCommand(input: "\u{8}", modifierFlags: [], action: #selector(didCloseTabKeyCommand), discoverabilityTitle: Strings.CloseTabFromTabTrayKeyCodeTitle),
            UIKeyCommand(input: "w", modifierFlags: [.command, .shift], action: #selector(didCloseAllTabsKeyCommand), discoverabilityTitle: Strings.CloseAllTabsFromTabTrayKeyCodeTitle),
            UIKeyCommand(input: "\r", modifierFlags: [], action: #selector(didEnterTabKeyCommand), discoverabilityTitle: Strings.OpenSelectedTabFromTabTrayKeyCodeTitle),
            UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(didEnterTabKeyCommand)),
            UIKeyCommand(input: "\t", modifierFlags: [.command, .alternate], action: #selector(didEnterTabKeyCommand)),
            UIKeyCommand(input: "t", modifierFlags: .command, action: #selector(didOpenNewTabKeyCommand), discoverabilityTitle: Strings.OpenNewTabFromTabTrayKeyCodeTitle),
            UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
            UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
            UIKeyCommand(input: UIKeyCommand.inputDownArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
            UIKeyCommand(input: UIKeyCommand.inputUpArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
        ]
    }

    @objc func didTogglePrivateModeKeyCommand() {
        // NOTE: We cannot and should not capture telemetry here.
        didTogglePrivateMode()
    }

    @objc func didCloseTabKeyCommand() {
        if let tab = tabManager.selectedTab {
            tabManager.removeTab(tab)
        }
    }

    @objc func didCloseAllTabsKeyCommand() {
        closeTabsForCurrentTray()
    }

    @objc func didEnterTabKeyCommand() {
        _ = self.navigationController?.popViewController(animated: true)
    }

    @objc func didOpenNewTabKeyCommand() {
        openNewTab()
    }

    @objc func didChangeSelectedTabKeyCommand(sender: UIKeyCommand) {
        let step: Int
        guard let input = sender.input else { return }
        switch input {
        case UIKeyCommand.inputLeftArrow:
            step = -1
        case UIKeyCommand.inputRightArrow:
            step = 1
        case UIKeyCommand.inputUpArrow:
            step = -numberOfColumns
        case UIKeyCommand.inputDownArrow:
            step = numberOfColumns
        default:
            step = 0
        }

        let tabs = self.tabs
        let currentIndex: Int
        if let selected = tabManager.selectedTab {
            currentIndex = tabs.index(of: selected) ?? 0
        } else {
            currentIndex = 0
        }

        let nextIndex = max(0, min(currentIndex + step, tabs.count - 1))
        let nextTab = tabs[nextIndex]
        tabManager.selectTab(nextTab)
    }
}
