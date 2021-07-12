/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared

// Naming functions: use the suffix 'KeyCommand' for an additional level of namespacing (bug 1415830)
extension BrowserViewController {
    
    enum TextSearchDirection: String {
        case next = "findNext"
        case previous = "findPrevious"
    }

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
    
    @objc private func addBookmarkCommand() {
        openAddBookmark()
    }
    
    @objc private func findNextCommand() {
        findTextInPage(.next)
    }
    
    @objc private func findPreviousCommand() {
        findTextInPage(.previous)
    }

    @objc private func moveURLCompletionKeyCommand(sender: UIKeyCommand) {
        guard let searchController = self.searchController else {
            return
        }

        searchController.handleKeyCommands(sender: sender)
    }

    override var keyCommands: [UIKeyCommand]? {
        let searchLocation = [
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
            UIKeyCommand(title: Strings.reloadPageTitle, action: #selector(reloadTabKeyCommand), input: "r", modifierFlags: .command),
            UIKeyCommand(title: Strings.backTitle, action: #selector(goBackKeyCommand), input: "[", modifierFlags: .command),
            UIKeyCommand(title: Strings.forwardTitle, action: #selector(goForwardKeyCommand), input: "]", modifierFlags: .command),

            UIKeyCommand(title: Strings.findTitle, action: #selector(findInPageKeyCommand), input: "f", modifierFlags: .command),
            UIKeyCommand(title: Strings.selectLocationBarTitle, action: #selector(selectLocationBarKeyCommand), input: "l", modifierFlags: .command),
            UIKeyCommand(title: Strings.newTabTitle, action: #selector(newTabKeyCommand), input: "t", modifierFlags: .command),
            UIKeyCommand(title: Strings.newPrivateTabTitle, action: #selector(newPrivateTabKeyCommand), input: "p", modifierFlags: [.command, .shift]),
            UIKeyCommand(title: Strings.closeTabTitle, action: #selector(closeTabKeyCommand), input: "w", modifierFlags: .command),
            UIKeyCommand(title: Strings.showNextTabTitle, action: #selector(nextTabKeyCommand), input: "\t", modifierFlags: .control),
            UIKeyCommand(title: Strings.showPreviousTabTitle, action: #selector(previousTabKeyCommand), input: "\t", modifierFlags: [.control, .shift]),

            // Switch tab to match Safari on iOS.
            UIKeyCommand(input: "]", modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
            UIKeyCommand(input: "[", modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),

            UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(showTabTrayKeyCommand)), // Safari on macOS
            UIKeyCommand(title: Strings.showTabTrayFromTabKeyCodeTitle, action: #selector(showTabTrayKeyCommand), input: "\t", modifierFlags: [.command, .alternate])
        ]
        
        let bookmarkEditing = [
            UIKeyCommand(input: "d", modifierFlags: [.command], action: #selector(addBookmarkCommand))
        ]
        
        let findText = [
            UIKeyCommand(input: "g", modifierFlags: [.command], action: #selector(findNextCommand)),
            UIKeyCommand(input: "g", modifierFlags: [.command, .shift], action: #selector(findPreviousCommand))
        ]

        let isEditingText = tabManager.selectedTab?.isEditing ?? false

        var keycommandList = tabNavigation + bookmarkEditing + findText
        
        if topToolbar.inOverlayMode {
            keycommandList.append(contentsOf: searchLocation)
        } else if !isEditingText {
            keycommandList.append(contentsOf: overidesTextEditing)
        }
        
        return keycommandList
    }
}
