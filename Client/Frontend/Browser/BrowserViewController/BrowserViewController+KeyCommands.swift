/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared

// Naming functions: use the suffix 'KeyCommand' for an additional level of namespacing (bug 1415830)
extension BrowserViewController {

    @objc private func reloadTabKeyCommand() {
        if let tab = tabManager.selectedTab, favoritesController == nil {
            tab.reload()
        }
    }

    @objc private func goBackKeyCommand() {
        if let tab = tabManager.selectedTab, tab.canGoBack, favoritesController == nil {
            tab.goBack()
        }
    }

    @objc private func goForwardKeyCommand() {
        if let tab = tabManager.selectedTab, tab.canGoForward {
            tab.goForward()
        }
    }

    @objc private func findInPageKeyCommand() {
        if let tab = tabManager.selectedTab, favoritesController == nil {
            self.tab(tab, didSelectFindInPageForSelection: "")
        }
    }

    @objc private func selectLocationBarKeyCommand() {
        if favoritesController == nil {
            scrollController.showToolbars(animated: true)
            topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
        }
    }

    @objc private func newTabKeyCommand() {
        openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }

    @objc private func newPrivateTabKeyCommand() {
        // NOTE: We cannot and should not distinguish between "new-tab" and "new-private-tab"
        // when recording telemetry for key commands.
        openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: true)
    }

    @objc private func closeTabKeyCommand() {
        guard let currentTab = tabManager.selectedTab else {
            return
        }
        tabManager.removeTab(currentTab)
    }

    @objc private func nextTabKeyCommand() {
        guard let currentTab = tabManager.selectedTab else {
            return
        }

        let tabs = tabManager.tabsForCurrentMode
        if let index = tabs.firstIndex(of: currentTab), index + 1 < tabs.count {
            tabManager.selectTab(tabs[index + 1])
        } else if let firstTab = tabs.first {
            tabManager.selectTab(firstTab)
        }
    }

    @objc private func previousTabKeyCommand() {
        guard let currentTab = tabManager.selectedTab else {
            return
        }

        let tabs = tabManager.tabsForCurrentMode
        if let index = tabs.firstIndex(of: currentTab), index - 1 < tabs.count && index != 0 {
            tabManager.selectTab(tabs[index - 1])
        } else if let lastTab = tabs.last {
            tabManager.selectTab(lastTab)
        }
    }

    @objc private func showTabTrayKeyCommand() {
        showTabTray()
    }

    @objc private func moveURLCompletionKeyCommand(sender: UIKeyCommand) {
        guard let searchController = self.searchController else {
            return
        }

        searchController.handleKeyCommands(sender: sender)
    }

    override var keyCommands: [UIKeyCommand]? {
        let searchLocationCommands = [
            UIKeyCommand(input: UIKeyCommand.inputDownArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:))),
            UIKeyCommand(input: UIKeyCommand.inputUpArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:))),
        ]
        let overidesTextEditing = [
            UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: .command, action: #selector(goBackKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: .command, action: #selector(goForwardKeyCommand)),
        ]
        let tabNavigation = [
            UIKeyCommand(input: "r", modifierFlags: .command, action: #selector(reloadTabKeyCommand), discoverabilityTitle: Strings.reloadPageTitle),
            UIKeyCommand(input: "[", modifierFlags: .command, action: #selector(goBackKeyCommand), discoverabilityTitle: Strings.backTitle),
            UIKeyCommand(input: "]", modifierFlags: .command, action: #selector(goForwardKeyCommand), discoverabilityTitle: Strings.forwardTitle),

            UIKeyCommand(input: "f", modifierFlags: .command, action: #selector(findInPageKeyCommand), discoverabilityTitle: Strings.findTitle),
            UIKeyCommand(input: "l", modifierFlags: .command, action: #selector(selectLocationBarKeyCommand), discoverabilityTitle: Strings.selectLocationBarTitle),
            UIKeyCommand(input: "t", modifierFlags: .command, action: #selector(newTabKeyCommand), discoverabilityTitle: Strings.newTabTitle),
            UIKeyCommand(input: "p", modifierFlags: [.command, .shift], action: #selector(newPrivateTabKeyCommand), discoverabilityTitle: Strings.newPrivateTabTitle),
            UIKeyCommand(input: "w", modifierFlags: .command, action: #selector(closeTabKeyCommand), discoverabilityTitle: Strings.closeTabTitle),
            UIKeyCommand(input: "\t", modifierFlags: .control, action: #selector(nextTabKeyCommand), discoverabilityTitle: Strings.showNextTabTitle),
            UIKeyCommand(input: "\t", modifierFlags: [.control, .shift], action: #selector(previousTabKeyCommand), discoverabilityTitle: Strings.showPreviousTabTitle),

            // Switch tab to match Safari on iOS.
            UIKeyCommand(input: "]", modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
            UIKeyCommand(input: "[", modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),

            UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(showTabTrayKeyCommand)), // Safari on macOS
            UIKeyCommand(input: "\t", modifierFlags: [.command, .alternate], action: #selector(showTabTrayKeyCommand), discoverabilityTitle: Strings.showTabTrayFromTabKeyCodeTitle)
        ]

        let isEditingText = tabManager.selectedTab?.isEditing ?? false

        if topToolbar.inOverlayMode {
            return tabNavigation + searchLocationCommands
        } else if !isEditingText {
            return tabNavigation + overidesTextEditing
        }
        return tabNavigation
    }
}
