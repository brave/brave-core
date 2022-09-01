// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

// MARK: UISearchResultUpdating

extension TabTrayController: UISearchResultsUpdating {

  func updateSearchResults(for searchController: UISearchController) {
    guard let query = searchController.searchBar.text else { return }
    
    invalidateSearchTimer()
    
    searchTabTrayTimer = Timer.scheduledTimer(
      timeInterval: 0.1,
      target: self,
      selector: #selector(fetchSearchResults(timer:)),
      userInfo: query,
      repeats: false)
  }

  @objc private func fetchSearchResults(timer: Timer) {
    guard let query = timer.userInfo as? String else {
      tabTraySearchQuery = ""
      return
    }
    
    tabTraySearchQuery = query
    
    refreshDataSource()
  }

  private func invalidateSearchTimer() {
    searchTabTrayTimer?.invalidate()
    searchTabTrayTimer = nil
  }
}

// MARK: UISearchControllerDelegate

extension TabTrayController: UISearchControllerDelegate {

  func willPresentSearchController(_ searchController: UISearchController) {
    isTabTrayBeingSearched = true
    tabTraySearchQuery = nil
    
    switch tabTrayMode {
    case .local:
      tabTrayView.collectionView.reloadData()
    case .sync:
      tabSyncView.tableView.reloadData()
    }
  }

  func willDismissSearchController(_ searchController: UISearchController) {
    invalidateSearchTimer()
    isTabTrayBeingSearched = false
    
    switch tabTrayMode {
    case .local:
      tabTrayView.collectionView.reloadData()
    case .sync:
      reloadOpenTabsSession()
    }
  }
}
