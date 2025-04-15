// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import OrderedCollections
import Shared
import Storage
import Web

public struct Site: Hashable {
  public enum SiteType {
    case unknown, bookmark, history, tab
  }
  public var url: String
  public var title: String
  public var siteType: SiteType = .unknown
  public var tabID: String?
  var tileURL: URL {
    return URL(string: url)?.domainURL ?? URL(string: "about:blank")!
  }
}

class FrequencyQuery {

  private let historyAPI: BraveHistoryAPI
  private let bookmarkManager: BookmarkManager
  private let tabManager: TabManager

  private var task: Task<Void, Error>?
  private var historyCancellable: HistoryCancellable?

  init(historyAPI: BraveHistoryAPI, bookmarkManager: BookmarkManager, tabManager: TabManager) {
    self.historyAPI = historyAPI
    self.bookmarkManager = bookmarkManager
    self.tabManager = tabManager
  }

  deinit {
    task?.cancel()
    historyCancellable = nil
  }

  @MainActor
  private func fetchOpenTabs(containing query: String) async -> [Site] {
    return (try? fetchSitesFromTabs(tabManager.tabsForCurrentMode(for: query))) ?? []
  }

  @MainActor
  private func fetchBookmarks(containing query: String) async -> [Site] {
    return await withCheckedContinuation { continuation in
      self.bookmarkManager.byFrequency(query: query) { [weak self] sites in
        guard let self = self, let task = self.task, !task.isCancelled else {
          continuation.resume(returning: [])
          return
        }

        continuation.resume(
          returning: sites.map {
            Site(url: $0.url ?? "", title: $0.title ?? "", siteType: .bookmark)
          }
        )
      }
    }
  }

  @MainActor
  private func fetchHistories(containing query: String) async -> [Site] {
    return await withCheckedContinuation { continuation in
      self.historyCancellable = self.historyAPI.byFrequency(query: query) { [weak self] sites in
        guard let self = self, let task = self.task, !task.isCancelled else {
          continuation.resume(returning: [])
          return
        }

        continuation.resume(
          returning: sites.map {
            Site(url: $0.url.absoluteString, title: $0.title ?? "", siteType: .history)
          }
        )
      }
    }
  }

  public func sitesByFrequency(containing query: String, completion: @escaping (Set<Site>) -> Void)
  {
    task?.cancel()
    historyCancellable = nil

    task = Task { @MainActor [weak self] in
      guard let self = self else { return }

      try Task.checkCancellation()

      let fetches = await [
        self.fetchOpenTabs(containing: query),
        self.fetchBookmarks(containing: query),
        self.fetchHistories(containing: query),
      ]

      try Task.checkCancellation()
      self.task = nil
      self.historyCancellable = nil

      completion(Set(fetches.flatMap { $0 }))
    }
  }

  private func fetchSitesFromTabs(_ tabs: [any TabState]) throws -> [Site] {
    var tabList = [Site]()
    tabList.reserveCapacity(tabs.count)

    guard let selectedTab = tabManager.selectedTab else {
      return []
    }

    for tab in tabs {
      if let url = tab.visibleURL, url.isWebPage(), !(InternalURL(url)?.isAboutHomeURL ?? false) {
        if selectedTab.id == tab.id {
          continue
        }

        tabList.append(
          Site(
            url: url.absoluteString,
            title: tab.displayTitle,
            siteType: .tab,
            tabID: tab.id.uuidString
          )
        )
      }
    }

    return tabList
  }
}
