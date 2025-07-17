// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Data
import LocalAuthentication
import Preferences
import SwiftUI
import Web

@MainActor
@Observable
class TabGridViewModel {
  private var _isPrivateBrowsing: Bool = false
  private(set) var tabs: [any TabState] = []
  private(set) var isPrivateTabsLocked: Bool = false

  var isPrivateBrowsing: Bool {
    get {
      _isPrivateBrowsing
    }
    set {
      if _isPrivateBrowsing == newValue { return }
      let oldValue = _isPrivateBrowsing
      browsingModeWillSwitch(to: newValue)
      _isPrivateBrowsing = newValue
      browsingModeDidSwitch(from: oldValue)
    }
  }

  var isSearching: Bool = false {
    didSet {
      if !isSearching {
        searchQuery = ""
      }
    }
  }

  var searchQuery: String = "" {
    didSet {
      withAnimation {
        updateTabs()
      }
    }
  }

  @ObservationIgnored
  private var cancellables: Set<AnyCancellable> = []
  private let tabManager: TabManager
  @ObservationIgnored
  private var tabManagerObserver: TabManagerObserver?
  @ObservationIgnored
  private var isPrivateTabsUnlockedThisSession: Bool = false

  // Dependencies
  @ObservationIgnored
  private(set) weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  let historyModel: HistoryModel?
  let openTabsModel: OpenTabsModel?
  let profileController: BraveProfileController?
  let windowProtection: WindowProtection?

  init(
    tabManager: TabManager,
    historyModel: HistoryModel?,
    openTabsModel: OpenTabsModel?,
    toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?,
    profileController: BraveProfileController?,
    windowProtection: WindowProtection?
  ) {
    self.tabManager = tabManager
    self.historyModel = historyModel
    self.openTabsModel = openTabsModel
    self.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
    self.profileController = profileController
    self.windowProtection = windowProtection
    self.tabs = tabManager.tabsForCurrentMode
    self._isPrivateBrowsing = tabManager.privateBrowsingManager.isPrivateBrowsing

    setUpObservations()
  }

  deinit {
    if let observer = tabManagerObserver {
      tabManager.removeDelegate(observer)
    }
  }

  func addTab() {
    withAnimation {
      _ = tabManager.addTabAndSelect(isPrivate: isPrivateBrowsing)
    }
  }

  var selectedTab: (any TabState)? {
    tabManager.selectedTab
  }

  var hasRecentlyClosedTabs: Bool {
    !isPrivateBrowsing && RecentlyClosed.first() != nil
  }

  func addAndSelectRecentlyClosedTab() {
    guard let recentlyClosed = RecentlyClosed.first() else { return }
    withAnimation {
      tabManager.addAndSelectRecentlyClosed(recentlyClosed)
      RecentlyClosed.remove(with: recentlyClosed.url)
    }
  }

  func closeSelectedTab() {
    guard let tab = tabManager.selectedTab else { return }
    withAnimation {
      tabManager.removeTab(tab)
    }
  }

  func closeTab(_ tab: any TabState) {
    withAnimation {
      tabManager.removeTab(tab)
    }
  }

  func closeAllTabs() {
    withAnimation {
      tabManager.removeAllTabsForPrivateMode(isPrivate: isPrivateBrowsing)
    }
  }

  func shredSelectedTab() {
    guard let tab = tabManager.selectedTab, let url = tab.visibleURL else { return }
    tabManager.shredData(for: url, in: tab)
  }

  func selectTab(_ tab: any TabState) {
    withAnimation {
      tabManager.selectTab(tab)
    }
  }

  func moveTab(_ tab: any TabState, to index: Int) {
    tabManager.moveTab(tab, toIndex: index)
    updateTabs()
  }

  func presentPrivateModeUnlock() {
    windowProtection?.isCancellable = true
    windowProtection?.presentAuthenticationForViewController(
      determineLockWithPasscode: false,
      viewType: .tabTray
    ) {
      [weak self] success, error in
      guard let self else { return }
      handleAuthenticationResult(success: success, error: error)
    }
  }

  // MARK -

  private func updateTabs() {
    let query = isSearching && !searchQuery.isEmpty ? searchQuery : nil
    tabs = tabManager.tabsForCurrentMode(for: query)
  }

  private func handleAuthenticationResult(success: Bool, error: LAError.Code?) {
    if success || (!success && error == .passcodeNotSet) {
      isPrivateTabsLocked = false
      isPrivateTabsUnlockedThisSession = true
      withAnimation {
        if isPrivateBrowsing {
          // Continue the switch to update the view model
          browsingModeDidSwitch(from: false)
        } else {
          // Handle a flow where the user cancels initial auth which would revert the user back
          // to normal mode, but then eventually auths successfully
          isPrivateBrowsing = true
        }
      }
    } else {
      isPrivateBrowsing = false
    }
  }

  private func browsingModeWillSwitch(to newValue: Bool) {
    // Record the selected index before mode navigation
    if !newValue {
      tabManager.privateTabSelectedIndex =
        Preferences.Privacy.persistentPrivateBrowsing.value ? tabManager.selectedIndex : 0
    } else {
      tabManager.normalTabSelectedIndex = tabManager.selectedIndex
    }
    tabManager.willSwitchTabMode(leavingPBM: !newValue)

    isPrivateTabsLocked =
      !isPrivateTabsUnlockedThisSession && Preferences.Privacy.privateBrowsingLock.value && newValue
    if isPrivateTabsLocked {
      presentPrivateModeUnlock()
    }
  }

  private func browsingModeDidSwitch(from oldValue: Bool) {
    tabManager.privateBrowsingManager.isPrivateBrowsing = isPrivateBrowsing
    if isPrivateTabsLocked {
      tabs = []
      return
    }

    updateTabs()

    let indexToSelect: Int? =
      isPrivateBrowsing ? tabManager.privateTabSelectedIndex : tabManager.normalTabSelectedIndex
    let tabToSelect = indexToSelect.flatMap { tabManager.allTabs[safe: $0] }
    if isPrivateBrowsing {
      if Preferences.Privacy.persistentPrivateBrowsing.value, let tabToSelect {
        tabManager.selectTab(tabToSelect)
      }
    } else {
      if tabManager.tabsForCurrentMode.isEmpty {
        tabManager.addTabAndSelect(isPrivate: false)
      } else {
        tabManager.selectTab(tabToSelect)
      }
    }

    // If by the time we switch to a new mode there is still no selected tab, simply select the
    // last tab in the list for that mode
    if tabs.first(where: \.isVisible) == nil, let tab = tabs.last {
      tabManager.selectTab(tab)
    }
  }

  private func setUpObservations() {
    tabManager.privateBrowsingManager.$isPrivateBrowsing
      .dropFirst()
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self] value in
        guard let self else { return }
        _isPrivateBrowsing = value
      }.store(in: &cancellables)

    let observer = TabManagerObserver { [weak self] in
      guard let self else { return }
      self.updateTabs()
    }
    tabManagerObserver = observer

    tabManager.addDelegate(observer)

    windowProtection?
      .cancelPressed
      .sink { [weak self] in
        self?.handleAuthenticationResult(success: false, error: nil)
      }
      .store(in: &cancellables)

    windowProtection?
      .finalizedAuthentication
      .sink { [weak self] success, type in
        guard type == .tabTray else { return }
        self?.handleAuthenticationResult(success: success, error: nil)
      }
      .store(in: &cancellables)
  }

  private class TabManagerObserver: NSObject, TabManagerDelegate {
    var notify: () -> Void

    init(notify: @escaping () -> Void) {
      self.notify = notify
    }

    func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState) {
      notify()
    }

    func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState) {
      notify()
    }

    func tabManager(
      _ tabManager: TabManager,
      didSelectedTabChange selected: (any Web.TabState)?,
      previous: (any Web.TabState)?
    ) {
      notify()
    }

    func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
      notify()
    }
  }
}
