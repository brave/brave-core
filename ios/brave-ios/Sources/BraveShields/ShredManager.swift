// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import Favicon
import Foundation
import Preferences
import Shared
import Web
import WebKit
import os.log

/// The tabs of a window, which `ShredManager` closes when their site is shredded.
public protocol ShredTabContainer: AnyObject {
  var windowId: UUID { get }
  @MainActor var isPrivateBrowsing: Bool { get }
  @MainActor var allTabs: [any TabState] { get }
  /// Removes `tabs`, deleting their web views first.
  @MainActor @discardableResult func removeTabsToShred(_ tabs: [any TabState]) -> [any TabState]
  @MainActor func removeAllTabsToShred(isPrivate: Bool)
  @MainActor func websiteDataStore(isPrivate: Bool) -> WKWebsiteDataStore
}

private struct WeakShredTabContainer {
  weak var value: (any ShredTabContainer)?
}

/// Shreds website data and closes the tabs of shredded sites across every window.
///
/// Website data stores are shared between windows, so a single instance per profile is shared by
/// every window, including the pending Auto Shred tasks for sites whose tabs were closed.
public class ShredManager {
  private let braveCore: BraveProfileController?
  private let profile: any Profile
  private let connectedWindowIdsProvider: @MainActor () -> [UUID]
  private let dormantWindowIdsProvider: @MainActor () -> [UUID]
  private let isShredFeatureEnabled: Bool
  private let siteClosedShredDelay: TimeInterval
  private var registeredContainers: [WeakShredTabContainer] = []
  /// The pending Auto Shred tasks for closed sites, keyed by whether they are private, then by
  /// base domain.
  private(set) var forgetTasks: [Bool: [String: Task<Void, Error>]] = [:]

  /// - Parameters:
  ///   - connectedWindowIds: The ids of windows whose scene is connected.
  ///   - dormantWindowIds: The ids of windows whose scene is dormant, e.g. a window on a now
  ///   disconnected external display.
  ///   - siteClosedShredDelay: How long after a site is closed to wait before forgetting its
  ///   data, allowing the user to go back to it.
  public init(
    braveCore: BraveProfileController?,
    profile: any Profile,
    connectedWindowIds: @escaping @MainActor () -> [UUID],
    dormantWindowIds: @escaping @MainActor () -> [UUID],
    isShredFeatureEnabled: Bool = FeatureList.kBraveShredFeature.enabled,
    siteClosedShredDelay: TimeInterval = 30
  ) {
    self.braveCore = braveCore
    self.profile = profile
    self.connectedWindowIdsProvider = connectedWindowIds
    self.dormantWindowIdsProvider = dormantWindowIds
    self.isShredFeatureEnabled = isShredFeatureEnabled
    self.siteClosedShredDelay = siteClosedShredDelay
  }

  /// Registers the tabs of a window. `container` is held weakly, and only considered while its
  /// window's scene is connected.
  public func register(_ container: some ShredTabContainer) {
    registeredContainers.removeAll { $0.value == nil }
    registeredContainers.append(WeakShredTabContainer(value: container))
  }

  // MARK: - Site Events

  /// Notifies that `url` is no longer displayed in `tab`, either because the tab was closed or
  /// because it navigated away to another site.
  ///
  /// Sites set to Auto Shred when the site is closed are forgotten after a short delay, which is
  /// cancelled by `navigated(from:to:in:)` if the user goes back to the site.
  @MainActor public func siteClosed(_ url: URL, in tab: some TabState) {
    guard isShredFeatureEnabled else { return }
    guard let url = url.urlToShred,
      let baseDomain = url.baseDomain
    else { return }
    forgetTasks[tab.isPrivate]?[baseDomain]?.cancel()
    let shredLevel =
      tab.braveShieldsHelper?.shredLevel(
        for: url,
        considerAllShieldsOption: true
      ) ?? .never

    switch shredLevel {
    case .never:
      return
    case .appExit:
      // Will be Shred on startup at next launch in `forgetDataWithPendingShredOnStartup()`.
      return
    case .whenSiteClosed:
      let openBaseDomains = openBaseDomainsInAllWindows(isPrivate: tab.isPrivate, excluding: tab)
      guard !openBaseDomains.contains(baseDomain) else { return }
      // If the app terminates before this delayed task runs, it is recovered on startup at the
      // next launch in `forgetDataWithPendingShredOnStartup()`.
      forgetDataDelayed(for: url, in: tab, delay: siteClosedShredDelay)
    }
  }

  /// Notifies that `tab` is navigating its main frame from `previousURL` to `url`.
  ///
  /// Cancels any pending Auto Shred for the site being opened, and treats the previous site as
  /// closed in `tab` if the navigation leaves it.
  @MainActor public func navigated(
    from previousURL: URL?,
    to url: URL,
    in tab: some TabState
  ) {
    siteOpened(url, in: tab)
    guard let previousURL, !InternalURL.isValid(url: previousURL),
      let previousBaseDomain = previousURL.baseDomain,
      url.baseDomain != previousBaseDomain
    else { return }
    siteClosed(previousURL, in: tab)
  }

  /// Cancels any pending Auto Shred for `url`, as it is being displayed in `tab`.
  @MainActor private func siteOpened(_ url: URL, in tab: some TabState) {
    guard let baseDomain = url.urlToShred?.baseDomain else { return }
    forgetTasks[tab.isPrivate]?[baseDomain]?.cancel()
    forgetTasks[tab.isPrivate]?.removeValue(forKey: baseDomain)
  }

  /// Forget all data for websites set to Auto Shred when the site is closed whose tabs were open
  /// in the given discarded windows.
  ///
  /// Tabs in a discarded window are closed without going through `removeTab`. Must be called
  /// before the windows' persisted tabs are deleted.
  @MainActor public func forgetDataForDiscardedWindows(_ windowIds: [UUID]) {
    // With no connected window, the next window to restore picks these sites up in
    // `forgetDataWithPendingShredOnStartup()`.
    guard isShredFeatureEnabled, !windowIds.isEmpty,
      let anyContainer = connectedTabContainers.first
    else { return }
    for isPrivate in [false, true] {
      let urls = SessionTab.allURLs(isPrivate: isPrivate, inWindows: windowIds)
      guard !urls.isEmpty else { continue }
      let openBaseDomains = openBaseDomainsInAllWindows(
        isPrivate: isPrivate,
        excludingWindows: windowIds
      )
      var checkedBaseDomains: Set<String> = []
      let urlsToShred = urls.compactMap(\.urlToShred).filter { url in
        guard let baseDomain = url.baseDomain, !openBaseDomains.contains(baseDomain),
          checkedBaseDomains.insert(baseDomain).inserted
        else { return false }
        return shredLevel(for: url, isPrivate: isPrivate) == .whenSiteClosed
      }
      guard !urlsToShred.isEmpty else { continue }
      let dataStore = anyContainer.websiteDataStore(isPrivate: isPrivate)
      Task {
        await forgetData(for: urlsToShred, dataStore: dataStore)
      }
    }
  }

  /// Forget all data for websites with a pending Auto Shred.
  /// Will forget all data instantly with no delay.
  ///
  /// This covers websites set to Auto Shred on app exit, as well as websites set to Auto Shred
  /// when the site is closed that have no open tab. The latter is required because the delayed
  /// task started in `siteClosed(_:in:)` does not survive app termination.
  ///
  /// Must be called after tabs are restored so that open tabs are taken into account.
  public func forgetDataWithPendingShredOnStartup() {
    guard isShredFeatureEnabled else { return }
    Task { @MainActor in
      // Private tabs use a non-persistent data store, so only regular tabs can keep a website's
      // persistent data alive across launches.
      let openBaseDomains = openBaseDomainsInAllWindows(isPrivate: false)
      let isSiteClosed: (URL) -> Bool = { url in
        guard let baseDomain = url.urlToShred?.baseDomain else { return false }
        return !openBaseDomains.contains(baseDomain)
      }

      var urlsToShred: Set<URL> = []
      if FeatureList.kBraveShieldsContentSettings.enabled {
        guard let braveShieldsSettings = BraveShieldsSettingsServiceFactory.get(profile: profile)
        else { return }
        // Auto Shred settings are per domain, but history can hold many URLs per domain, so
        // cache the lookups.
        var shredLevelCache: [String: SiteShredLevel] = [:]
        func shouldShred(_ url: URL, considerAllShieldsOption: Bool) -> Bool {
          let cacheKey = "\(url.domainURL.absoluteString)\(considerAllShieldsOption)"
          let shredLevel: SiteShredLevel
          if let cachedShredLevel = shredLevelCache[cacheKey] {
            shredLevel = cachedShredLevel
          } else {
            shredLevel =
              braveShieldsSettings.autoShredMode(
                for: url,
                considerAllShieldsOption: considerAllShieldsOption
              ).siteShredLevel
            shredLevelCache[cacheKey] = shredLevel
          }
          switch shredLevel {
          case .never:
            return false
          case .appExit:
            return true
          case .whenSiteClosed:
            return isSiteClosed(url)
          }
        }

        // iterate over WKWebsiteDataStore data records
        let dataRecords = await WKWebsiteDataStore.default().dataRecords(
          ofTypes: WKWebsiteDataStore.allWebsiteDataTypesIncludingPrivate()
        )
        for record in dataRecords {
          if let url = URL(string: "https://" + record.displayName),
            shouldShred(url, considerAllShieldsOption: true)
          {
            urlsToShred.insert(url)
          }
        }
        if Preferences.Shields.shredHistoryItems.value {
          // if user enabled shred and/or shred history but does not have data
          // in WKWebsiteDataStore, we still need to shred it.
          if let historyNodes = await braveCore?.historyAPI.search(
            withQuery: nil,
            options: HistorySearchOptions(
              maxCount: 0,
              hostnameSuffix: nil,
              duplicateHandling: .removeAll,
              begin: nil,
              end: nil
            )
          ) {
            for node in historyNodes
            where shouldShred(node.url, considerAllShieldsOption: false) {
              urlsToShred.insert(node.url)
            }
          }
          // Similar to history above for Recently Closed tabs
          for tab in RecentlyClosed.all() {
            if let url = URL(string: tab.url), shouldShred(url, considerAllShieldsOption: false) {
              urlsToShred.insert(url)
            }
          }
        }
      } else {  // kBraveShieldsContentSettings disabled
        let globalShredLevel = Preferences.Shields.shredLevel
        urlsToShred = Set(
          await Domain.allURLsWithShredLevel(
            rawShredLevel: SiteShredLevel.appExit.rawValue,
            isGlobalShredLevel: globalShredLevel == .appExit
          )
        )
        let whenSiteClosedURLs = await Domain.allURLsWithShredLevel(
          rawShredLevel: SiteShredLevel.whenSiteClosed.rawValue,
          isGlobalShredLevel: globalShredLevel == .whenSiteClosed
        )
        urlsToShred.formUnion(whenSiteClosedURLs.filter(isSiteClosed))
      }
      guard !urlsToShred.isEmpty else { return }
      await forgetData(for: Array(urlsToShred))
    }
  }

  // MARK: - Manual Shred

  /// Shreds the site displayed at `url`, closing its tabs in every window.
  ///
  /// - Returns: The task forgetting the site's data.
  @discardableResult
  @MainActor public func shredData(for url: URL, in tab: some TabState) -> Task<Void, Never>? {
    guard let url = url.urlToShred,
      let baseDomain = url.baseDomain
    else { return nil }

    closeTabsToShredInAllWindows(isPrivate: tab.isPrivate) {
      $0.urlToShred?.baseDomain == baseDomain
    }

    return Task {
      await forgetData(for: url, in: tab)
    }
  }

  /// Shreds the sites displayed in `tabs`, closing their tabs in every window.
  ///
  /// - Returns: The tabs closed in `container`.
  @MainActor public func shredData(
    for tabs: [any TabState],
    in container: any ShredTabContainer
  ) -> Set<TabState.ID> {
    let isPrivate = container.isPrivateBrowsing
    let urlsToShred = Set(tabs.compactMap(\.visibleURL?.urlToShred))
    let shouldClose: (URL) -> Bool = { $0.urlToShred.map(urlsToShred.contains) ?? false }
    let closedTabs = closeTabsToShred(in: container, isPrivate: isPrivate, where: shouldClose)
    closeTabsToShredInAllWindows(isPrivate: isPrivate, excluding: container, where: shouldClose)
    let dataStore = container.websiteDataStore(isPrivate: isPrivate)
    Task {
      await forgetData(for: Array(urlsToShred), dataStore: dataStore)
    }
    return Set(closedTabs.map(\.id))
  }

  /// Shreds the sites of every tab in `container` for its current mode, closing all of its tabs
  /// and the tabs of those sites in every other window.
  ///
  /// - Returns: The task forgetting the sites' data.
  @discardableResult
  @MainActor public func shredAllTabs(in container: any ShredTabContainer) -> Task<Void, Never> {
    let isPrivate = container.isPrivateBrowsing
    let urlsToShred = Set(
      container.tabs(isPrivate: isPrivate).compactMap(\.visibleURL?.urlToShred)
    )
    let baseDomainsToShred = Set(urlsToShred.compactMap(\.baseDomain))
    container.removeAllTabsToShred(isPrivate: isPrivate)
    closeTabsToShredInAllWindows(isPrivate: isPrivate, excluding: container) {
      guard let baseDomain = $0.urlToShred?.baseDomain else { return false }
      return baseDomainsToShred.contains(baseDomain)
    }
    let dataStore = container.websiteDataStore(isPrivate: isPrivate)
    return Task {
      await forgetData(for: Array(urlsToShred), dataStore: dataStore)
    }
  }

  // MARK: - Shred Level

  /// The Auto Shred level for `url`, for when no `TabState` is available to ask its
  /// `BraveShieldsTabHelper`.
  @MainActor public func shredLevel(for url: URL, isPrivate: Bool) -> SiteShredLevel {
    if FeatureList.kBraveShieldsContentSettings.enabled {
      let profile = isPrivate ? profile.offTheRecordProfile : profile
      return BraveShieldsSettingsServiceFactory.get(profile: profile)?.autoShredMode(
        for: url,
        considerAllShieldsOption: true
      ).siteShredLevel ?? .never
    }
    // Deprecated access until `kBraveShieldsContentSettings` feature flag is removed.
    return Domain.getOrCreate(forUrl: url, persistent: !isPrivate).shredLevel
  }

  // MARK: - Windows

  @MainActor private var connectedTabContainers: [any ShredTabContainer] {
    let connectedWindowIds = Set(connectedWindowIdsProvider())
    return registeredContainers.compactMap(\.value).filter {
      connectedWindowIds.contains($0.windowId)
    }
  }

  @MainActor private var dormantWindowIds: [UUID] {
    dormantWindowIdsProvider()
  }

  /// The base domains with an open tab in any window, including windows whose
  /// scene is dormant.
  ///
  /// In-memory tabs are authoritative for a connected window, but a dormant
  /// window has none, so its persisted session is read instead (this can occur
  /// when window was on a now disconnected external display).
  /// - Parameter excluding: A tab to disregard, for callers asking whether a site
  /// remains open elsewhere once this tab is gone.
  /// - Parameter excludedWindowIds: Dormant windows to disregard, e.g. windows being discarded.
  @MainActor private func openBaseDomainsInAllWindows(
    isPrivate: Bool,
    excluding excludedTab: (any TabState)? = nil,
    excludingWindows excludedWindowIds: [UUID] = []
  ) -> Set<String> {
    let openURLs =
      connectedTabContainers
      .flatMap { $0.tabs(isPrivate: isPrivate) }
      .filter { $0 !== excludedTab }
      .compactMap { $0.visibleURL }
    let dormantWindowIds = dormantWindowIds.filter { !excludedWindowIds.contains($0) }
    let dormantURLs =
      dormantWindowIds.isEmpty
      ? [] : SessionTab.allURLs(isPrivate: isPrivate, inWindows: dormantWindowIds)
    return Set((openURLs + dormantURLs).compactMap { $0.urlToShred?.baseDomain })
  }

  @MainActor private func isTabOpenInAnyWindow(_ tab: any TabState) -> Bool {
    connectedTabContainers.contains { container in
      container.allTabs.contains { $0 === tab }
    }
  }

  /// Closes the tabs in `container` whose URL matches `shouldClose`.
  @discardableResult
  @MainActor private func closeTabsToShred(
    in container: any ShredTabContainer,
    isPrivate: Bool,
    where shouldClose: (URL) -> Bool
  ) -> [any TabState] {
    let tabsToClose = container.tabs(isPrivate: isPrivate).filter {
      $0.visibleURL.map(shouldClose) ?? false
    }
    return container.removeTabsToShred(tabsToClose)
  }

  /// Closes the tabs whose URL matches `shouldClose` in every window, including windows whose
  /// scene is dormant.
  ///
  /// Website data stores are shared between windows, so a shred would otherwise leave a live tab
  /// in another window pointing at wiped data, free to write more of it. A dormant window has no
  /// live tabs, but would restore the shredded site when reconnected, so its persisted tabs are
  /// deleted instead.
  @MainActor private func closeTabsToShredInAllWindows(
    isPrivate: Bool,
    excluding excludedContainer: (any ShredTabContainer)? = nil,
    where shouldClose: @escaping (URL) -> Bool
  ) {
    for container in connectedTabContainers where container !== excludedContainer {
      closeTabsToShred(in: container, isPrivate: isPrivate, where: shouldClose)
    }
    let dormantWindowIds = dormantWindowIds
    if !dormantWindowIds.isEmpty {
      SessionTab.deleteAll(isPrivate: isPrivate, inWindows: dormantWindowIds, where: shouldClose)
    }
  }

  // MARK: - Forget Data

  /// Start a task to delete all data for this url
  /// The task may be delayed in case we want to cancel it
  @MainActor private func forgetDataDelayed(
    for url: URL,
    in tab: some TabState,
    delay: TimeInterval
  ) {
    guard let url = url.urlToShred,
      let baseDomain = url.baseDomain
    else { return }
    let isPrivate = tab.isPrivate
    let dataStore = tab.configuration?.websiteDataStore
    forgetTasks[isPrivate] = forgetTasks[isPrivate] ?? [:]
    forgetTasks[isPrivate]?[baseDomain] = Task { [weak tab] in
      try await Task.sleep(seconds: delay)
      // A closed tab's live WebView keeps writing Local Storage after it is deleted. The tab may
      // instead still be open, having navigated away from the site, so only delete closed ones.
      if let tab, !isTabOpenInAnyWindow(tab) {
        tab.deleteWebView()
      }
      await self.forgetData(for: [url], dataStore: dataStore)
      self.forgetTasks[isPrivate]?.removeValue(forKey: baseDomain)
    }
  }

  @MainActor private func forgetData(for url: URL, in tab: (any TabState)?) async {
    await forgetData(for: [url], dataStore: tab?.configuration?.websiteDataStore)

    Logger.module.debug("Cleared website data for `\(url.baseDomain ?? "")`")
    if let baseDomain = url.baseDomain, let tab {
      forgetTasks[tab.isPrivate]?.removeValue(forKey: baseDomain)
    }
  }

  @MainActor private func forgetData(for urls: [URL], dataStore: WKWebsiteDataStore? = nil) async {
    let urls = urls.compactMap(\.urlToShred)
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
    if let historyAPI = braveCore?.historyAPI, Preferences.Shields.shredHistoryItems.value {
      // if we're only forgetting 1 site, we can query history by it's domain
      let query = urls.count == 1 ? urls.first?.baseDomain : nil

      let nodes = await historyAPI.search(
        withQuery: query,
        options: HistorySearchOptions(
          maxCount: 0,
          hostnameSuffix: nil,
          duplicateHandling: .keepAll,
          begin: nil,
          end: nil
        )
      ).filter { node in
        guard let baseDomain = node.url.baseDomain else { return false }
        return baseDomains.contains(baseDomain)
      }
      historyAPI.removeHistory(for: nodes)
    }

    if Preferences.Shields.shredHistoryItems.value {
      RecentlyClosed.remove(baseDomains: baseDomains)
      RecentSearch.remove(baseDomains: baseDomains)
    }

    for url in urls {
      await FaviconFetcher.deleteCache(for: url)
    }
  }
}

extension ShredTabContainer {
  @MainActor fileprivate func tabs(isPrivate: Bool) -> [any TabState] {
    allTabs.filter { $0.isPrivate == isPrivate }
  }
}

extension WKWebsiteDataStore {
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
}

extension WKWebsiteDataStore {
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
    return types
  }
}
