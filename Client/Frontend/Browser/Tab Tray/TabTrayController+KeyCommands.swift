/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import UIKit

extension TabTrayController {
    override var keyCommands: [UIKeyCommand]? {
        let toggleText = privateMode ? Strings.switchToNonPBMKeyCodeTitle: Strings.switchToPBMKeyCodeTitle
        
        let arrowCommands: [UIKeyCommand] =
        [UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
         UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
         UIKeyCommand(input: UIKeyCommand.inputDownArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:))),
         UIKeyCommand(input: UIKeyCommand.inputUpArrow, modifierFlags: [], action: #selector(didChangeSelectedTabKeyCommand(sender:)))]
        
        arrowCommands.forEach {
            if #available(iOS 15.0, *) {
                $0.wantsPriorityOverSystemBehavior = true
            }
        }
        
        return [
            UIKeyCommand(title: toggleText, action: #selector(didTogglePrivateModeKeyCommand), input: "`", modifierFlags: .command),
            UIKeyCommand(input: "w", modifierFlags: .command, action: #selector(didCloseTabKeyCommand)),
            UIKeyCommand(title: Strings.closeTabFromTabTrayKeyCodeTitle, action: #selector(didCloseTabKeyCommand), input: "\u{8}", modifierFlags: []),
            UIKeyCommand(title: Strings.closeAllTabsFromTabTrayKeyCodeTitle, action: #selector(didCloseAllTabsKeyCommand), input: "w", modifierFlags: [.command, .shift]),
            UIKeyCommand(title: Strings.openSelectedTabFromTabTrayKeyCodeTitle, action: #selector(didEnterTabKeyCommand), input: "\r", modifierFlags: []),
            UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(didEnterTabKeyCommand)),
            UIKeyCommand(input: "\t", modifierFlags: [.command, .alternate], action: #selector(didEnterTabKeyCommand)),
            UIKeyCommand(title: Strings.openNewTabFromTabTrayKeyCodeTitle, action: #selector(didOpenNewTabKeyCommand), input: "t", modifierFlags: .command),
        ] + arrowCommands
        
    }

    @objc func didTogglePrivateModeKeyCommand() {
        togglePrivateModeAction()
    }

    @objc func didCloseTabKeyCommand() {
        if let tab = tabManager.selectedTab {
            tabManager.removeTab(tab)
        }
    }

    @objc func didCloseAllTabsKeyCommand() {
        removeAllTabs()
    }

    @objc func didEnterTabKeyCommand() {
        dismiss(animated: true)
    }

    @objc func didOpenNewTabKeyCommand() {
        newTabAction()
    }

    @objc func didChangeSelectedTabKeyCommand(sender: UIKeyCommand) {
        let step: Int
        guard let input = sender.input else { return }
        
        let numberOfColumns = tabTrayView.numberOfColumns
        
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

        guard let selectedTab = tabManager.selectedTab,
              let currentIndex = dataSource.indexPath(for: selectedTab) else { return }

        let tabsCount = tabTrayView.collectionView.numberOfItems(inSection: 0)
        let nextItem = max(0, min(currentIndex.row + step, tabsCount - 1))
        let nextTab = dataSource.itemIdentifier(for: .init(row: nextItem, section: currentIndex.section))
        tabManager.selectTab(nextTab)
        if nextTab != nil {
            // Small hack here to update UI.
            // At the moment `isSelected` flag is not part of Tab's hashable implementation.
            // So to update UI we force reload on the collection view.
            // In all other cases when a tab selection changes, the tab tray is dismissed.
            forceReload()
        }
        
    }
}
