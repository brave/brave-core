// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import UIKit
import Web

extension TabDataValues {
  private struct PullToRefreshHelperKey: TabDataKey {
    static var defaultValue: PullToRefreshTabHelper?
  }
  var pullToRefresh: PullToRefreshTabHelper? {
    get { self[PullToRefreshHelperKey.self] }
    set { self[PullToRefreshHelperKey.self] = newValue }
  }
}

class PullToRefreshTabHelper: TabObserver, PreferencesObserver {
  private weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab

    tab.addObserver(self)
    Preferences.General.enablePullToRefresh.observe(from: self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  private func updatePullToRefreshVisibility() {
    guard let url = tab?.visibleURL, let scrollView = tab?.webViewProxy?.scrollView else { return }
    let isRefreshControlVisible =
      !url.isLocalUtility && Preferences.General.enablePullToRefresh.value
    if isRefreshControlVisible {
      if scrollView.refreshControl == nil {
        scrollView.refreshControl = UIRefreshControl(
          frame: .zero,
          primaryAction: .init(handler: { [weak tab] _ in
            tab?.reload()
          })
        )
      }
    } else {
      scrollView.refreshControl = nil
    }
    if #unavailable(iOS 26) {
      // Ensure we disable clipsToBounds again as its re-enabled when you set `refreshControl` and
      // we want it disabled to have a stylistic blur shown below the toolbars
      scrollView.clipsToBounds = false
    }
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    updatePullToRefreshVisibility()
  }

  func tabDidCreateWebView(_ tab: some TabState) {
    updatePullToRefreshVisibility()
  }

  func tabDidStopLoading(_ tab: some TabState) {
    guard let refreshControl = tab.webViewProxy?.scrollView?.refreshControl else { return }
    refreshControl.endRefreshing()
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  func preferencesDidChange(for key: String) {
    updatePullToRefreshVisibility()
  }
}
