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
  private var _isTorBrowsing = false
  var availableModes: [TabBrowsingMode] {
    var modes: [TabBrowsingMode] = Preferences.Privacy.privateBrowsingOnly.value ? [.private] : [.regular, .private]
    if Preferences.Privacy.torEnabled.value { modes.append(.tor) }
    return modes
  }

  var mode: TabBrowsingMode {
    get { _isTorBrowsing ? .tor : (isPrivateBrowsing ? .private : .regular) }
    set {
      guard mode != newValue, availableModes.contains(newValue) else { return }
      let oldPrivateIndex = tabManager.privateTabSelectedIndex
      if _isTorBrowsing { tabManager.torTabSelectedIndex = tabManager.selectedIndex }
      else if isPrivateBrowsing { tabManager.privateTabSelectedIndex = tabManager.selectedIndex }
      let wasPrivate = isPrivateBrowsing
      let willBePrivate = newValue != .regular
      let willBeTor = newValue == .tor
      _isTorBrowsing = willBeTor
      tabManager.privateBrowsingManager.isTorBrowsing = _isTorBrowsing
      isPrivateBrowsing = willBePrivate
      if !willBePrivate { tabManager.privateTabSelectedIndex = oldPrivateIndex }
      tabManager.privateBrowsingManager.isTorBrowsing = _isTorBrowsing
      if wasPrivate == willBePrivate && !isPrivateTabsLocked {
        updateTabs()
        let index = _isTorBrowsing ? tabManager.torTabSelectedIndex : tabManager.privateTabSelectedIndex
        let tab = index.flatMap { tabManager.allTabs[safe: $0] }
        tabManager.selectTab(tab.flatMap { $0.isTor == willBeTor ? $0 : nil } ?? tabs.last)
        // Selecting nil resets the global mode; preserve the empty selected group.
        tabManager.privateBrowsingManager.isPrivateBrowsing = willBePrivate
        tabManager.privateBrowsingManager.isTorBrowsing = willBeTor
      }
      _isPrivateBrowsing = willBePrivate
      _isTorBrowsing = willBeTor
      updateTabs()
    }
  }
  private(set) var tabs: [any TabState] = []
  private(set) var isPrivateTabsLocked: Bool = false
  var onModeSwitchComplete: (() -> Void)?
  private(set) var isSceneActive: Bool = true

  var isPrivateBrowsing: Bool {
    get {
      _isPrivateBrowsing
    }
    set {
      if _isPrivateBrowsing == newValue { return }
      let oldValue = _isPrivateBrowsing
      if !newValue { _isTorBrowsing = false }
      browsingModeWillSwitch(to: newValue)
      _isPrivateBrowsing = newValue
      browsingModeDidSwitch(from: oldValue)
    }
  }

  var isSearching: Bool = false {
    didSet {
      if !isSearching {
        searchQuery = ""
        updateTabs()
      }
    }
  }

  var searchQuery: String = "" {
    didSet {
      // Avoid updating tabs if the search query hasn't changed
      if oldValue == searchQuery { return }
      if isSearching {
        withAnimation {
          updateTabs()
        }
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
    self._isTorBrowsing = tabManager.privateBrowsingManager.isTorBrowsing

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

  var regularTabCount: Int {
    tabManager.tabsCountForMode(isPrivate: false)
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
    closeTab(tab)
  }

  func closeTab(_ tab: any TabState) {
    withAnimation {
      tabManager.addTabToRecentlyClosed(tab)
      tabManager.removeTab(tab)
    }
  }

  func closeTabs(_ tabIDs: Set<TabState.ID>) {
    let tabs = tabIDs.compactMap { tabManager[$0] }
    withAnimation {
      tabManager.addTabsToRecentlyClosed(tabs)
      tabManager.removeTabs(tabs)
    }
  }

  func closeOtherTabs(_ tabIDs: Set<TabState.ID>) {
    let tabs = tabManager.tabsForCurrentMode.filter({ !tabIDs.contains($0.id) })
    withAnimation {
      tabManager.addTabsToRecentlyClosed(tabs)
      tabManager.removeTabs(tabs)
    }
  }

  func closeAllTabs() {
    withAnimation {
      if !isPrivateBrowsing {
        tabManager.addAllTabsToRecentlyClosed(isActiveTabIncluded: true)
      }
      tabManager.removeAllTabsForPrivateMode(isPrivate: isPrivateBrowsing)
    }
  }

  var isShredMenuVisible: Bool {
    if mode == .tor { return tabs.contains { $0.visibleURL?.isShredAvailable == true } }
    return isSelectedTabShredAvailable
      || tabManager.tabsForCurrentMode.contains(where: { $0.visibleURL?.isShredAvailable == true })
  }

  var isSelectedTabShredAvailable: Bool {
    guard let selectedTab = tabManager.selectedTab,
      selectedTab.isPrivate == isPrivateBrowsing
    else { return false }
    if mode == .tor { return selectedTab.isTor && selectedTab.visibleURL?.isShredAvailable == true }
    guard !selectedTab.isTor else { return false }
    return selectedTab.visibleURL?.isShredAvailable == true
  }

  func isShredAvailableForSelectedTabs(_ tabs: Set<TabState.ID>) -> Bool {
    if mode == .tor {
      return self.tabs.contains { tabs.contains($0.id) && $0.visibleURL?.isShredAvailable == true }
    }
    let tabs = tabs.map({ tabManager[$0] })
    return tabs.contains(where: { $0?.visibleURL?.isShredAvailable == true })
  }

  func shredSelectedTab() {
    if mode == .tor, let tab = tabManager.selectedTab, tab.isTor {
      _ = tabManager.shredTorTabs([tab])
      return
    }
    guard let tab = tabManager.selectedTab, !tab.isTor, let url = tab.visibleURL else { return }
    tabManager.shredData(for: url, in: tab)
  }

  func shredTorTabs(_ ids: Set<TabState.ID>) -> Set<TabState.ID> {
    tabManager.shredTorTabs(tabManager.allTabs.filter { $0.isTor && ids.contains($0.id) })
  }

  func shredSelectedTabs(_ tabs: Set<TabState.ID>) -> Set<TabState.ID> {
    if mode == .tor {
      return tabManager.shredTorTabs(self.tabs.filter { tabs.contains($0.id) })
    }
    return tabManager.shredDataForTabs(tabs.compactMap({ tabManager[$0] }))
  }

  func shredAllTabs() {
    if mode == .tor {
      _ = tabManager.shredTorTabs(tabs)
      return
    }
    tabManager.shredAllTabsForCurrentMode()
  }

  func selectTab(_ tab: any TabState) {
    withAnimation {
      tabManager.selectTab(tab)
    }
  }

  func moveTabs(_ tabs: [TabState.ID], to index: Int) {
    tabManager.moveTabs(tabs, toIndex: index)
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

  private func persistPrivateTabs() {
    tabManager.saveAllTabs()
  }

  private func unpersistPrivateTabs() {
    if isPrivateBrowsing {
      // Since this change is happening inside tab grid, while private browsing, we dont actually
      // want to remove the tabs. If the user leaves private browsing they will be removed later
      SessionTab.deleteAll(tabIds: tabManager.allTabs.filter(\.isPrivate).map(\.id))
    } else {
      // Theoretically this shouldn't happen since the settings that change this pref are only
      // accessible when in private mode, but just in case we'll perform the regular mode wipe too
      tabManager.removeAllTabsForPrivateMode(isPrivate: true, isActiveTabIncluded: true)
    }
  }

  // MARK -

  private func updateTabs() {
    guard !isPrivateTabsLocked else { tabs = []; return }
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
      _isTorBrowsing ? tabManager.torTabSelectedIndex
        : (isPrivateBrowsing ? tabManager.privateTabSelectedIndex : tabManager.normalTabSelectedIndex)
    let tabToSelect = indexToSelect.flatMap { tabManager.allTabs[safe: $0] }
      .flatMap { $0.isTor == _isTorBrowsing && $0.isPrivate == isPrivateBrowsing ? $0 : nil }
    if isPrivateBrowsing {
      if _isTorBrowsing || Preferences.Privacy.persistentPrivateBrowsing.value, let tabToSelect {
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

    // Trigger scroll after ALL tab selection is complete
    Task {
      self.onModeSwitchComplete?()
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

    tabManager.privateBrowsingManager.$isTorBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self] value in
        guard let self else { return }
        _isTorBrowsing = value
        updateTabs()
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

    Preferences.Privacy.persistentPrivateBrowsing.$value
      .receive(on: RunLoop.main)
      .sink { [weak self] newValue in
        guard let self else { return }
        if newValue {
          persistPrivateTabs()
        } else {
          unpersistPrivateTabs()
        }
      }
      .store(in: &cancellables)

    let willDeactivate = NotificationCenter.default.addObserver(
      forName: UIScene.willDeactivateNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self else { return }
      MainActor.assumeIsolated {
        self.isSceneActive = false
      }
    }

    let didActivate = NotificationCenter.default.addObserver(
      forName: UIScene.didActivateNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self else { return }
      MainActor.assumeIsolated {
        self.isSceneActive = true
      }
    }

    cancellables.formUnion([
      .init { _ = willDeactivate },
      .init { _ = didActivate },
    ])
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
