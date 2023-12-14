/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import Foundation
import UIKit
import Data
import Preferences

extension BrowserViewController {
  
  // MARK: Actions
  
  @objc private func reloadTabKeyCommand() {
    if let tab = tabManager.selectedTab, favoritesController == nil {
      tab.reload()
    }
  }

  @objc private func goBackKeyCommand() {
    if let tab = tabManager.selectedTab, tab.canGoBack, favoritesController == nil {
      tab.goBack()
      resetExternalAlertProperties(tab)
    }
  }

  @objc private func goForwardKeyCommand() {
    if let tab = tabManager.selectedTab, tab.canGoForward {
      tab.goForward()
      resetExternalAlertProperties(tab)
    }
  }

  @objc private func findInPageKeyCommand() {
    if let tab = tabManager.selectedTab, favoritesController == nil {
      self.tab(tab, didSelectFindInPageFor: "")
    }
  }

  @objc private func selectLocationBarKeyCommand() {
    if favoritesController == nil {
      toolbarVisibilityViewModel.toolbarState = .expanded
      topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
    }
  }

  @objc private func newTabKeyCommand() {
    openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: privateBrowsingManager.isPrivateBrowsing)
  }

  @objc private func newPrivateTabKeyCommand() {
    // NOTE: We cannot and should not distinguish between "new-tab" and "new-private-tab"
    // when recording telemetry for key commands.
    if !privateBrowsingManager.isPrivateBrowsing, Preferences.Privacy.privateBrowsingLock.value {
      self.askForLocalAuthentication { [weak self] success, error in
        if success {
          self?.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)

        }
      }
    } else {
      openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
    }
  }

  @objc private func closeTabKeyCommand() {
    guard let currentTab = tabManager.selectedTab else {
      return
    }
    
    if topToolbar.locationView.readerModeState == .active {
      hideReaderModeBar(animated: false)
    }
    
    // Initially add the tab to recently closed and remove it from Tab Data after
    tabManager.addTabToRecentlyClosed(currentTab)
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
    tabManager.removeAllForCurrentMode()
  }

  @objc private func addBookmarkCommand() {
    openAddBookmark()
  }

  @objc private func addToFavouritesCommand() {
    guard let selectedTab = tabManager.selectedTab,
      let selectedUrl = selectedTab.url,
      !(selectedUrl.isLocal || selectedUrl.isReaderModeURL)
    else {
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

  @objc private func showShieldsKeyCommand() {
    presentBraveShieldsViewController()
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
  
#if canImport(BraveTalk)
  @objc private func toggleBraveTalkMuteCommand() {
    braveTalkJitsiCoordinator.toggleMute()
  }
#endif

  @objc private func reopenRecentlyClosedTabCommand() {
    guard let recentlyClosed = RecentlyClosed.all().first else {
      return
    }
    
    tabManager.addAndSelectRecentlyClosed(recentlyClosed)
    RecentlyClosed.remove(with: recentlyClosed.url)
  }
  
  @objc private func zoomInPageKeyCommand() {
    changeZoomLevel(.increment)
  }
  
  @objc private func zoomOutPageKeyCommand() {
    changeZoomLevel(.decrement)
  }
  
  private func changeZoomLevel(_ status: PageZoomHandler.ChangeStatus) {
    guard let selectTab = tabManager.selectedTab else { return }
    let zoomHandler = PageZoomHandler(
      tab: selectTab, isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing)
    
    zoomHandler.changeZoomLevel(status)
  }
  
  // MARK: KeyCommands
  
  override public var keyCommands: [UIKeyCommand]? {
    let isEditingText = tabManager.selectedTab?.isEditing ?? false
    
    var navigationCommands = [
      // Web Page Key Commands
      UIKeyCommand(title: Strings.Hotkey.reloadPageTitle, action: #selector(reloadTabKeyCommand), input: "r", modifierFlags: .command),
    ]
    
    let tabMovementCommands = [
      UIKeyCommand(title: Strings.Hotkey.backTitle, action: #selector(goBackKeyCommand), input: UIKeyCommand.inputLeftArrow, modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.forwardTitle, action: #selector(goForwardKeyCommand), input: UIKeyCommand.inputRightArrow, modifierFlags: .command)
    ]
    
    if !isEditingText {
      navigationCommands += tabMovementCommands
    }
    
    navigationCommands += [
      UIKeyCommand(input: "[", modifierFlags: .command, action: #selector(goBackKeyCommand)),
      UIKeyCommand(input: "]", modifierFlags: .command, action: #selector(goForwardKeyCommand)),
    ]
    
    navigationCommands += [
      UIKeyCommand(title: Strings.Hotkey.zoomInTitle, action: #selector(zoomInPageKeyCommand), input: "+", modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.zoomOutTitle, action: #selector(zoomOutPageKeyCommand), input: "-", modifierFlags: .command)
    ]
    
    // URL Bar - Tab Key Commands
    navigationCommands += [
      UIKeyCommand(title: Strings.Hotkey.selectLocationBarTitle, action: #selector(selectLocationBarKeyCommand), input: "l", modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.newTabTitle, action: #selector(newTabKeyCommand), input: "t", modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.newPrivateTabTitle, action: #selector(newPrivateTabKeyCommand), input: "n", modifierFlags: [.command, .shift]),
    ]
    
    if !privateBrowsingManager.isPrivateBrowsing {
      navigationCommands += [
        UIKeyCommand(title: Strings.Hotkey.recentlyClosedTabTitle, action: #selector(reopenRecentlyClosedTabCommand), input: "t", modifierFlags: [.command, .shift]),
        UIKeyCommand(action: #selector(reopenRecentlyClosedTabCommand), input: "t", modifierFlags: [.control, .shift])
      ]
    }
    
    navigationCommands += [
      UIKeyCommand(title: Strings.Hotkey.closeTabTitle, action: #selector(closeTabKeyCommand), input: "w", modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.closeAllTabsFromTabTrayKeyCodeTitle, action: #selector(closeAllTabsKeyCommand), input: "w", modifierFlags: [.command, .alternate])
    ]
    
    let tabNavigationKeyCommands = [
      // Tab Navigation Key Commands
      UIKeyCommand(title: Strings.Hotkey.showNextTabTitle, action: #selector(nextTabKeyCommand), input: UIKeyCommand.inputRightArrow, modifierFlags: [.command, .alternate]),
      UIKeyCommand(title: Strings.Hotkey.showPreviousTabTitle, action: #selector(previousTabKeyCommand), input: UIKeyCommand.inputLeftArrow, modifierFlags: [.command, .alternate])
    ]
    
    if !isEditingText {
      navigationCommands += tabNavigationKeyCommands
    }
  
    navigationCommands += [
      UIKeyCommand(input: "}", modifierFlags: [.command], action: #selector(nextTabKeyCommand)),
      UIKeyCommand(input: "{", modifierFlags: [.command], action: #selector(previousTabKeyCommand)),
      UIKeyCommand(title: Strings.Hotkey.showTabTrayFromTabKeyCodeTitle, action: #selector(showTabTrayKeyCommand), input: "\t", modifierFlags: [.command, .alternate]),
      
      // Page Navigation Key Commands
      UIKeyCommand(title: Strings.Hotkey.showHistoryTitle, action: #selector(showHistoryKeyCommand), input: "y", modifierFlags: [.command]),
      UIKeyCommand(title: Strings.Hotkey.showDownloadsTitle, action: #selector(showDownloadsKeyCommand), input: "j", modifierFlags: .command),
      UIKeyCommand(title: Strings.Hotkey.showShieldsTitle, action: #selector(showShieldsKeyCommand), input: ",", modifierFlags: .command),

      // Switch tab to match Safari on iOS.
      UIKeyCommand(input: "]", modifierFlags: [.command, .shift], action: #selector(nextTabKeyCommand)),
      UIKeyCommand(input: "[", modifierFlags: [.command, .shift], action: #selector(previousTabKeyCommand)),
      UIKeyCommand(input: "\\", modifierFlags: [.command, .shift], action: #selector(showTabTrayKeyCommand)),  // Safari on macOS
    ]

    // Tab Navigation Key Commands Show tab # 1-9 - Command + 1-9
    var tabNavigationCommands: [UIKeyCommand] = []
    for index in 1..<10 {
      tabNavigationCommands.append(UIKeyCommand(input: String(index), modifierFlags: [.command], action: #selector(showTabKeyCommand(sender:))))
    }

    // Bookmarks Key Commands
    let bookmarkEditingCommands = [
      UIKeyCommand(title: Strings.Hotkey.showBookmarksTitle, action: #selector(showBookmarksKeyCommand), input: "o", modifierFlags: [.shift, .command]),
      UIKeyCommand(title: Strings.Hotkey.addBookmarkTitle, action: #selector(addBookmarkCommand), input: "d", modifierFlags: [.command]),
      UIKeyCommand(title: Strings.Hotkey.addFavouritesTitle, action: #selector(addToFavouritesCommand), input: "d", modifierFlags: [.command, .shift]),
    ]

    // Find in Page Key Commands
    var findTextCommands = [
      UIKeyCommand(title: Strings.Hotkey.findInPageTitle, action: #selector(findInPageKeyCommand), input: "f", modifierFlags: .command)
    ]

    // These are automatically handled in iOS 16's UIFindInteraction
    if #unavailable(iOS 16.0) {
      let findTextUtilitiesCommands = [
        UIKeyCommand(title: Strings.Hotkey.findNextTitle, action: #selector(findNextCommand), input: "g", modifierFlags: [.command]),
        UIKeyCommand(title: Strings.Hotkey.findPreviousTitle, action: #selector(findPreviousCommand), input: "g", modifierFlags: [.command, .shift]),
      ]
      
      let isFindingText = !(findInPageBar?.text?.isEmpty ?? true)
      
      if isFindingText {
        findTextCommands.append(contentsOf: findTextUtilitiesCommands)
      }
    }

    // Share With Key Command
    let shareCommands = [
      UIKeyCommand(title: Strings.Hotkey.shareWithTitle, action: #selector(shareWithKeyCommand), input: "s", modifierFlags: .command)
    ]

    // Additional Commands which will have priority over system
    let additionalPriorityCommandKeys = [
      UIKeyCommand(input: "\t", modifierFlags: .control, action: #selector(nextTabKeyCommand)),
      UIKeyCommand(input: "\t", modifierFlags: [.control, .shift], action: #selector(previousTabKeyCommand))
    ]
    
    var keyCommandList = navigationCommands + tabNavigationCommands + bookmarkEditingCommands + shareCommands + findTextCommands + additionalPriorityCommandKeys

    // URL completion and Override Key commands
    let searchLocationCommands = [
      UIKeyCommand(input: UIKeyCommand.inputDownArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:))),
      UIKeyCommand(input: UIKeyCommand.inputUpArrow, modifierFlags: [], action: #selector(moveURLCompletionKeyCommand(sender:)))
    ]
    
#if canImport(BraveTalk)
    let braveTalkKeyCommands: [UIKeyCommand] = [
      UIKeyCommand(input: "m", modifierFlags: [], action: #selector(toggleBraveTalkMuteCommand))
    ]
#endif

    // In iOS 15+, certain keys events are delivered to the text input or focus systems first, unless specified otherwise
    searchLocationCommands.forEach { $0.wantsPriorityOverSystemBehavior = true }
    tabMovementCommands.forEach { $0.wantsPriorityOverSystemBehavior = true }
    tabNavigationKeyCommands.forEach { $0.wantsPriorityOverSystemBehavior = true }
    additionalPriorityCommandKeys.forEach { $0.wantsPriorityOverSystemBehavior = true }

    if topToolbar.inOverlayMode {
      keyCommandList.append(contentsOf: searchLocationCommands)
    }
    
#if canImport(BraveTalk)
    if braveTalkJitsiCoordinator.isCallActive {
      keyCommandList.append(contentsOf: braveTalkKeyCommands)
    }
#endif

    return keyCommandList
  }
  
#if canImport(BraveTalk)
  public override func pressesBegan(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
    super.pressesBegan(presses, with: event)
    braveTalkJitsiCoordinator.handleResponderPresses(presses: presses, phase: .began)
  }
  
  public override func pressesChanged(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
    super.pressesChanged(presses, with: event)
    braveTalkJitsiCoordinator.handleResponderPresses(presses: presses, phase: .changed)
  }
  
  public override func pressesEnded(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
    super.pressesEnded(presses, with: event)
    braveTalkJitsiCoordinator.handleResponderPresses(presses: presses, phase: .ended)
  }
  
  public override func pressesCancelled(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
    super.pressesCancelled(presses, with: event)
    braveTalkJitsiCoordinator.handleResponderPresses(presses: presses, phase: .cancelled)
  }
#endif
}
