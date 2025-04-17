// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveWallet
import CoreData
import Data
import Favicon
import Foundation
import Growth
import Preferences
import Shared
import Storage
import Web
import WebKit
import os.log

protocol TabManagerDelegate: AnyObject {
  func tabManager(
    _ tabManager: TabManager,
    didSelectedTabChange selected: (any TabState)?,
    previous: (any TabState)?
  )
  func tabManager(_ tabManager: TabManager, willAddTab tab: some TabState)
  func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState)
  func tabManager(_ tabManager: TabManager, willRemoveTab tab: some TabState)
  func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState)

  func tabManagerDidRestoreTabs(_ tabManager: TabManager)
  func tabManagerDidAddTabs(_ tabManager: TabManager)
  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?)
}

protocol TabManagerStateDelegate: AnyObject {
  func tabManagerWillStoreTabs(_ tabs: [any TabState])
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

  private(set) var allTabs = [any TabState]()
  private var _selectedIndex = -1
  private(set) var isRestoring = false
  private(set) var isBulkDeleting = false

  fileprivate let prefs: Prefs
  var selectedIndex: Int {
    return _selectedIndex
  }
  var normalTabSelectedIndex: Int = 0
  var privateTabSelectedIndex: Int = 0
  var tempTabs: [any TabState]?
  private weak var rewards: BraveRewards?
  private weak var tabGeneratorAPI: BraveTabGeneratorAPI?
  private var domainFrc = Domain.frc()
  private let syncedTabsQueue = DispatchQueue(label: "synced-tabs-queue")
  private var syncTabsTask: DispatchWorkItem?
  private var metricsHeartbeat: Timer?
  private let historyAPI: BraveHistoryAPI?
  public let privateBrowsingManager: PrivateBrowsingManager
  private var forgetTasks: [Bool: [String: Task<Void, Error>]] = [:]

  let windowId: UUID

  /// The property returning only existing tab is NTP for current mode
  var isBrowserEmptyForCurrentMode: Bool {
    guard tabsForCurrentMode.count == 1,
      let tabURL = tabsForCurrentMode.first?.visibleURL,
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
    self.rewards = rewards
    self.tabGeneratorAPI = tabGeneratorAPI
    self.historyAPI = historyAPI
    self.privateBrowsingManager = privateBrowsingManager
    super.init()

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

  var count: Int {
    assert(Thread.isMainThread)

    return allTabs.count
  }

  var selectedTab: (any TabState)? {
    assert(Thread.isMainThread)
    if !(0..<count ~= _selectedIndex) {
      return nil
    }

    return allTabs[_selectedIndex]
  }

  subscript(index: Int) -> (any TabState)? {
    assert(Thread.isMainThread)

    if index >= allTabs.count {
      return nil
    }
    return allTabs[index]
  }

  var currentDisplayedIndex: Int? {
    assert(Thread.isMainThread)

    guard let selectedTab = self.selectedTab else {
      return nil
    }

    return tabsForCurrentMode.firstIndex(where: { $0 === selectedTab })
  }

  // What the users sees displayed based on current private browsing mode
  var tabsForCurrentMode: [any TabState] {
    let isPrivate = privateBrowsingManager.isPrivateBrowsing
    return tabs(isPrivate: isPrivate)
  }

  var openedWebsitesCount: Int {
    tabsForCurrentMode.filter {
      if let url = $0.visibleURL {
        return url.isWebPage() && !(InternalURL(url)?.isAboutHomeURL ?? false)
      }
      return false
    }.count
  }

  subscript(id: TabState.ID) -> (any TabState)? {
    return allTabs.first(where: { $0.id == id })
  }

  func tabsForCurrentMode(for query: String? = nil) -> [any TabState] {
    if let query = query {
      let isPrivate = privateBrowsingManager.isPrivateBrowsing
      return tabs(isPrivate: isPrivate, query: query)
    } else {
      return tabsForCurrentMode
    }
  }

  func tabsCountForMode(isPrivate: Bool) -> Int {
    return tabs(isPrivate: isPrivate).count
  }

  private func tabs(isPrivate: Bool, query: String? = nil) -> [any TabState] {
    assert(Thread.isMainThread)

    let allTabs = allTabs.filter { $0.isPrivate == isPrivate }

    if let query = query, !query.isEmpty {
      // Display title is the only data that will be present on every situation
      return allTabs.filter {
        $0.displayTitle.lowercased().contains(query)
          || ($0.visibleURL?.baseDomain?.contains(query) ?? false)
      }
    } else {
      return allTabs
    }
  }

  /// Function for adding local tabs as synced sessions
  /// This is used when open tabs toggle is enabled in sync settings and browser constructor
  func addRegularTabsToSyncChain() {
    let regularTabs = tabs(isPrivate: false)

    syncTabsTask?.cancel()

    syncTabsTask = DispatchWorkItem {
      guard let task = self.syncTabsTask, !task.isCancelled else {
        return
      }

      for tab in regularTabs {
        if let url = tab.fetchedURL, !tab.isPrivate, !url.isLocal,
          !InternalURL.isValid(url: url), !url.isInternalURL(for: .readermode)
        {
          tab.browserData?.addTabInfoToSyncedSessions(url: url, displayTitle: tab.displayTitle)
        }
      }
    }

    if let task = self.syncTabsTask {
      DispatchQueue.main.async(execute: task)
    }
  }

  private static var defaultConfiguration = getNewConfiguration(isPrivate: false)
  private static var privateConfiguration = getNewConfiguration(isPrivate: true)

  private class func getNewConfiguration(isPrivate: Bool = false) -> WKWebViewConfiguration {
    let configuration: WKWebViewConfiguration = .init()
    configuration.processPool = WKProcessPool()
    configuration.preferences.javaScriptCanOpenWindowsAutomatically = !Preferences.General
      .blockPopups.value
    configuration.websiteDataStore = isPrivate ? sharedNonPersistentStore() : .default()

    // Dev note: Do NOT add `.link` to the list, it breaks interstitial pages
    // and pages that don't want the URL highlighted!
    configuration.dataDetectorTypes = [.phoneNumber]
    configuration.userContentController = WKUserContentController()
    configuration.preferences = WKPreferences()
    configuration.preferences.javaScriptCanOpenWindowsAutomatically = false
    configuration.preferences.isFraudulentWebsiteWarningEnabled =
      Preferences.Shields.googleSafeBrowsing.value
    configuration.allowsInlineMediaPlayback = true
    // Enables Zoom in website by ignoring their javascript based viewport Scale limits.
    configuration.ignoresViewportScaleLimits = true
    configuration.upgradeKnownHostsToHTTPS = ShieldPreferences.httpsUpgradeLevel.isEnabled

    if configuration.urlSchemeHandler(forURLScheme: InternalURL.scheme) == nil {
      configuration.setURLSchemeHandler(
        InternalSchemeHandler(),
        forURLScheme: InternalURL.scheme
      )
    }

    return configuration
  }

  func reset() {
    Self.defaultConfiguration = Self.getNewConfiguration(isPrivate: false)
    Self.privateConfiguration = Self.getNewConfiguration(isPrivate: true)
    for tab in allTabs {
      if tab.isWebViewCreated {
        tab.deleteWebView()
      }
    }
  }

  func clearTabHistory(_ completion: (() -> Void)? = nil) {
    allTabs.filter({ $0.isWebViewCreated }).forEach({
      $0.clearBackForwardList()
      SessionTab.update(
        tabId: $0.id,
        interactionState: $0.sessionData ?? Data(),
        title: $0.title ?? "",
        url: $0.visibleURL ?? TabManager.ntpInteralURL
      )
    })

    completion?()
  }

  func reloadSelectedTab() {
    let tab = selectedTab
    _selectedIndex = -1
    selectTab(tab)
    if let url = selectedTab?.visibleURL {
      selectedTab?.loadRequest(PrivilegedRequest(url: url) as URLRequest)
    }
  }

  func selectTab(_ tab: (any TabState)?, previous: (any TabState)? = nil) {
    assert(Thread.isMainThread)
    let previous = previous ?? selectedTab

    if previous === tab {
      return
    }
    // Convert the global mode to normal/private
    privateBrowsingManager.isPrivateBrowsing = tab?.isPrivate == true

    // Make sure to wipe the private tabs if the user has the pref turned on
    if tab?.isPrivate == false
      && (Preferences.Privacy.privateBrowsingOnly.value
        || !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      removeAllPrivateTabs()
    }

    if let tab = tab {
      _selectedIndex = allTabs.firstIndex(where: { $0 === tab }) ?? -1
    } else {
      _selectedIndex = -1
    }

    if let previousTab = previous {
      preserveScreenshot(for: previousTab)
    }

    if let t = selectedTab, !t.isWebViewCreated {
      selectedTab?.createWebView()
      restoreTab(t)
    }

    guard tab === selectedTab else {
      Logger.module.error(
        "Expected tab (\(tab?.visibleURL?.absoluteString ?? "nil")) is not selected. Selected index: \(self.selectedIndex)"
      )
      return
    }

    if let tabId = tab?.id {
      SessionTab.setSelected(tabId: tabId)
    }

    UIImpactFeedbackGenerator(style: .light).vibrate()
    selectedTab?.createWebView()

    if let selectedTab = selectedTab,
      selectedTab.visibleURL == nil
    {
      selectedTab.setVirtualURL(selectedTab.visibleURL ?? TabManager.ntpInteralURL)
      restoreTab(selectedTab)
      Logger.module.error("Force Restored a Zombie (any TabState)?!")
    }

    delegates.forEach { $0.get()?.tabManager(self, didSelectedTabChange: tab, previous: previous) }
    if let tab = previous {
      tab.isVisible = false
    }
    if let tab = selectedTab {
      tab.isVisible = true
    }

    if let tabID = tab?.id {
      SessionTab.touch(tabId: tabID)
    }

    guard let newSelectedTab = tab, let previousTab = previous,
      let newTabUrl = newSelectedTab.visibleURL,
      let previousTabUrl = previousTab.visibleURL
    else { return }

    if !privateBrowsingManager.isPrivateBrowsing {
      if previousTab.displayFavicon == nil {
        adsRewardsLog.warning("No favicon found in tab to report to rewards panel")
      }
      rewards?.maybeNotifyTabDidChange(
        tab: previousTab,
        isSelected: false
      )
      rewards?.reportTabUpdated(
        tab: previousTab,
        isSelected: false,
        isPrivate: previousTab.isPrivate
      )

      if newSelectedTab.displayFavicon == nil && !newTabUrl.isLocal {
        adsRewardsLog.warning("No favicon found in tab to report to rewards panel")
      }
      rewards?.maybeNotifyTabDidChange(
        tab: newSelectedTab,
        isSelected: true
      )
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

  @MainActor func addPopupForParentTab(
    _ parentTab: any TabState
  ) -> any TabState {
    let popup = TabStateFactory.create(
      with: .init(
        initialConfiguration: parentTab.configuration,
        braveCore: nil
      )
    )
    configureTab(
      popup,
      request: nil,
      afterTab: parentTab,
      flushToDisk: true,
      zombie: true,
      isPopup: true
    )
    return popup
  }

  @discardableResult
  @MainActor func addTabAndSelect(
    _ request: URLRequest! = nil,
    afterTab: (any TabState)? = nil,
    isPrivate: Bool
  ) -> any TabState {
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

    var tabs = [any TabState]()
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
      tab.setVirtualURL(url)
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
      guard let url = $0.visibleURL else { return }
      let request =
        InternalURL.isValid(url: url)
        ? PrivilegedRequest(url: url) as URLRequest : URLRequest(url: url)
      $0.createWebView()
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
    afterTab: (any TabState)? = nil,
    flushToDisk: Bool = true,
    zombie: Bool = false,
    id: UUID? = nil,
    lastActiveTime: Date? = nil,
    isPrivate: Bool
  ) -> any TabState {
    assert(Thread.isMainThread)

    let tabId = id ?? UUID()
    let initialConfiguration = isPrivate ? Self.privateConfiguration : Self.defaultConfiguration
    let tab = TabStateFactory.create(
      with: .init(
        id: tabId,
        initialConfiguration: initialConfiguration,
        lastActiveTime: lastActiveTime,
        braveCore: nil
      )
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

  func moveTab(_ tab: some TabState, toIndex visibleToIndex: Int) {
    assert(Thread.isMainThread)

    let currentTabs = tabs(isPrivate: tab.isPrivate)

    let toTab = currentTabs[visibleToIndex]

    guard let fromIndex = allTabs.firstIndex(where: { $0 === tab }),
      let toIndex = allTabs.firstIndex(where: { $0 === toTab })
    else {
      return
    }

    // Make sure to save the selected tab before updating the tabs list
    let previouslySelectedTab = selectedTab

    let tab = allTabs.remove(at: fromIndex)
    allTabs.insert(tab, at: toIndex)

    if let previouslySelectedTab = previouslySelectedTab,
      let previousSelectedIndex = allTabs.firstIndex(where: { $0 === previouslySelectedTab })
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
    _ tab: some TabState,
    request: URLRequest?,
    afterTab parent: (any TabState)? = nil,
    flushToDisk: Bool,
    zombie: Bool,
    isPopup: Bool = false
  ) {
    assert(Thread.isMainThread)
    var request = request
    let isPrivate = tab.isPrivate
    let isPersistentTab =
      !isPrivate
      || (isPrivate && !Preferences.Privacy.privateBrowsingOnly.value
        && Preferences.Privacy.persistentPrivateBrowsing.value)

    // WebKit can sometimes return a URL that isn't valid at all!
    // Do not allow configuring a tab with a Bookmarklet or Javascript URL
    if let requestURL = request?.url,
      NSURL(idnString: requestURL.absoluteString) == nil || requestURL.isBookmarklet
    {
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

    if parent == nil || parent?.isPrivate != tab.isPrivate {
      allTabs.append(tab)
    } else if let parent = parent, var insertIndex = allTabs.firstIndex(where: { $0 === parent }) {
      insertIndex += 1
      while insertIndex < allTabs.count && allTabs[insertIndex].isDescendentOf(parent) {
        insertIndex += 1
      }
      tab.opener = parent
      allTabs.insert(tab, at: insertIndex)
    }

    delegates.forEach { $0.get()?.tabManager(self, didAddTab: tab) }

    if !zombie {
      tab.createWebView()
    }

    if let request = request {
      tab.loadRequest(request)
      tab.setVirtualURL(request.url)
    } else if !isPopup {
      tab.loadRequest(PrivilegedRequest(url: TabManager.ntpInteralURL) as URLRequest)
      tab.setVirtualURL(TabManager.ntpInteralURL)
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

          tab.browserData?.resetExternalAlertProperties()
          self.preserveScreenshot(for: tab)
          self.saveTab(tab)
        }
      }
    }
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
      Preferences.Privacy.persistentPrivateBrowsing.value ? allTabs : tabs(isPrivate: false)
    SessionTab.updateAll(
      tabs: tabs.compactMap({
        if let sessionData = $0.sessionData {
          return ($0.id, sessionData, $0.title ?? "", $0.visibleURL ?? TabManager.ntpInteralURL)
        }
        return nil
      })
    )
  }

  func saveTab(_ tab: some TabState, saveOrder: Bool = false) {
    if Preferences.Privacy.privateBrowsingOnly.value
      || (tab.isPrivate && !Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      return
    }
    SessionTab.update(
      tabId: tab.id,
      interactionState: tab.sessionData ?? Data(),
      title: tab.title ?? "",
      url: tab.visibleURL ?? TabManager.ntpInteralURL
    )
    if saveOrder {
      saveTabOrder()
    }
  }

  /// Forget all data for websites that have forget me enabled
  /// Will forget all data instantly with no delay
  @MainActor func forgetDataOnAppExitDomains() {
    guard BraveCore.FeatureList.kBraveShredFeature.enabled else { return }
    let shredOnAppExitURLs = Domain.allDomainsWithShredLevelAppExit()?
      .compactMap { domain -> URL? in
        guard let urlString = domain.url,
          let url = URL(string: urlString),
          !InternalURL.isValid(url: url)
        else {
          return nil
        }
        return url
      }
    guard let shredOnAppExitURLs, !shredOnAppExitURLs.isEmpty else { return }
    Task {
      await forgetData(for: shredOnAppExitURLs)
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
    in tab: some TabState
  ) {
    guard FeatureList.kBraveShredFeature.enabled else { return }
    guard let url = url.urlToShred,
      let etldP1 = url.baseDomain
    else { return }
    forgetTasks[tab.isPrivate]?[etldP1]?.cancel()
    let siteDomain = Domain.getOrCreate(
      forUrl: url,
      persistent: !tab.isPrivate
    )

    switch siteDomain.shredLevel {
    case .never:
      return
    case .appExit:
      // Will be Shred on startup at next launch in `forgetDataOnAppExitDomains()`.
      return
    case .whenSiteClosed:
      let tabs = tabs(isPrivate: tab.isPrivate).filter { existingTab in
        existingTab !== tab
      }
      // Ensure that no othe tabs are open for this domain
      guard !tabs.contains(where: { $0.visibleURL?.urlToShred?.baseDomain == etldP1 }) else {
        return
      }
      forgetDataDelayed(for: url, in: tab, delay: 30)
    }
  }

  @MainActor func shredData(for url: URL, in tab: some TabState) {
    guard let url = url.urlToShred,
      let etldP1 = url.baseDomain
    else { return }

    // Select the next or previous tab that is not being destroyed
    if let index = allTabs.firstIndex(where: { $0 === tab }) {
      var nextTab: (any TabState)?
      // First seach down or up for a tab that is not being destroyed
      var increasingIndex = index + 1
      while nextTab == nil, increasingIndex < allTabs.count {
        if allTabs[increasingIndex].visibleURL?.urlToShred?.baseDomain != etldP1
          && allTabs[increasingIndex].isPrivate == tab.isPrivate
        {
          nextTab = allTabs[increasingIndex]
        }
        increasingIndex += 1
      }

      var decreasingIndex = index - 1
      while nextTab == nil, decreasingIndex > 0 {
        if allTabs[decreasingIndex].visibleURL?.urlToShred?.baseDomain != etldP1
          && allTabs[decreasingIndex].isPrivate == tab.isPrivate
        {
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
      guard tab.visibleURL?.urlToShred?.baseDomain == etldP1 else { continue }
      // The Tab's WebView is not deinitialized immediately, so it's possible the
      // WebView still stores data after we shred but before the WebView is deinitialized.
      // Delete the web view to prevent data being stored after data is Shred.
      tab.deleteWebView()
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
    in tab: some TabState,
    delay: TimeInterval
  ) {
    guard let url = url.urlToShred,
      let etldP1 = url.baseDomain
    else { return }
    forgetTasks[tab.isPrivate] = forgetTasks[tab.isPrivate] ?? [:]
    // Start a task to delete all data for this etldP1
    // The task may be delayed in case we want to cancel it
    forgetTasks[tab.isPrivate]?[etldP1] = Task {
      try await Task.sleep(seconds: delay)
      await self.forgetData(for: url, in: tab)
    }
  }

  @MainActor private func forgetData(for url: URL, in tab: (any TabState)?) async {
    await forgetData(for: [url], dataStore: tab?.configuration.websiteDataStore)

    ContentBlockerManager.log.debug("Cleared website data for `\(url.baseDomain ?? "")`")
    if let etldP1 = url.baseDomain, let tab {
      forgetTasks[tab.isPrivate]?.removeValue(forKey: etldP1)
    }
  }

  @MainActor private func forgetData(for urls: [URL], dataStore: WKWebsiteDataStore? = nil) async {
    var urls = urls.compactMap(\.urlToShred)
    let baseDomains = Set(urls.compactMap { $0.baseDomain })
    guard !baseDomains.isEmpty else { return }

    let dataStore = dataStore ?? WKWebsiteDataStore.default()
    // Delete 1P data records
    await dataStore.deleteDataRecords(
      forDomains: baseDomains
    )

    if BraveCore.FeatureList.kBraveShredCacheData.enabled {
      // Delete all cache data (otherwise 3P cache entries left behind
      // are visible in Manage Website Data view brave-browser #41095)
      let cacheTypes = Set([
        WKWebsiteDataTypeMemoryCache, WKWebsiteDataTypeDiskCache,
        WKWebsiteDataTypeOfflineWebApplicationCache,
      ])
      let cacheRecords = await dataStore.dataRecords(ofTypes: cacheTypes)
      await dataStore.removeData(ofTypes: cacheTypes, for: cacheRecords)
    }

    // Delete the history for forgotten websites
    if let historyAPI = self.historyAPI {
      // if we're only forgetting 1 site, we can query history by it's domain
      let query = urls.count == 1 ? urls.first?.baseDomain : nil

      let nodes = await withCheckedContinuation { continuation in
        var historyCancellable: HistoryCancellable?
        _ = historyCancellable
        historyCancellable = historyAPI.search(
          withQuery: query,
          options: HistorySearchOptions(
            maxCount: 0,
            hostOnly: false,
            duplicateHandling: .keepAll,
            begin: nil,
            end: nil
          ),
          completion: {
            historyCancellable = nil
            continuation.resume(returning: $0)
          }
        )
      }.filter { node in
        guard let baseDomain = node.url.baseDomain else { return false }
        return baseDomains.contains(baseDomain)
      }
      historyAPI.removeHistory(for: nodes)
    }

    RecentlyClosed.remove(baseDomains: baseDomains)

    for url in urls {
      await FaviconFetcher.deleteCache(for: url)
    }
  }

  /// Cancel a forget data request in case we navigate back to the tab within a certain period
  @MainActor func cancelForgetData(for url: URL, in tab: some TabState) {
    guard let etldP1 = url.urlToShred?.baseDomain else { return }
    forgetTasks[tab.isPrivate]?[etldP1]?.cancel()
    forgetTasks[tab.isPrivate]?.removeValue(forKey: etldP1)
  }

  @MainActor func removeTab(_ tab: any TabState) {
    guard let removalIndex = allTabs.firstIndex(where: { $0 === tab }) else {
      Logger.module.debug("Could not find index of tab to remove")
      return
    }

    if let url = tab.visibleURL {
      // Check if this data needs to be forgotten
      forgetDataIfNeeded(for: url, in: tab)
    }

    if tab.isPrivate {
      // Only when ALL tabs are dead, we clean up.
      // This is because other tabs share the same data-store.
      if tabsCountForMode(isPrivate: true) <= 1 {
        removeAllBrowsingDataForTab(tab)

        // After clearing the very last webview from the storage, give it a blank persistent store
        // This is the only way to guarantee that the last reference to the shared persistent store
        // reaches zero and destroys all its data.

        Self.nonPersistentDataStore = nil
        Self.privateConfiguration = Self.getNewConfiguration(isPrivate: true)
      }
    }

    let oldSelectedTab = selectedTab

    delegates.forEach { $0.get()?.tabManager(self, willRemoveTab: tab) }

    // The index of the tab in its respective tab grouping. Used to figure out which tab is next
    var currentTabs = tabs(isPrivate: tab.isPrivate)

    var tabIndex: Int = -1
    if let oldTab = oldSelectedTab {
      tabIndex = currentTabs.firstIndex(where: { $0 === oldTab }) ?? -1
    }

    let prevCount = count
    allTabs.remove(at: removalIndex)

    SessionTab.delete(tabId: tab.id)

    currentTabs = tabs(isPrivate: tab.isPrivate)

    // Let's select the tab to be selected next.
    if let oldTab = oldSelectedTab, tab !== oldTab {
      // If it wasn't the selected tab we removed, then keep it like that.
      // It might have changed index, so we look it up again.
      _selectedIndex = allTabs.firstIndex(where: { $0 === oldTab }) ?? -1
    } else if let parentTab = tab.opener,
      currentTabs.count > 1,
      let newTab = currentTabs.reduce(
        currentTabs.first,
        { currentBestTab, tab2 in
          if let tab1 = currentBestTab, let time1 = tab1.lastActiveTime {
            if let time2 = tab2.lastActiveTime {
              return time1 <= time2 ? tab2 : tab1
            }
            return tab1
          } else {
            return tab2
          }
        }
      ), parentTab === newTab, tab !== newTab, newTab.lastActiveTime != nil
    {
      // We select the most recently visited tab, only if it is also the parent tab of the closed tab.
      _selectedIndex = allTabs.firstIndex(where: { $0 === newTab }) ?? -1
    } else {
      // By now, we've just removed the selected one, and no previously loaded
      // tabs. So let's load the final one in the tab tray.
      if tabIndex == currentTabs.count {
        tabIndex -= 1
      }

      if let currentTab = currentTabs[safe: tabIndex] {
        _selectedIndex = allTabs.firstIndex(where: { $0 === currentTab }) ?? -1
      } else {
        _selectedIndex = -1
      }
    }

    assert(count == prevCount - 1, "Make sure the tab count was actually removed")

    delegates.forEach { $0.get()?.tabManager(self, didRemoveTab: tab) }

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

  private static var nonPersistentDataStore: WKWebsiteDataStore?
  static func sharedNonPersistentStore() -> WKWebsiteDataStore {
    if let dataStore = nonPersistentDataStore {
      return dataStore
    }

    let dataStore = WKWebsiteDataStore.nonPersistent()
    nonPersistentDataStore = dataStore
    return dataStore
  }

  /// Removes all private tabs from the manager without notifying delegates.
  private func removeAllPrivateTabs() {
    // reset the selectedTabIndex if we are on a private tab because we will be removing it.
    if selectedTab?.isPrivate == true {
      _selectedIndex = -1
    }

    allTabs.forEach { tab in
      if tab.isPrivate {
        tab.view.removeFromSuperview()
        removeAllBrowsingDataForTab(tab)
      }
    }

    Self.nonPersistentDataStore = nil

    allTabs = tabs(isPrivate: false)
  }

  func removeAllBrowsingDataForTab(
    _ tab: some TabState,
    completionHandler: @escaping () -> Void = {}
  ) {
    let dataTypes = WKWebsiteDataStore.allWebsiteDataTypes()
    tab.configuration.websiteDataStore.removeData(
      ofTypes: dataTypes,
      modifiedSince: Date.distantPast,
      completionHandler: completionHandler
    )
  }

  @MainActor func removeTabsWithUndoToast(_ tabs: [any TabState]) {
    isBulkDeleting = true
    tempTabs = tabs
    var tabsCopy = tabs

    // Remove the current tab last to prevent switching tabs while removing tabs
    if let selectedTab = selectedTab {
      if let selectedIndex = tabsCopy.firstIndex(where: { $0 === selectedTab }) {
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

    let tabsCopy = tabs(isPrivate: false)

    restoreDeletedTabs(tempTabs)
    self.isRestoring = true

    for tab in tempTabs {
      tab.showContent(true)
    }

    let tab = tempTabs.first
    if tab?.isPrivate == false {
      removeTabs(tabsCopy)
    }
    selectTab(tab)

    self.isRestoring = false
    delegates.forEach { $0.get()?.tabManagerDidRestoreTabs(self) }
    self.tempTabs?.removeAll()
    allTabs.first?.createWebView()
  }

  func eraseUndoCache() {
    tempTabs?.removeAll()
  }

  @MainActor func removeTabs(_ tabs: [any TabState]) {
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

  @MainActor func removeAllTabsForPrivateMode(isPrivate: Bool, isActiveTabIncluded: Bool = true) {
    isBulkDeleting = true

    var tabsToDelete = tabs(isPrivate: isPrivate)
    if !isActiveTabIncluded, let selectedTab,
      let index = tabsToDelete.firstIndex(where: { $0.id == selectedTab.id })
    {
      tabsToDelete.remove(at: index)
    }
    removeTabs(tabsToDelete)

    isBulkDeleting = false
    // No change needed here regarding to isActiveTabIncluded
    // Toast value is nil and TabsBarViewController is updating the from current tabs
    delegates.forEach { $0.get()?.tabManagerDidRemoveAllTabs(self, toast: nil) }
  }

  @MainActor func removeAllForCurrentMode(isActiveTabIncluded: Bool = true) {
    removeAllTabsForPrivateMode(
      isPrivate: privateBrowsingManager.isPrivateBrowsing,
      isActiveTabIncluded: isActiveTabIncluded
    )
  }

  func getIndex(_ tab: some TabState) -> Int? {
    assert(Thread.isMainThread)

    for i in 0..<count where allTabs[i] === tab {
      return i
    }

    assertionFailure("Tab not in tabs list")
    return nil
  }

  func getTabForURL(_ url: URL, isPrivate: Bool) -> (any TabState)? {
    assert(Thread.isMainThread)

    let tab = allTabs.filter {
      guard let tabURL = $0.visibleURL, $0.isPrivate else {
        return false
      }

      return tabURL.schemelessAbsoluteDisplayString == url.schemelessAbsoluteDisplayString
    }.first

    return tab
  }

  func getTabForID(_ id: UUID) -> (any TabState)? {
    assert(Thread.isMainThread)
    return allTabs.filter { $0.id == id }.first
  }

  func preserveScreenshot(for tab: some TabState) {
    assert(Thread.isMainThread)
    if isRestoring { return }

    guard let screenshot = tab.screenshot else { return }
    SessionTab.updateScreenshot(tabId: tab.id, screenshot: screenshot)
  }

  @MainActor fileprivate var restoreTabsInternal: (any TabState)? {
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

    /// Cache on if we should shred a given domain.
    var shouldShredDomainCache: [String: Bool] = [:]
    /// Checks if we should shred a Tab
    let shouldShredDomain: (_ tabURL: URL, _ isPrivate: Bool) -> Bool = { url, isPrivate in
      var shouldShredTab = false
      let cacheKey = "\(url.domainURL.absoluteString)\(isPrivate)"
      if let shouldShredDomain = shouldShredDomainCache[cacheKey] {
        shouldShredTab = shouldShredDomain
      } else {
        let siteDomain = Domain.getOrCreate(
          forUrl: url,
          persistent: !isPrivate
        )
        shouldShredTab = siteDomain.shredLevel.shredOnAppExit
        shouldShredDomainCache[cacheKey] = shouldShredTab
      }
      return shouldShredTab
    }

    var tabToSelect: (any TabState)?
    for savedTab in savedTabs {
      if let tabURL = savedTab.url {
        // Provide an empty request to prevent a new tab from loading the home screen
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        if FeatureList.kBraveShredFeature.enabled,
          shouldShredDomain(tabURL, savedTab.isPrivate)
        {
          // Delete SessionTab to prevent restore next launch
          SessionTab.delete(tabId: savedTab.tabId)
          // Don't restore this tab, it will be Shred after setup
          continue
        }

        let tab = addTab(
          request,
          flushToDisk: false,
          zombie: true,
          id: savedTab.tabId,
          isPrivate: savedTab.isPrivate
        )

        tab.lastTitle = savedTab.title
        tab.favicon = Favicon.default
        tab.browserData?.setScreenshot(savedTab.screenshot)

        Task { @MainActor in
          tab.favicon = try await FaviconFetcher.loadIcon(
            url: tabURL,
            kind: .smallIcon,
            persistent: !tab.isPrivate
          )
          tab.browserData?.setScreenshot(savedTab.screenshot)
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
        tab.browserData?.setScreenshot(savedTab.screenshot)

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

  func restoreTab(_ tab: some TabState) {
    guard let sessionTab = SessionTab.from(tabId: tab.id) else {
      if let tabURL = tab.visibleURL {
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        let sessionData = (tab.lastTitle ?? tabURL.absoluteDisplayString, request)

        tab.lastTitle = sessionData.0
        tab.loadRequest(sessionData.1)
      }
      return
    }

    // Tab was created with no active webview session data.
    // Restore tab data from Core-Data URL, and configure it.
    if sessionTab.interactionState.isEmpty {
      if let tabURL = sessionTab.url {
        let request =
          InternalURL.isValid(url: tabURL)
          ? PrivilegedRequest(url: tabURL) as URLRequest : URLRequest(url: tabURL)

        tab.lastTitle = tabURL.absoluteDisplayString
        tab.loadRequest(request)
      } else {
        tab.lastTitle = TabManager.aboutBlankBlockedURL.absoluteDisplayString
        tab.loadRequest(URLRequest(url: TabManager.aboutBlankBlockedURL))
      }
      return
    }

    tab.lastTitle = sessionTab.title
    tab.restore(using: sessionTab.interactionState)
  }

  /// Restores all tabs.
  /// Returns the tab that has to be selected after restoration.
  @MainActor var restoreAllTabs: any TabState {
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

  func restoreDeletedTabs(_ savedTabs: [any TabState]) {
    isRestoring = true
    for tab in savedTabs {
      allTabs.append(tab)
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
  func addTabToRecentlyClosed(_ tab: some TabState) {
    if let savedItem = createRecentlyClosedFromActiveTab(tab) {
      RecentlyClosed.insert(savedItem)
    }
  }

  /// Function to add all the tabs to recently closed before the list is removed entirely by Close All Tabs
  func addAllTabsToRecentlyClosed(isActiveTabIncluded: Bool) {
    var allRecentlyClosed: [SavedRecentlyClosed] = []

    for tab in tabs(isPrivate: false) {
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
      let currentTabURL = currentTab.visibleURL,
      InternalURL(currentTabURL)?.isAboutHomeURL == true
    {
      removeTab(currentTab)
    }

    let tab = addTab(URLRequest(url: url), isPrivate: false)

    if let interactionState = recentlyClosed.interactionState, !interactionState.isEmpty {
      tab.lastTitle = recentlyClosed.title ?? ""
      tab.restore(using: interactionState)
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
  /// - Parameter tab: some TabState to be converted to Recently Closed
  /// - Returns: Recently Closed item
  private func createRecentlyClosedFromActiveTab(_ tab: some TabState) -> SavedRecentlyClosed? {
    // Private Tabs can not be added to Recently Closed
    if tab.isPrivate {
      return nil
    }

    // NTP should not be passed as Recently Closed item
    let recentlyClosedURL = tab.visibleURL ?? SessionTab.from(tabId: tab.id)?.url

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
      interactionState: tab.sessionData,
      order: -1
    )
  }
}

// MARK: - TabManagerDelegate optional methods.
extension TabManagerDelegate {
  func tabManager(_ tabManager: TabManager, willAddTab tab: some TabState) {}
  func tabManager(_ tabManager: TabManager, willRemoveTab tab: some TabState) {}
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
        $0.configuration.preferences.javaScriptCanOpenWindowsAutomatically = allowPopups
      }
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
        $0.visibleURL?.domainURL.absoluteString.caseInsensitiveCompare(domainURL) == .orderedSame
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
            // currently connected
            tab.browserData?.isSolanaAccountConnected(selectedSolAccount.address) == true,
            tab.browserData?.isAccountAllowed(.sol, account: selectedSolAccount.address) == false
          {  // user revoked access
            tab.walletSolProvider?.disconnect()
          }

          let ethAccountAddressess = allAccounts.accounts.filter { $0.coin == .eth }.map(\.address)
          let allowedEthAccountAddresses =
            tab.browserData?.getAllowedAccounts(.eth, accounts: ethAccountAddressess) ?? []
          tab.browserData?.accountsChangedEvent(accounts: Array(allowedEthAccountAddresses))
        }
      }
    }
  }
}

extension WKWebsiteDataStore {
  @MainActor fileprivate func deleteDataRecords(forDomain domain: String) async {
    await deleteDataRecords(forDomains: Set([domain]))
  }

  @MainActor fileprivate func deleteDataRecords(forDomains domains: Set<String>) async {
    let records = await dataRecords(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypesIncludingPrivate()
    )
    let websiteRecords = records.filter { record in
      domains.contains(record.displayName)
    }

    await removeData(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypesIncludingPrivate(),
      for: websiteRecords
    )
  }

  /// This includes all public types from `WKWebsiteDataStore.allWebsiteDataTypes` as well as private data types.
  public static func allWebsiteDataTypesIncludingPrivate() -> Set<String> {
    // https://github.com/WebKit/WebKit/blob/b66e4895df40202b14bb20fb47444c3e0a3c164e/Source/WebKit/UIProcess/API/Cocoa/WKWebsiteDataRecordPrivate.h
    var types = WKWebsiteDataStore.allWebsiteDataTypes()
    types.insert("_WKWebsiteDataTypeHSTSCache")
    types.insert("_WKWebsiteDataTypeResourceLoadStatistics")
    types.insert("_WKWebsiteDataTypeCredentials")
    types.insert("_WKWebsiteDataTypeAdClickAttributions")
    types.insert("_WKWebsiteDataTypePrivateClickMeasurements")
    types.insert("_WKWebsiteDataTypeAlternativeServices")
    if #unavailable(iOS 17) {
      types.insert("_WKWebsiteDataTypeMediaKeys")
      types.insert("_WKWebsiteDataTypeSearchFieldRecentSearches")
    }
    return types
  }
}
