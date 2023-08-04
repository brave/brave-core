// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Storage
import Data
import BraveCore
import OrderedCollections

class FrequencyQuery {

  private let historyAPI: BraveHistoryAPI
  private let bookmarkManager: BookmarkManager
  private let tabManager: TabManager

  private let frequencyQueue = DispatchQueue(label: "frequency-query-queue")
  private var task: DispatchWorkItem?
    
  init(historyAPI: BraveHistoryAPI, bookmarkManager: BookmarkManager, tabManager: TabManager) {
    self.historyAPI = historyAPI
    self.bookmarkManager = bookmarkManager
    self.tabManager = tabManager
  }

  deinit {
    task?.cancel()
  }

  public func sitesByFrequency(containing query: String, completion: @escaping (Set<Site>) -> Void) {
    task?.cancel()

    task = DispatchWorkItem {
      // brave-core fetch can be slow over 200ms per call,
      // a cancellable serial queue is used for it.
      DispatchQueue.main.async { [weak self] in
        guard let self = self, let task = self.task, !task.isCancelled else {
          return
        }
        
        // Tab Fetch
        let fetchedTabs = self.tabManager.tabsForCurrentMode(for: query)
        let openTabSites = self.fetchSitesFromTabs(fetchedTabs)

        let group = DispatchGroup()
        group.enter()
        
        // Bookmarks Fetch
        var bookmarkSites = [Site]()
        self.bookmarkManager.byFrequency(query: query) { sites in
          bookmarkSites = sites.map { Site(url: $0.url ?? "", title: $0.title ?? "", siteType: .bookmark) }
          group.leave()
        }
        
        group.enter()

        // History Fetch
        var historySites = [Site]()
        self.historyAPI.byFrequency(query: query) { historyList in
          historySites = historyList.map { Site(url: $0.url.absoluteString, title: $0.title ?? "", siteType: .history) }
          group.leave()
        }
        
        group.notify(queue: .main) {
          self.task = nil
          completion(Set<Site>(openTabSites + bookmarkSites + historySites))
        }
      }
    }
        
    if let task = self.task {
      frequencyQueue.async(execute: task)
    }
  }
    
  private func fetchSitesFromTabs(_ tabs: [Tab]) -> [Site] {
    var tabList = [Site]()
        
    for tab in tabs {
      if tab.isPrivate {
        if let url = tab.url, url.isWebPage(), !(InternalURL(url)?.isAboutHomeURL ?? false) {
          if let selectedTabID = tabManager.selectedTab?.id, selectedTabID == tab.id {
            continue
          }
          
          tabList.append(Site(url: url.absoluteString, title: tab.displayTitle, siteType: .tab, tabID: tab.id.uuidString))
        }
      } else {
        let tabURL = tab.url ?? SessionTab.from(tabId: tab.id)?.url
        if let url = tabURL, url.isWebPage(), !(InternalURL(url)?.isAboutHomeURL ?? false) {
          if let selectedTabID = tabManager.selectedTab?.id, selectedTabID == tab.id {
            continue
          }
          
          tabList.append(Site(url: url.absoluteString, title: tab.displayTitle, siteType: .tab, tabID: tab.id.uuidString))
        }
      }
    }
        
    return tabList
  }
}
