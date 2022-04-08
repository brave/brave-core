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
      DispatchQueue.main.async {
        // Tab Fetch
        let fetchedTabs = self.tabManager.tabsForCurrentMode(for: query)
        let openTabSites = self.fetchSitesFromTabs(fetchedTabs)

        // Bookmarks Fetch
        self.bookmarkManager.byFrequency(query: query) { sites in
          let bookmarkSites = sites.map { Site(url: $0.url ?? "", title: $0.title ?? "", siteType: .bookmark) }

          // History Fetch
          self.historyAPI.byFrequency(query: query) { historyList in
            let historySites = historyList.map { Site(url: $0.url.absoluteString, title: $0.title ?? "", siteType: .history) }

            let result = Set<Site>(openTabSites + historySites + bookmarkSites)
            completion(result)
          }
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
        
      if PrivateBrowsingManager.shared.isPrivateBrowsing {
        if let url = tab.url, url.isWebPage(), !(InternalURL(url)?.isAboutHomeURL ?? false) {
          
          if let selectedTabID = tabManager.selectedTab?.id, let tabID = tab.id, selectedTabID == tabID {
            continue
          }
          
          tabList.append(Site(url: url.absoluteString, title: tab.displayTitle, siteType: .tab, tabID: tab.id))
        }
      } else {
        var tabURL: URL?
        
        if let url = tab.url {
          tabURL = url
        } else if let tabID = tab.id {
          let fetchedTab = TabMO.get(fromId: tabID)
          
          if let urlString = fetchedTab?.url, let url = URL(string: urlString) {
            tabURL = url
          }
        }
        
        if let url = tabURL, url.isWebPage(), !(InternalURL(url)?.isAboutHomeURL ?? false) {
          
          if let selectedTabID = tabManager.selectedTab?.id, let tabID = tab.id, selectedTabID == tabID {
            continue
          }
          
          tabList.append(Site(url: url.absoluteString, title: tab.title ?? tab.displayTitle, siteType: .tab, tabID: tab.id))
        }
      }
    }
        
    return tabList
  }
}
