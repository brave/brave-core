// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Combine
import Data
import LocalAuthentication
import Preferences
import Shared
import SwiftUI
import Web

@MainActor
@Observable
class TabGridViewModel {
  private var _isPrivateBrowsing: Bool = false
  private(set) var tabs: [any TabState] = []
  private(set) var items: [TabGridItem] = []
  private(set) var openGroup: TabGroup?
  private(set) var openGroupItems: [TabGridItem] = []
  private(set) var openGroupSourceFrame: CGRect?
  private var isNamingOpenGroup = false
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
    self.items = TabGridItem.items(for: tabs, using: tabManager)

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
    isSelectedTabShredAvailable
      || tabManager.tabsForCurrentMode.contains(where: { $0.visibleURL?.isShredAvailable == true })
  }

  var isSelectedTabShredAvailable: Bool {
    guard let selectedTab = tabManager.selectedTab,
      selectedTab.isPrivate == isPrivateBrowsing
    else { return false }
    return selectedTab.visibleURL?.isShredAvailable == true
  }

  func isShredAvailableForSelectedTabs(_ tabs: Set<TabState.ID>) -> Bool {
    let tabs = tabs.map({ tabManager[$0] })
    return tabs.contains(where: { $0?.visibleURL?.isShredAvailable == true })
  }

  func shredSelectedTab() {
    guard let tab = tabManager.selectedTab, let url = tab.visibleURL else { return }
    tabManager.shredData(for: url, in: tab)
  }

  func shredSelectedTabs(_ tabs: Set<TabState.ID>) -> Set<TabState.ID> {
    return tabManager.shredDataForTabs(tabs.compactMap({ tabManager[$0] }))
  }

  func shredAllTabs() {
    tabManager.shredAllTabsForCurrentMode()
  }

  func selectTab(_ tab: any TabState) {
    withAnimation {
      tabManager.selectTab(tab)
    }
  }

  /// Moves tabs to where the item at `itemIndex` is. Dropping on a group moves the tabs before the group's first tab.
  func moveTabs(_ tabs: [TabState.ID], toItemIndex itemIndex: Int) {
    let tabIndex: Int
    if let item = items[safe: itemIndex], let firstTab = item.tabs.first {
      tabIndex = self.tabs.firstIndex(where: { $0 === firstTab }) ?? self.tabs.count
    } else {
      tabIndex = self.tabs.count
    }
    tabManager.moveTabs(tabs, toIndex: tabIndex)
    updateTabs()
  }

  // MARK: - Tab Groups

  /// Whether tabs can be added to groups in the current browsing mode
  var canGroupTabs: Bool {
    tabManager.canGroupTabsForCurrentMode
  }

  var tabGroups: [TabGroup] {
    tabManager.tabGroups
  }

  func addTabs(_ tabIDs: Set<TabState.ID>, to group: TabGroup) {
    withAnimation {
      tabManager.addTabs(currentModeTabs(with: tabIDs), to: group)
    }
  }

  /// Creates a group with a default name out of the tabs and opens it, ready for the user to give it a name
  func createGroup(adding tabIDs: Set<TabState.ID>) {
    withAnimation {
      guard
        let group = tabManager.createTabGroup(
          title: defaultGroupTitle,
          adding: currentModeTabs(with: tabIDs)
        )
      else { return }
      isNamingOpenGroup = true
      showGroup(group)
    }
  }

  /// Whether the open group was just created and should be named. Reading this resets it.
  func consumeNamingRequest() -> Bool {
    defer { isNamingOpenGroup = false }
    return isNamingOpenGroup
  }

  func ungroup(_ group: TabGroup) {
    withAnimation {
      tabManager.ungroupTabs(in: group)
    }
  }

  func renameGroup(_ group: TabGroup, to title: String) {
    tabManager.renameTabGroup(group, to: title)
  }

  /// Adds a new tab to the end of the group and selects it
  func addTab(in group: TabGroup) {
    withAnimation {
      _ = tabManager.addTabAndSelect(in: group)
    }
  }

  func tabCount(in group: TabGroup) -> Int {
    tabManager.tabs(in: group).count
  }

  /// Closes every tab in the group, which removes the group
  func closeTabs(in group: TabGroup) {
    closeTabs(Set(tabManager.tabs(in: group).map(\.id)))
  }

  func showGroup(_ group: TabGroup, from sourceFrame: CGRect? = nil) {
    openGroupSourceFrame = sourceFrame
    openGroup = group
    updateOpenGroupItems()
  }

  func closeOpenGroup() {
    isNamingOpenGroup = false
    openGroupSourceFrame = nil
    openGroup = nil
    openGroupItems = []
  }

  private func updateOpenGroupItems() {
    guard let group = openGroup else { return }
    guard let currentGroup = tabManager.tabGroups.first(where: { $0 == group }) else {
      // The group is gone, e.g. its last tab was closed or it was ungrouped
      closeOpenGroup()
      return
    }
    // Pick up a new title
    openGroup = currentGroup
    openGroupItems = tabManager.tabs(in: currentGroup).map { TabGridItem.tab($0) }
  }

  /// The name given to a new group when the user doesn't provide one
  var defaultGroupTitle: String {
    String.localizedStringWithFormat(
      Strings.TabGrid.defaultGroupTitleFormat,
      tabManager.tabGroups.count + 1
    )
  }

  /// The given tabs in tab order
  private func currentModeTabs(with tabIDs: Set<TabState.ID>) -> [any TabState] {
    tabManager.tabsForCurrentMode.filter { tabIDs.contains($0.id) }
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
    let query = isSearching && !searchQuery.isEmpty ? searchQuery : nil
    tabs = tabManager.tabsForCurrentMode(for: query)
    // Search results are shown flat so that matches inside a group remain visible
    items =
      query != nil
      ? tabs.map { TabGridItem.tab($0) } : TabGridItem.items(for: tabs, using: tabManager)
    updateOpenGroupItems()
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
      items = []
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

    func tabManagerDidChangeTabGroups(_ tabManager: TabManager) {
      notify()
    }
  }
}
