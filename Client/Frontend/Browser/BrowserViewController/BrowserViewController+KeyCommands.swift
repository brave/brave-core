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
    
    @objc private func showTabKeyCommand(sender: UIKeyCommand) {
        guard let input = sender.input, let index = Int(input) else {
            return
        }

        let tabs = tabManager.tabsForCurrentMode

        if tabs.count > index - 1 {
            tabManager.selectTab(tabs[index - 1])
        }
    }

    @objc private func showTabTrayKeyCommand() {
        showTabTray()
    }
    
    @objc private func closeAllTabsKeyCommand() {
        tabManager.removeAll()
    }
    
    @objc private func addBookmarkCommand() {
        openAddBookmark()
    }
    
    @objc private func addToFavouritesCommand() {
        guard let selectedTab = tabManager.selectedTab,
              let selectedUrl = selectedTab.url,
              !(selectedUrl.isLocal || selectedUrl.isReaderModeURL) else {
            return
        }
        
        FavoritesHelper.add(url: selectedUrl, title: selectedTab.displayTitle)
    }
    
    @objc private func shareWithKeyCommand() {
        navigationHelper.openShareSheet()
    }
    
    @objc private func showHistoryKeyCommand() {
        navigationHelper.openHistory()
    }
    
    @objc private func showBookmarksKeyCommand() {
        navigationHelper.openBookmarks()
    }
    
    @objc private func showDownloadsKeyCommand() {
        navigationHelper.openDownloads() { [weak self] success in
            if !success {
                self?.displayOpenDownloadsError()
            }
        }
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
        let navigationCommands = [
            // Web Page Key Commands
            UIKeyCommand(title: Strings.reloadPageTitle, action: #selector(reloadTabKeyCommand), input: "r", modifierFlags: .command),
            UIKeyCommand(title: Strings.backTitle, action: #selector(goBackKeyCommand), input: "[", modifierFlags: .command),
            UIKeyCommand(title: Strings.forwardTitle, action: #selector(goForwardKeyCommand), input: "]", modifierFlags: .command),
            // URL Bar - Tab Key Commands
            UIKeyCommand(title: Strings.selectLocationBarTitle, action: #selector(selectLocationBarKeyCommand), input: "l", modifierFlags: .command),
            UIKeyCommand(title: Strings.newTabTitle, action: #selector(newTabKeyCommand), input: "t", modifierFlags: .command),
            UIKeyCommand(title: Strings.newPrivateTabTitle, action: #selector(newPrivateTabKeyCommand), input: "p", modifierFlags: [.command, .shift]),
            UIKeyCommand(title: Strings.closeTabTitle, action: #selector(closeTabKeyCommand), input: "w", modifierFlags: .command),
            UIKeyCommand(title: Strings.showNextTabTitle, action: #selector(nextTabKeyCommand), input: "\t", modifierFlags: .control),
            UIKeyCommand(title: Strings.showPreviousTabTitle, action: #selector(previousTabKeyCommand), input: "\t", modifierFlags: [.control, .shift]),
            UIKeyCommand(title: Strings.showTabTrayFromTabKeyCodeTitle, action: #selector(showTabTrayKeyCommand), input: "\t", modifierFlags: [.command, .alternate]),
            UIKeyCommand(title: String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count),
                         action: #selector(showTabTrayKeyCommand), input: "\t", modifierFlags: [.command, .shift]),
            // Page Navigation Key Commands
            UIKeyCommand(title: Strings.showHistoryTitle, action: #selector(showHistoryKeyCommand), input: "y", modifierFlags: [.command]),
            UIKeyCommand(title: Strings.showDownloadsTitle, action: #selector(showDownloadsKeyCommand), input: "j", modifierFlags: .command),
            // Switch tab to match Safari on iOS.
            UIKeyCommand(input: "]", modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
            UIKeyCommand(input: "[", modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),
            UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(showTabTrayKeyCommand)), // Safari on macOS
        ]
        
        // Tab Navigation Key Commands Show tab # 1-9 - Command + 1-9
        var tabNavigationCommands: [UIKeyCommand] = []
        for index in 1..<10 {
            tabNavigationCommands.append(UIKeyCommand(input: String(index), modifierFlags: [.command], action: #selector(showTabKeyCommand(sender:))))
        }
        
        // Bookmarks Key Commands
        let bookmarkEditingCommands = [
            UIKeyCommand(title: Strings.showBookmarksTitle, action: #selector(showBookmarksKeyCommand), input: "o", modifierFlags: [.shift, .command]),
            UIKeyCommand(title: Strings.addBookmarkTitle, action: #selector(addBookmarkCommand), input: "d", modifierFlags: [.command]),
            UIKeyCommand(title: Strings.addFavouritesTitle, action: #selector(addToFavouritesCommand), input: "d", modifierFlags: [.command, .shift])
        ]
        
        // Find in Page Key Commands
        var findTextCommands = [
            UIKeyCommand(title: Strings.findInPageTitle, action: #selector(findInPageKeyCommand), input: "f", modifierFlags: .command),
        ]
        
        let findTextUtilitiesCommands = [
            UIKeyCommand(title: Strings.findNextTitle, action: #selector(findNextCommand), input: "g", modifierFlags: [.command]),
            UIKeyCommand(title: Strings.findPreviousTitle, action: #selector(findPreviousCommand), input: "g", modifierFlags: [.command, .shift])
        ]
        
        let isFindingText = !(findInPageBar?.text?.isEmpty ?? true)
        
        if isFindingText {
            findTextCommands.append(contentsOf: findTextUtilitiesCommands)
        }
        
        // Share With Key Command
        let shareCommands = [
            UIKeyCommand(title: Strings.shareWithTitle, action: #selector(shareWithKeyCommand), input: "s", modifierFlags: .command)
        ]
        
        var keyCommandList = navigationCommands + tabNavigationCommands + bookmarkEditingCommands + shareCommands + findTextCommands
        
        // URL completion and Override Key commands
        let searchLocationCommands = [
            UIKeyCommand(input: UIKeyCommand.inputDownArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:))),
            UIKeyCommand(input: UIKeyCommand.inputUpArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:))),
        ]
        
        let overridesTextEditingCommands = [
            UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputLeftArrow, modifierFlags: .command, action: #selector(goBackKeyCommand)),
            UIKeyCommand(input: UIKeyCommand.inputRightArrow, modifierFlags: .command, action: #selector(goForwardKeyCommand)),
        ]
        
        // In iOS 15+, certain keys events are delivered to the text input or focus systems first, unless specified otherwise
        if #available(iOS 15, *) {
            searchLocationCommands.forEach { $0.wantsPriorityOverSystemBehavior = true }
            overridesTextEditingCommands.forEach { $0.wantsPriorityOverSystemBehavior = true }
        }
        
        let isEditingText = tabManager.selectedTab?.isEditing ?? false

        if topToolbar.inOverlayMode {
            keyCommandList.append(contentsOf: searchLocationCommands)
        } else if !isEditingText {
            keyCommandList.append(contentsOf: overridesTextEditingCommands)
        }
        
        return keyCommandList
    }
}
