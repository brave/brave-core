// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveWallet
import CoreData
import Data
import Favicon
import Foundation
import Growth
import Preferences
import Shared
import Storage
import WebKit
import os.log

protocol TabManagerDelegate: AnyObject {
  func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?)
  func tabManager(_ tabManager: TabManager, willAddTab tab: Tab)
  func tabManager(_ tabManager: TabManager, didAddTab tab: Tab)
  func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab)
  func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab)

  func tabManagerDidRestoreTabs(_ tabManager: TabManager)
  func tabManagerDidAddTabs(_ tabManager: TabManager)
  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?)
}

protocol TabManagerStateDelegate: AnyObject {
  func tabManagerWillStoreTabs(_ tabs: [Tab])
}

// We can't use a WeakList here because this is a protocol.
class WeakTabManagerDelegate {
  weak var value: TabManagerDelegate?

  init(value: TabManagerDelegate) {
    self.value = value
  }

  func get() -> TabManagerDelegate? {
    return value
  }
}

// TabManager must extend NSObjectProtocol in order to implement WKNavigationDelegate
class TabManager: NSObject {
  fileprivate var delegates = [WeakTabManagerDelegate]()
  fileprivate let tabEventHandlers: [TabEventHandler]
  weak var stateDelegate: TabManagerStateDelegate?

  /// Internal url to access the new tab page.
  static let ntpInteralURL = URL(string: "\(InternalURL.baseUrl)/\(AboutHomeHandler.path)#panel=0")!

  /// When a URL is invalid and can't be restored or loaded, we display about:blank#blocked (same as on Desktop)
  static let aboutBlankBlockedURL = URL(string: "about:blank")!

  func addDelegate(_ delegate: TabManagerDelegate) {
    assert(Thread.isMainThread)
    delegates.append(WeakTabManagerDelegate(value: delegate))
  }

  func removeDelegate(_ delegate: TabManagerDelegate) {
    assert(Thread.isMainThread)
    for i in 0..<delegates.count {
      let del = delegates[i]
      if delegate === del.get() || del.get() == nil {
        delegates.remove(at: i)
        return
      }
    }
  }

  private(set) var allTabs = [Tab]()
  private var _selectedIndex = -1
  private let navDelegate: TabManagerNavDelegate
  private(set) var isRestoring = false
  private(set) var isBulkDeleting = false

  // A WKWebViewConfiguration used for normal tabs
  lazy fileprivate var configuration: WKWebViewConfiguration = {
    return TabManager.getNewConfiguration()
  }()

  fileprivate let prefs: Prefs
  var selectedIndex: Int {
    return _selectedIndex
  }
  var normalTabSelectedIndex: Int = 0
  var privateTabSelectedIndex: Int = 0
  var tempTabs: [Tab]?
  private weak var rewards: BraveRewards?
  private weak var tabGeneratorAPI: BraveTabGeneratorAPI?
  private var domainFrc = Domain.frc()
  private let syncedTabsQueue = DispatchQueue(label: "synced-tabs-queue")
  private var syncTabsTask: DispatchWorkItem?
  private var metricsHeartbeat: Timer?
  private let historyAPI: BraveHistoryAPI?
  public let privateBrowsingManager: PrivateBrowsingManager
  private var forgetTasks: [TabType: [String: Task<Void, Error>]] = [:]

  let windowId: UUID

  /// The property returning only existing tab is NTP for current mode
  var isBrowserEmptyForCurrentMode: Bool {
    guard tabsForCurrentMode.count == 1,
      let tabURL = tabsForCurrentMode.first?.url,
      InternalURL(tabURL)?.isAboutHomeURL == true
    else {
      return false
    }

    return true
  }

  init(
    windowId: UUID,
    prefs: Prefs,
    rewards: BraveRewards?,
    tabGeneratorAPI: BraveTabGeneratorAPI?,
    historyAPI: BraveHistoryAPI?,
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    assert(Thread.isMainThread)

    self.windowId = windowId
    self.prefs = prefs
    self.navDelegate = TabManagerNavDelegate()
    self.rewards = rewards
    self.tabGeneratorAPI = tabGeneratorAPI
    self.historyAPI = historyAPI
    self.privateBrowsingManager = privateBrowsingManager
    self.tabEventHandlers = TabEventHandlers.create(with: prefs)
    super.init()

    self.navDelegate.tabManager = self
    addNavigationDelegate(self)

    Preferences.Shields.blockImages.observe(from: self)
    Preferences.General.blockPopups.observe(from: self)
    Preferences.General.nightModeEnabled.observe(from: self)

    domainFrc.delegate = self
    do {
      try domainFrc.performFetch()
    } catch {
      Logger.module.error(
        "Failed to perform fetch of Domains for observing dapps permission changes: \(error.localizedDescription, privacy: .public)"
      )
    }

    // Initially fired and set up after tabs are restored
    metricsHeartbeat = Timer(
      timeInterval: 5.minutes,
      repeats: true,
      block: { [weak self] _ in
        self?.recordTabCountP3A()
      }
    )
  }

  deinit {
    syncTabsTask?.cancel()
  }

  func addNavigationDelegate(_ delegate: WKNavigationDelegate) {
    assert(Thread.isMainThread)

    self.navDelegate.insert(delegate)
  }

  var count: Int {
    assert(Thread.isMainThread)

    return allTabs.count
  }

  var selectedTab: Tab? {
    assert(Thread.isMainThread)
    if !(0..<count ~= _selectedIndex) {
      return nil
    }

    return allTabs[_selectedIndex]
  }

  subscript(index: Int) -> Tab? {
    assert(Thread.isMainThread)

    if index >= allTabs.count {
      return nil
    }
    return allTabs[index]
  }

  subscript(webView: WKWebView) -> Tab? {
    assert(Thread.isMainThread)

    for tab in allTabs where tab.webView === webView {
      return tab
    }

    return nil
  }

  var currentDisplayedIndex: Int? {
    assert(Thread.isMainThread)

    guard let selectedTab = self.selectedTab else {
      return nil
    }

    return tabsForCurrentMode.firstIndex(of: selectedTab)
  }

  // What the users sees displayed based on current private browsing mode
  var tabsForCurrentMode: [Tab] {
    let tabType: TabType = privateBrowsingManager.isPrivateBrowsing ? .private : .regular
    return tabs(withType: tabType)
  }

  var openedWebsitesCount: Int {
    tabsForCurrentMode.filter {
      if let url = $0.url {
        return url.isWebPage() && !(InternalURL(url)?.isAboutHomeURL ?? false)
      }
      return false
    }.count
  }

  func tabsForCurrentMode(for query: String? = nil) -> [Tab] {
    if let query = query {
      let tabType: TabType = privateBrowsingManager.isPrivateBrowsing ? .private : .regular
      return tabs(withType: tabType, query: query)
    } else {
      return tabsForCurrentMode
    }
  }

  func tabsCountForMode(isPrivate: Bool) -> Int {
    let tabType: TabType = isPrivate ? .private : .regular
    return tabs(withType: tabType).count
  }

  private func tabs(withType type: TabType, query: String? = nil) -> [Tab] {
    assert(Thread.isMainThread)

    let allTabs = allTabs.filter { $0.type == type }

    if let query = query, !query.isEmpty {
      // Display title is the only data that will be present on every situation
      return allTabs.filter {
        $0.displayTitle.lowercased().contains(query)
          || ($0.url?.baseDomain?.contains(query) ?? false)
      }
    } else {
      return allTabs
    }
  }

  /// Function for adding local tabs as synced sessions
  /// This is used when open tabs toggle is enabled in sync settings and browser constructor
  func addRegularTabsToSyncChain() {
    let regularTabs = tabs(withType: .regular)

    syncTabsTask?.cancel()

    syncTabsTask = DispatchWorkItem {
      guard let task = self.syncTabsTask, !task.isCancelled else {
        return
      }

      for tab in regularTabs {
        if let url = tab.fetchedURL, !tab.type.isPrivate, !url.isLocal,
          !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode)
        {
          tab.addTabInfoToSyncedSessions(url: url, displayTitle: tab.displayTitle)
        }
      }
    }

    if let task = self.syncTabsTask {
      DispatchQueue.main.async(execute: task)
    }
  }

  private class func getNewConfiguration() -> WKWebViewConfiguration {
    let configuration = WKWebViewConfiguration()
    configuration.processPool = WKProcessPool()
    configuration.preferences.javaScriptCanOpenWindowsAutomatically = !Preferences.General
      .blockPopups.value

    // Dev note: Do NOT add `.link` to the list, it breaks interstitial pages
    // and pages that don't want the URL highlighted!
    configuration.dataDetectorTypes = [.phoneNumber]

    return configuration
  }

  func resetConfiguration() {
    configuration = TabManager.getNewConfiguration()
  }

  func reset() {
    resetConfiguration()
    allTabs.filter({ $0.webView != nil }).forEach({
      $0.resetWebView(config: configuration)
    })
  }

  func clearTabHistory(_ completion: (() -> Void)? = nil) {
    allTabs.filter({ $0.webView != nil }).forEach({
      $0.clearHistory(config: configuration)
    })

    completion?()
  }

  func reloadSelectedTab() {
    let tab = selectedTab
    _selectedIndex = -1
    selectTab(tab)
    if let url = selectedTab?.url {
      selectedTab?.loadRequest(PrivilegedRequest(url: url) as URLRequest)
    }
  }

  func selectTab(_ tab: Tab?, previous: Tab? = nil) {
    assert(Thread.isMainThread)
    let previous = previous ?? selectedTab

    if previous === tab {
      return
    }
    // Convert the global mode to private if opening private tab from normal tab/ history/bookmark.
    if selectedTab?.isPrivate != true && tab?.isPrivate == true {
      privateBrowsingManager.isPrivateBrowsing = true
    }
    // Make sure to wipe the private tabs if the user has the pref turned on
    if !TabType.of(tab).isPrivate
      && (Preferences.Privacy.privateBrowsingOnly.value
        || !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      removeAllPrivateTabs()
    }

    if let tab = tab {
      _selectedIndex = allTabs.firstIndex(of: tab) ?? -1
    } else {
      _selectedIndex = -1
    }

    if let previousTab = previous {
      preserveScreenshot(for: previousTab)
    }

    if let t = selectedTab, t.webView == nil {
      selectedTab?.createWebview()
      restoreTab(t)
    }

    guard tab === selectedTab else {
      Logger.module.error(
        "Expected tab (\(tab?.url?.absoluteString ?? "nil")) is not selected. Selected index: \(self.selectedIndex)"
      )
      return
    }

    if let tabId = tab?.id {
      SessionTab.setSelected(tabId: tabId)
    }

    UIImpactFeedbackGenerator(style: .light).vibrate()
    selectedTab?.createWebview()
    selectedTab?.lastExecutedTime = Date.now()

    if let selectedTab = selectedTab,
      let webView = selectedTab.webView,
      webView.url == nil
    {

      selectedTab.url = selectedTab.url ?? TabManager.ntpInteralURL
      restoreTab(selectedTab)
      Logger.module.error("Force Restored a Zombie Tab?!")
    }

    delegates.forEach { $0.get()?.tabManager(self, didSelectedTabChange: tab, previous: previous) }
    if let tab = previous {
      TabEvent.post(.didLoseFocus, for: tab)
    }
    if let tab = selectedTab {
      TabEvent.post(.didGainFocus, for: tab)
    }

    if let tabID = tab?.id {
      SessionTab.touch(tabId: tabID)
    }

    guard let newSelectedTab = tab, let previousTab = previous, let newTabUrl = newSelectedTab.url,
      let previousTabUrl = previousTab.url
    else { return }

    if !privateBrowsingManager.isPrivateBrowsing {
      if previousTab.displayFavicon == nil {
        adsRewardsLog.warning("No favicon found in \(previousTab) to report to rewards panel")
      }
      rewards?.reportTabUpdated(
        tab: previousTab,
        isSelected: false,
        isPrivate: previousTab.isPrivate
      )

      if newSelectedTab.displayFavicon == nil && !newTabUrl.isLocal {
        adsRewardsLog.warning("No favicon found in \(newSelectedTab) to report to rewards panel")
      }
      rewards?.reportTabUpdated(
        tab: newSelectedTab,
        isSelected: true,
        isPrivate: newSelectedTab.isPrivate
      )
    }
  }

  // Called by other classes to signal that they are entering/exiting private mode
  // This is called by TabTrayVC when the private mode button is pressed and BEFORE we've switched to the new mode
  // we only want to remove all private tabs when leaving PBM and not when entering.
  func willSwitchTabMode(leavingPBM: Bool) {
    if leavingPBM {
      if Preferences.Privacy.privateBrowsingOnly.value
        || !Preferences.Privacy.persistentPrivateBrowsing.value
      {
        removeAllPrivateTabs()
      }
    }
  }

  /// Called to turn selectedIndex back to -1
  func resetSelectedIndex() {
    _selectedIndex = -1
  }

  func expireSnackbars() {
    assert(Thread.isMainThread)

    for tab in allTabs {
      tab.expireSnackbars()
    }
  }

  @MainActor func addPopupForParentTab(
    _ parentTab: Tab,
    configuration: WKWebViewConfiguration
  ) -> Tab {
    let popup = Tab(
      configuration: configuration,
      id: UUID(),
      type: parentTab.type,
      tabGeneratorAPI: tabGeneratorAPI
    )
    configureTab(
      popup,
      request: nil,
      afterTab: parentTab,
      flushToDisk: true,
      zombie: false,
      isPopup: true
    )
    return popup
  }

  @discardableResult
  @MainActor func addTabAndSelect(
    _ request: URLRequest! = nil,
    afterTab: Tab? = nil,
    isPrivate: Bool
  ) -> Tab {
    let tab = addTab(request, afterTab: afterTab, isPrivate: isPrivate)
    selectTab(tab)
    return tab
  }

  @MainActor func addTabsForURLs(_ urls: [URL], zombie: Bool, isPrivate: Bool = false) {
    assert(Thread.isMainThread)

    if urls.isEmpty {
      return
    }
    // When bulk adding tabs don't notify delegates until we are done
    self.isRestoring = true

    var tabs = [Tab]()
    for url in urls {
      let request =
        InternalURL.isValid(url: url)
        ? PrivilegedRequest(url: url) as URLRequest : URLRequest(url: url)
      let tab = self.addTab(
        request,
        flushToDisk: false,
        zombie: true,
        isPrivate: isPrivate
      )
      tab.lastTitle = url.absoluteDisplayString
      tab.url = url
      tab.favicon = Favicon.default
      Task { @MainActor in
        if let icon = await FaviconFetcher.getIconFromCache(for: url) {
          tab.favicon = icon
        }
      }
      tabs.append(tab)
    }

    // Load at most X of the most recent tabs
    // IE: Load the last X tabs, lazy load all the rest
    let amountOfTabsToRestoreImmediately = 5
    Array(tabs.suffix(amountOfTabsToRestoreImmediately)).reversed().forEach {
      guard let url = $0.url else { return }
      let request =
        InternalURL.isValid(url: url)
        ? PrivilegedRequest(url: url) as URLRequest : URLRequest(url: url)
      $0.createWebview()
      $0.loadRequest(request)
    }

    // Select the most recent.
    self.selectTab(tabs.last)
    self.isRestoring = false
    // Okay now notify that we bulk-loaded so we can adjust counts and animate changes.
    delegates.forEach { $0.get()?.tabManagerDidAddTabs(self) }
  }

  @discardableResult
  @MainActor func addTab(
    _ request: URLRequest? = nil,
    afterTab: Tab? = nil,
    flushToDisk: Bool = true,
    zombie: Bool = false,
    id: UUID? = nil,
    isPrivate: Bool
  ) -> Tab {
    assert(Thread.isMainThread)

    let tabId = id ?? UUID()
    let type: TabType = isPrivate ? .private : .regular
    let tab = Tab(
      configuration: configuration,
      id: tabId,
      type: type,
      tabGeneratorAPI: tabGeneratorAPI
    )
    configureTab(
      tab,
      request: request,
      afterTab: afterTab,
      flushToDisk: flushToDisk,
      zombie: zombie
    )
    return tab
  }

  func moveTab(_ tab: Tab, toIndex visibleToIndex: Int) {
    assert(Thread.isMainThread)

    let currentTabs = tabs(withType: tab.type)

    let toTab = currentTabs[visibleToIndex]

    guard let fromIndex = allTabs.firstIndex(of: tab), let toIndex = allTabs.firstIndex(of: toTab)
    else {
      return
    }

    // Make sure to save the selected tab before updating the tabs list
    let previouslySelectedTab = selectedTab

    let tab = allTabs.remove(at: fromIndex)
    allTabs.insert(tab, at: toIndex)

    if let previouslySelectedTab = previouslySelectedTab,
      let previousSelectedIndex = allTabs.firstIndex(of: previouslySelectedTab)
    {
      _selectedIndex = previousSelectedIndex
    }

    saveTabOrder()
  }

  private func saveTabOrder() {
    if Preferences.Privacy.privateBrowsingOnly.value
      || (privateBrowsingManager.isPrivateBrowsing
        && !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      return
    }
    let allTabIds = allTabs.compactMap { $0.id }
    SessionTab.saveTabOrder(tabIds: allTabIds)
  }

  @MainActor func configureTab(
    _ tab: Tab,
    request: URLRequest?,
    afterTab parent: Tab? = nil,
    flushToDisk: Bool,
    zombie: Bool,
    isPopup: Bool = false
  ) {
    assert(Thread.isMainThread)

    var request = request
    let isPrivate = tab.type == .private
    let isPersistentTab =
      !isPrivate
      || (isPrivate && !Preferences.Privacy.privateBrowsingOnly.value
        && Preferences.Privacy.persistentPrivateBrowsing.value)

    // WebKit can sometimes return a URL that isn't valid at all!
    if let requestURL = request?.url, NSURL(idnString: requestURL.absoluteString) == nil {
      request?.url = TabManager.aboutBlankBlockedURL
    }

    if isPersistentTab {
      SessionTab.createIfNeeded(
        windowId: windowId,
        tabId: tab.id,
        title: Strings.newTab,
        tabURL: request?.url ?? TabManager.ntpInteralURL,
        isPrivate: isPrivate
      )
    }

    delegates.forEach { $0.get()?.tabManager(self, willAddTab: tab) }

    if parent == nil || parent?.type != tab.type {
      allTabs.append(tab)
    } else if let parent = parent, var insertIndex = allTabs.firstIndex(of: parent) {
      insertIndex += 1
      while insertIndex < allTabs.count && allTabs[insertIndex].isDescendentOf(parent) {
        insertIndex += 1
      }
      tab.parent = parent
      allTabs.insert(tab, at: insertIndex)
    }

    delegates.forEach { $0.get()?.tabManager(self, didAddTab: tab) }

    if !zombie {
      tab.createWebview()
    }
    tab.navigationDelegate = self.navDelegate

    if let request = request {
      tab.loadRequest(request)
      tab.url = request.url
    } else if !isPopup {
      tab.loadRequest(PrivilegedRequest(url: TabManager.ntpInteralURL) as URLRequest)
      tab.url = TabManager.ntpInteralURL
    }

    // Ignore on restore.
    if flushToDisk && !zombie && isPersistentTab {
      saveTab(tab, saveOrder: true)
    }

    // When the state of the page changes, we debounce a call to save the screenshots and tab information
    // This fixes pages that have dynamic URL via changing history
    // as well as regular pages that load DOM normally.
    tab.onPageReadyStateChanged = { [weak tab] state in
      guard let tab = tab else { return }
      tab.webStateDebounceTimer?.invalidate()
      tab.webStateDebounceTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: false) {
        [weak self, weak tab] _ in
        guard let self = self, let tab = tab else { return }
        tab.webStateDebounceTimer?.invalidate()

        if state == .complete || state == .loaded || state == .pushstate || state == .popstate
          || state == .replacestate
        {

          if Preferences.Privacy.privateBrowsingOnly.value
            || (tab.isPrivate && !Preferences.Privacy.persistentPrivateBrowsing.value)
          {
            return
          }

          tab.resetExternalAlertProperties()
          self.preserveScreenshot(for: tab)
          self.saveTab(tab)
        }
      }
    }
  }

  func indexOfWebView(_ webView: WKWebView) -> UInt? {
    objc_sync_enter(self)
    defer { objc_sync_exit(self) }

    var count = UInt(0)
    for tab in allTabs {
      if tab.webView === webView {
        return count
      }
      count = count + 1
    }

    return nil
  }

  func removePrivateWindows() {
    if Preferences.Privacy.privateBrowsingOnly.value
      || (privateBrowsingManager.isPrivateBrowsing
        && !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      SessionWindow.deleteAllWindows(privateOnly: true)
    }
  }

  func saveAllTabs() {
    if Preferences.Privacy.privateBrowsingOnly.value
      || (privateBrowsingManager.isPrivateBrowsing
        && !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      return
    }

    let tabs =
      Preferences.Privacy.persistentPrivateBrowsing.value ? allTabs : tabs(withType: .regular)
    SessionTab.updateAll(
      tabs: tabs.compactMap({
        if let sessionData = $0.webView?.sessionData {
          return ($0.id, sessionData, $0.title, $0.url ?? TabManager.ntpInteralURL)
        }
        return nil
      })
    )
  }

  func saveTab(_ tab: Tab, saveOrder: Bool = false) {
    if Preferences.Privacy.privateBrowsingOnly.value
      || (tab.isPrivate && !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      return
    }
    SessionTab.update(
      tabId: tab.id,
      interactionState: tab.webView?.sessionData ?? Data(),
      title: tab.title,
      url: tab.url ?? TabManager.ntpInteralURL
    )
    if saveOrder {
      saveTabOrder()
    }
  }

  /// Forget all data for websites that have forget me enabled
  /// Will forget all data instantly with no delay
  @MainActor func forgetDataOnAppClose() {
    for tab in tabs(withType: .regular) {
      guard let url = tab.url, !InternalURL.isValid(url: url) else {
        continue
      }

      let siteDomain = Domain.getOrCreate(
        forUrl: url,
        persistent: !tab.isPrivate
      )

      if siteDomain.shredLevel.shredOnAppExit {
        Task {
          await forgetData(for: url, in: tab)
        }
      }
    }
  }

  /// Forget all data for websites if the website has "Forget Me" enabled
  ///
  /// A delay allows us to cancel this forget in case the user goes back to this website.
  ///
  /// - Parameters:
  ///   - url: The url of the website to forget
  ///   - tab: The tab in which the website is or was open in
  ///   - delayInSeconds: Only attempt to forget the content after a short delay (default: 30s)
  ///   - checkOtherTabs: Check if other tabs are open for the given domain
  @MainActor func forgetDataIfNeeded(
    for url: URL,
    in tab: Tab
  ) {
    guard FeatureList.kBraveShredFeature.enabled else { return }
    guard let etldP1 = url.baseDomain else { return }
    forgetTasks[tab.type]?[etldP1]?.cancel()
    let siteDomain = Domain.getOrCreate(
      forUrl: url,
      persistent: !tab.isPrivate
    )

    switch siteDomain.shredLevel {
    case .appExit, .never:
      return
    case .whenSiteClosed:
      let tabs = tabs(withType: tab.type).filter { existingTab in
        existingTab != tab
      }
      // Ensure that no othe tabs are open for this domain
      guard !tabs.contains(where: { $0.url?.baseDomain == etldP1 }) else {
        return
      }
      forgetDataDelayed(for: url, in: tab, delay: 30)
    }
  }

  @MainActor func shredData(for url: URL, in tab: Tab) {
    guard let etldP1 = url.baseDomain else { return }

    // Select the next or previous tab that is not being destroyed
    if let index = allTabs.firstIndex(where: { $0 == tab }) {
      var nextTab: Tab?
      // First seach down or up for a tab that is not being destroyed
      var increasingIndex = index + 1
      while nextTab == nil, increasingIndex < allTabs.count {
        if allTabs[increasingIndex].url?.baseDomain != etldP1 {
          nextTab = allTabs[increasingIndex]
        }
        increasingIndex += 1
      }

      var decreasingIndex = index - 1
      while nextTab == nil, decreasingIndex > 0 {
        if allTabs[decreasingIndex].url?.baseDomain != etldP1 {
          nextTab = allTabs[decreasingIndex]
        }
        decreasingIndex -= 1
      }

      // Select the found tab
      if let nextTab = nextTab {
        selectTab(nextTab, previous: tab)
      }
    }

    // Remove all unwanted tabs
    for tab in allTabs {
      guard tab.url?.baseDomain == etldP1 else { continue }
      removeTab(tab)
    }

    Task {
      // Forget all the data
      await forgetData(for: url, in: tab)
    }
  }

  /// Start a task to delete all data for this url
  /// The task may be delayed in case we want to cancel it
  @MainActor private func forgetDataDelayed(
    for url: URL,
    in tab: Tab,
    delay: TimeInterval
  ) {
    guard let etldP1 = url.baseDomain else { return }
    forgetTasks[tab.type] = forgetTasks[tab.type] ?? [:]
    // Start a task to delete all data for this etldP1
    // The task may be delayed in case we want to cancel it
    forgetTasks[tab.type]?[etldP1] = Task {
      try await Task.sleep(seconds: delay)
      await self.forgetData(for: url, in: tab)
    }
  }

  @MainActor private func forgetData(for url: URL, in tab: Tab) async {
    await FaviconFetcher.deleteCache(for: url)
    guard let etldP1 = url.baseDomain else { return }

    let dataStore = tab.webView?.configuration.websiteDataStore
    // Delete 1P data records
    await dataStore?.deleteDataRecords(
      forDomain: etldP1
    )
    if BraveCore.FeatureList.kBraveShredCacheData.enabled {
      // Delete all cache data (otherwise 3P cache entries left behind
      // are visible in Manage Website Data view brave-browser #41095)
      let cacheTypes = Set([
        WKWebsiteDataTypeMemoryCache, WKWebsiteDataTypeDiskCache,
        WKWebsiteDataTypeOfflineWebApplicationCache,
      ])
      let cacheRecords = await dataStore?.dataRecords(ofTypes: cacheTypes) ?? []
      await dataStore?.removeData(ofTypes: cacheTypes, for: cacheRecords)
    }

    // Delete the history for forgotten websites
    if let historyAPI = self.historyAPI {
      let nodes = await historyAPI.search(
        withQuery: etldP1,
        options: HistorySearchOptions(
          maxCount: 0,
          hostOnly: false,
          duplicateHandling: .keepAll,
          begin: nil,
          end: nil
        )
      ).filter { node in
        node.url.baseDomain == etldP1
      }
      historyAPI.removeHistory(for: nodes)
    }

    ContentBlockerManager.log.debug("Cleared website data for `\(etldP1)`")
    forgetTasks[tab.type]?.removeValue(forKey: etldP1)
  }

  /// Cancel a forget data request in case we navigate back to the tab within a certain period
  @MainActor func cancelForgetData(for url: URL, in tab: Tab) {
    guard let etldP1 = url.baseDomain else { return }
    forgetTasks[tab.type]?[etldP1]?.cancel()
    forgetTasks[tab.type]?.removeValue(forKey: etldP1)
  }

  @MainActor func removeTab(_ tab: Tab) {
    guard let removalIndex = allTabs.firstIndex(where: { $0 === tab }) else {
      Logger.module.debug("Could not find index of tab to remove")
      return
    }

    if let url = tab.url {
      // Check if this data needs to be forgotten
      forgetDataIfNeeded(for: url, in: tab)
    }

    if tab.isPrivate {
      // Only when ALL tabs are dead, we clean up.
      // This is because other tabs share the same data-store.
      if tabs(withType: .private).count <= 1 {
        removeAllBrowsingDataForTab(tab)

        // After clearing the very last webview from the storage, give it a blank persistent store
        // This is the only way to guarantee that the last reference to the shared persistent store
        // reaches zero and destroys all its data.

        BraveWebView.removeNonPersistentStore()
      }
    }

    let oldSelectedTab = selectedTab

    delegates.forEach { $0.get()?.tabManager(self, willRemoveTab: tab) }

    // The index of the tab in its respective tab grouping. Used to figure out which tab is next
    var currentTabs = tabs(withType: tab.type)

    var tabIndex: Int = -1
    if let oldTab = oldSelectedTab {
      tabIndex = currentTabs.firstIndex(of: oldTab) ?? -1
    }

    let prevCount = count
    allTabs.remove(at: removalIndex)

    SessionTab.delete(tabId: tab.id)

    currentTabs = tabs(withType: tab.type)

    // Let's select the tab to be selected next.
    if let oldTab = oldSelectedTab, tab !== oldTab {
      // If it wasn't the selected tab we removed, then keep it like that.
      // It might have changed index, so we look it up again.
      _selectedIndex = allTabs.firstIndex(of: oldTab) ?? -1
    } else if let parentTab = tab.parent,
      currentTabs.count > 1,
      let newTab = currentTabs.reduce(
        currentTabs.first,
        { currentBestTab, tab2 in
          if let tab1 = currentBestTab, let time1 = tab1.lastExecutedTime {
            if let time2 = tab2.lastExecutedTime {
              return time1 <= time2 ? tab2 : tab1
            }
            return tab1
          } else {
            return tab2
          }
        }
      ), parentTab == newTab, tab !== newTab, newTab.lastExecutedTime != nil
    {
      // We select the most recently visited tab, only if it is also the parent tab of the closed tab.
      _selectedIndex = allTabs.firstIndex(of: newTab) ?? -1
    } else {
      // By now, we've just removed the selected one, and no previously loaded
      // tabs. So let's load the final one in the tab tray.
      if tabIndex == currentTabs.count {
        tabIndex -= 1
      }

      if let currentTab = currentTabs[safe: tabIndex] {
        _selectedIndex = allTabs.firstIndex(of: currentTab) ?? -1
      } else {
        _selectedIndex = -1
      }
    }

    assert(count == prevCount - 1, "Make sure the tab count was actually removed")

    // There's still some time between this and the webView being destroyed. We don't want to pick up any stray events.
    tab.webView?.navigationDelegate = nil

    delegates.forEach { $0.get()?.tabManager(self, didRemoveTab: tab) }
    TabEvent.post(.didClose, for: tab)

    if currentTabs.isEmpty {
      addTab(isPrivate: tab.isPrivate)
    }

    // If the removed tab was selected, find the new tab to select.
    if selectedTab != nil {
      selectTab(selectedTab, previous: oldSelectedTab)
    } else {
      selectTab(allTabs.last, previous: oldSelectedTab)
    }
  }

  /// Removes all private tabs from the manager without notifying delegates.
  private func removeAllPrivateTabs() {
    // reset the selectedTabIndex if we are on a private tab because we will be removing it.
    if TabType.of(selectedTab).isPrivate {
      _selectedIndex = -1
    }

    allTabs.forEach { tab in
      if tab.isPrivate {
        tab.webView?.removeFromSuperview()
        removeAllBrowsingDataForTab(tab)
      }
    }

    BraveWebView.removeNonPersistentStore()

    allTabs = tabs(withType: .regular)
  }

  func removeAllBrowsingDataForTab(_ tab: Tab, completionHandler: @escaping () -> Void = {}) {
    let dataTypes = WKWebsiteDataStore.allWebsiteDataTypes()
    tab.webView?.configuration.websiteDataStore.removeData(
      ofTypes: dataTypes,
      modifiedSince: Date.distantPast,
      completionHandler: completionHandler
    )
  }

  @MainActor func removeTabsWithUndoToast(_ tabs: [Tab]) {
    isBulkDeleting = true
    tempTabs = tabs
    var tabsCopy = tabs

    // Remove the current tab last to prevent switching tabs while removing tabs
    if let selectedTab = selectedTab {
      if let selectedIndex = tabsCopy.firstIndex(of: selectedTab) {
        let removed = tabsCopy.remove(at: selectedIndex)
        removeTabs(tabsCopy)
        removeTab(removed)
      } else {
        removeTabs(tabsCopy)
      }
    }
    for tab in tabs {
      tab.hideContent()
    }
    var toast: ButtonToast?
    if let numberOfTabs = tempTabs?.count, numberOfTabs > 0 {
      toast = ButtonToast(
        labelText: String.localizedStringWithFormat(Strings.tabsDeleteAllUndoTitle, numberOfTabs),
        buttonText: Strings.tabsDeleteAllUndoAction,
        completion: { buttonPressed in
          if buttonPressed {
            self.undoCloseTabs()
            for delegate in self.delegates {
              delegate.get()?.tabManagerDidAddTabs(self)
            }
          }
          self.eraseUndoCache()
        }
      )
    }

    isBulkDeleting = false
    delegates.forEach { $0.get()?.tabManagerDidRemoveAllTabs(self, toast: toast) }
  }

  @MainActor func undoCloseTabs() {
    guard let tempTabs = self.tempTabs, !tempTabs.isEmpty else {
      return
    }

    let tabsCopy = tabs(withType: .regular)

    restoreDeletedTabs(tempTabs)
    self.isRestoring = true

    for tab in tempTabs {
      tab.showContent(true)
    }

    let tab = tempTabs.first
    if !TabType.of(tab).isPrivate {
      removeTabs(tabsCopy)
    }
    selectTab(tab)

    self.isRestoring = false
    delegates.forEach { $0.get()?.tabManagerDidRestoreTabs(self) }
    self.tempTabs?.removeAll()
    allTabs.first?.createWebview()
  }

  func eraseUndoCache() {
    tempTabs?.removeAll()
  }

  @MainActor func removeTabs(_ tabs: [Tab]) {
    for tab in tabs {
      self.removeTab(tab)
    }
  }

  @MainActor func removeAll() {
    isBulkDeleting = true
    removeTabs(self.allTabs)
    isBulkDeleting = false
    delegates.forEach { $0.get()?.tabManagerDidRemoveAllTabs(self, toast: nil) }
  }

  @MainActor func removeAllForCurrentMode(isActiveTabIncluded: Bool = true) {
    isBulkDeleting = true

    if isActiveTabIncluded {
      removeTabs(tabsForCurrentMode)
    } else {
      let tabsToDelete = tabsForCurrentMode.filter {
        guard let currentTab = selectedTab else { return false }
        return currentTab.id != $0.id
      }
      removeTabs(tabsToDelete)
    }

    isBulkDeleting = false
    // No change needed here regarding to isActiveTabIncluded
    // Toast value is nil and TabsBarViewController is updating the from current tabs
    delegates.forEach { $0.get()?.tabManagerDidRemoveAllTabs(self, toast: nil) }
  }

  func getIndex(_ tab: Tab) -> Int? {
    assert(Thread.isMainThread)

    for i in 0..<count where allTabs[i] === tab {
      return i
    }

    assertionFailure("Tab not in tabs list")
    return nil
  }

  func getTabForURL(_ url: URL, isPrivate: Bool) -> Tab? {
    assert(Thread.isMainThread)

    let tab = allTabs.filter {
      guard let webViewURL = $0.webView?.url, $0.isPrivate else {
        return false
      }

      return webViewURL.schemelessAbsoluteDisplayString == url.schemelessAbsoluteDisplayString
    }.first

    return tab
  }

  func getTabForID(_ id: UUID) -> Tab? {
    assert(Thread.isMainThread)
    return allTabs.filter { $0.id == id }.first
  }

  func resetProcessPool() {
    assert(Thread.isMainThread)

    configuration.processPool = WKProcessPool()
  }

  private func preserveScreenshot(for tab: Tab) {
    assert(Thread.isMainThread)
    if isRestoring { return }

    guard let screenshot = tab.screenshot else { return }
    SessionTab.updateScreenshot(tabId: tab.id, screenshot: screenshot)
  }

  @MainActor fileprivate var restoreTabsInternal: Tab? {
    var savedTabs = [SessionTab]()

    if let autocloseTime = Preferences.AutoCloseTabsOption(
      rawValue: Preferences.General.autocloseTabs.value
    )?.timeInterval {
      // To avoid db problems, we first retrieve fresh tabs(on main thread context)
      // then delete old tabs(background thread context)
      savedTabs = SessionTab.all(noOlderThan: autocloseTime)
      SessionTab.deleteAll(olderThan: autocloseTime)
    } else {
      savedTabs = SessionTab.all()
    }

    savedTabs = savedTabs.filter({ $0.sessionWindow?.windowId == windowId })
    if savedTabs.isEmpty { return nil }

    var tabToSelect: Tab?
    for savedTab in savedTabs {
      if let tabURL = savedTab.url {
        // Provide an empty request to prevent a new tab from loading the home screen
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        let tab = addTab(
          request,
          flushToDisk: false,
          zombie: true,
          id: savedTab.tabId,
          isPrivate: savedTab.isPrivate
        )

        tab.lastTitle = savedTab.title
        tab.favicon = Favicon.default
        tab.setScreenshot(savedTab.screenshot)

        Task { @MainActor in
          tab.favicon = try await FaviconFetcher.loadIcon(
            url: tabURL,
            kind: .smallIcon,
            persistent: !tab.isPrivate
          )
          tab.setScreenshot(savedTab.screenshot)
        }

        // Do not select the private tab since we always restore to regular mode!
        if savedTab.isSelected && !savedTab.isPrivate {
          tabToSelect = tab
        }
      } else {
        let tab = addTab(
          nil,
          flushToDisk: false,
          zombie: true,
          id: savedTab.tabId,
          isPrivate: savedTab.isPrivate
        )

        tab.lastTitle = savedTab.title
        tab.favicon = Favicon.default
        tab.setScreenshot(savedTab.screenshot)

        // Do not select the private tab since we always restore to regular mode!
        if savedTab.isSelected && !savedTab.isPrivate {
          tabToSelect = tab
        }
      }
    }

    if let tabToSelect = tabToSelect ?? tabsForCurrentMode.last {
      // Only tell our delegates that we restored tabs if we actually restored something
      delegates.forEach {
        $0.get()?.tabManagerDidRestoreTabs(self)
      }

      // No tab selection, since this is unfamiliar with launch timings (e.g. compiling blocklists)

      // Must return inside this `if` to potentially return the conditional fallback
      return tabToSelect
    }
    return nil
  }

  func restoreTab(_ tab: Tab) {
    guard let webView = tab.webView else { return }
    guard let sessionTab = SessionTab.from(tabId: tab.id) else {

      // Restore Tab with its Last-Request URL
      tab.navigationDelegate = navDelegate

      var sessionData: (String, URLRequest)?

      if let tabURL = tab.url {
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        sessionData = (tab.lastTitle ?? tabURL.absoluteDisplayString, request)
      }

      tab.restore(
        webView,
        requestRestorationData: sessionData
      )
      return
    }

    // Tab was created with no active webview session data.
    // Restore tab data from Core-Data URL, and configure it.
    if sessionTab.interactionState.isEmpty {
      tab.navigationDelegate = navDelegate

      if let tabURL = sessionTab.url {
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        tab.restore(
          webView,
          requestRestorationData: (tabURL.absoluteDisplayString, request)
        )
      } else {
        tab.restore(
          webView,
          requestRestorationData: (
            TabManager.aboutBlankBlockedURL.absoluteDisplayString,
            URLRequest(url: TabManager.aboutBlankBlockedURL)
          )
        )
      }
      return
    }

    // Restore tab data from Core-Data, and configure it.
    tab.navigationDelegate = navDelegate
    tab.restore(
      webView,
      restorationData: (sessionTab.title, sessionTab.interactionState)
    )
  }

  /// Restores all tabs.
  /// Returns the tab that has to be selected after restoration.
  @MainActor var restoreAllTabs: Tab {
    defer {
      metricsHeartbeat?.fire()
      RunLoop.current.add(metricsHeartbeat!, forMode: .default)
    }

    isRestoring = true
    let tabToSelect = self.restoreTabsInternal
    isRestoring = false

    // Always make sure there is at least one tab.
    let isPrivate =
      privateBrowsingManager.isPrivateBrowsing || Preferences.Privacy.privateBrowsingOnly.value
    return tabToSelect ?? self.addTab(isPrivate: isPrivate)
  }

  func restoreDeletedTabs(_ savedTabs: [Tab]) {
    isRestoring = true
    for tab in savedTabs {
      allTabs.append(tab)
      tab.navigationDelegate = self.navDelegate
      for delegate in delegates {
        delegate.get()?.tabManager(self, didAddTab: tab)
      }
    }
    isRestoring = false
  }

  // MARK: - P3A

  private func recordTabCountP3A() {
    // Q7 How many open tabs do you have?
    let count = allTabs.count
    UmaHistogramRecordValueToBucket(
      "Brave.Core.TabCount",
      buckets: [
        .r(0...1),
        .r(2...5),
        .r(6...10),
        .r(11...50),
        .r(51...),
      ],
      value: count
    )
  }

  // MARK: - Recently Closed

  /// Function used to add the tab information to Recently Closed when it is removed
  /// - Parameter tab: The tab which is removed
  func addTabToRecentlyClosed(_ tab: Tab) {
    if let savedItem = createRecentlyClosedFromActiveTab(tab) {
      RecentlyClosed.insert(savedItem)
    }
  }

  /// Function to add all the tabs to recently closed before the list is removef entirely by Close All Tabs
  func addAllTabsToRecentlyClosed(isActiveTabIncluded: Bool) {
    var allRecentlyClosed: [SavedRecentlyClosed] = []

    for tab in tabs(withType: .regular) {
      // Do not include the active tab for case isActiveTabIncluded is false
      if !isActiveTabIncluded, let currentTab = selectedTab, currentTab.id == tab.id {
        continue
      }

      if let savedItem = createRecentlyClosedFromActiveTab(tab) {
        allRecentlyClosed.append(savedItem)
      }
    }

    RecentlyClosed.insertAll(allRecentlyClosed)
  }

  /// Function invoked when a Recently Closed item is selected
  /// This function adss a new tab, populates this tab with necessary information
  /// Also executes restore function on this tab to load History Snapshot to the webview
  /// - Parameter recentlyClosed: Recently Closed item to be processed
  @MainActor func addAndSelectRecentlyClosed(_ recentlyClosed: RecentlyClosed) {
    guard let url = NSURL(idnString: recentlyClosed.url) as? URL ?? URL(string: recentlyClosed.url)
    else { return }

    // The NTP shold be removed if last recently close opened using empty tab
    if let currentTab = selectedTab,
      let currentTabURL = currentTab.url,
      InternalURL(currentTabURL)?.isAboutHomeURL == true
    {
      removeTab(currentTab)
    }

    let tab = addTab(URLRequest(url: url), isPrivate: false)
    guard let webView = tab.webView else { return }

    if let interactionState = recentlyClosed.interactionState, !interactionState.isEmpty {
      tab.navigationDelegate = navDelegate
      tab.restore(webView, restorationData: (recentlyClosed.title ?? "", interactionState))
    }

    selectTab(tab)
  }

  /// Function used to auto delete outdated Recently Closed Tabs
  func deleteOutdatedRecentlyClosed() {
    // The time interval to remove Recently Closed 3 days
    RecentlyClosed.deleteAll(olderThan: 3.days)
  }

  /// An internal function to create a RecentlyClosed item from a tab
  /// Also handles the backforward list transfer
  /// - Parameter tab: Tab to be converted to Recently Closed
  /// - Returns: Recently Closed item
  private func createRecentlyClosedFromActiveTab(_ tab: Tab) -> SavedRecentlyClosed? {
    // Private Tabs can not be added to Recently Closed
    if tab.isPrivate {
      return nil
    }

    // NTP should not be passed as Recently Closed item
    let recentlyClosedURL = tab.url ?? SessionTab.from(tabId: tab.id)?.url

    guard let tabUrl = recentlyClosedURL, tabUrl.isWebPage() else {
      return nil
    }

    // Convert any internal URLs to their real URL for the Recently Closed item
    var fetchedTabURL = tabUrl
    if let url = InternalURL(fetchedTabURL),
      let actualURL = url.extractedUrlParam ?? url.originalURLFromErrorPage
    {
      fetchedTabURL = actualURL
    }

    return SavedRecentlyClosed(
      url: fetchedTabURL,
      title: tab.displayTitle,
      interactionState: tab.webView?.sessionData,
      order: -1
    )
  }
}

extension TabManager: WKNavigationDelegate {

  // Note the main frame JSContext (i.e. document, window) is not available yet.
  func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
    if let tab = self[webView] {
      tab.contentBlocker.clearPageStats()
    }
  }

  // The main frame JSContext is available, and DOM parsing has begun.
  // Do not excute JS at this point that requires running prior to DOM parsing.
  func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    // only store changes if this is not an error page
    // as we current handle tab restore as error page redirects then this ensures that we don't
    // call storeChanges unnecessarily on startup

    if let url = webView.url {
      // tab restore uses internal pages,
      // so don't call storeChanges unnecessarily on startup
      if InternalURL(url)?.isSessionRestore == true {
        return
      }

      if let tab = tabForWebView(webView) {
        if Preferences.Privacy.privateBrowsingOnly.value
          || (tab.isPrivate && !Preferences.Privacy.persistentPrivateBrowsing.value)
        {
          return
        }

        preserveScreenshot(for: tab)
        saveTab(tab)
      }
    }
  }

  func tabForWebView(_ webView: WKWebView) -> Tab? {
    objc_sync_enter(self)
    defer { objc_sync_exit(self) }

    return allTabs.first(where: { $0.webView === webView })
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
  }

  /// Called when the WKWebView's content process has gone away. If this happens for the currently selected tab
  /// then we immediately reload it.
  func webViewWebContentProcessDidTerminate(_ webView: WKWebView) {
    if let tab = selectedTab, tab.webView == webView {
      webView.reload()
    }
  }
}

// MARK: - TabManagerDelegate optional methods.
extension TabManagerDelegate {
  func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {}
  func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {}
  func tabManagerDidAddTabs(_ tabManager: TabManager) {}
  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {}
}

extension TabManager: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    switch key {
    case Preferences.General.blockPopups.key:
      let allowPopups = !Preferences.General.blockPopups.value
      // Each tab may have its own configuration, so we should tell each of them in turn.
      allTabs.forEach {
        $0.webView?.configuration.preferences.javaScriptCanOpenWindowsAutomatically = allowPopups
      }
      // The default tab configurations also need to change.
      configuration.preferences.javaScriptCanOpenWindowsAutomatically = allowPopups
    case Preferences.General.nightModeEnabled.key:
      DarkReaderScriptHandler.set(
        tabManager: self,
        enabled: Preferences.General.nightModeEnabled.value
      )
    default:
      break
    }
  }
}

extension TabManager: NSFetchedResultsControllerDelegate {
  func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {
    if let domain = anObject as? Domain, let domainURL = domain.url {
      // if `wallet_permittedAccounts` changes on a `Domain` from
      // wallet settings / manage web3 site connections, we need to
      // fire `accountsChanged` event on open tabs for this `Domain`
      let tabsForDomain = self.allTabs.filter {
        $0.url?.domainURL.absoluteString.caseInsensitiveCompare(domainURL) == .orderedSame
      }
      tabsForDomain.forEach { tab in
        Task { @MainActor in
          let privateMode = privateBrowsingManager.isPrivateBrowsing
          guard let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: privateMode)
          else {
            return
          }
          let allAccounts = await keyringService.allAccounts()
          // iOS does not have `HostContentSettingsMap`, so we must
          // implement `SolanaProviderImpl::OnContentSettingChanged`
          if let selectedSolAccount = allAccounts.solDappSelectedAccount,
            tab.isSolanaAccountConnected(selectedSolAccount.address),  // currently connected
            !tab.isAccountAllowed(.sol, account: selectedSolAccount.address)
          {  // user revoked access
            tab.walletSolProvider?.disconnect()
          }

          let ethAccountAddressess = allAccounts.accounts.filter { $0.coin == .eth }.map(\.address)
          let allowedEthAccountAddresses =
            tab.getAllowedAccounts(.eth, accounts: ethAccountAddressess) ?? []
          tab.accountsChangedEvent(accounts: Array(allowedEthAccountAddresses))
        }
      }
    }
  }
}

extension WKWebsiteDataStore {
  @MainActor fileprivate func deleteDataRecords(forDomain domain: String) async {
    let records = await dataRecords(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypes()
    )
    let websiteRecords = records.filter { record in
      record.displayName == domain
    }

    await removeData(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
      for: websiteRecords
    )
  }
}
