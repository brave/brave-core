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
  private weak var tab: TabState?
  private lazy var refreshControl = UIRefreshControl(
    frame: .zero,
    primaryAction: .init(handler: { [weak self] _ in
      self?.handleRefresh()
    })
  )

  init(tab: TabState) {
    self.tab = tab

    tab.addObserver(self)
    Preferences.General.enablePullToRefresh.observe(from: self)
  }

  private func handleRefresh() {
    tab?.reload()
  }

  private func updatePullToRefreshVisibility() {
    guard let url = tab?.visibleURL, let scrollView = tab?.webScrollView else { return }
    scrollView.refreshControl =
      url.isLocalUtility || !Preferences.General.enablePullToRefresh.value ? nil : refreshControl
  }

  func tabDidUpdateURL(_ tab: TabState) {
    updatePullToRefreshVisibility()
  }

  func tab(_ tab: TabState, didCreateWebView webView: UIView) {
    updatePullToRefreshVisibility()
  }

  func tabDidStopLoading(_ tab: TabState) {
    refreshControl.endRefreshing()
  }

  func tabWillBeDestroyed(_ tab: TabState) {
    tab.removeObserver(self)
  }

  func preferencesDidChange(for key: String) {
    updatePullToRefreshVisibility()
  }
}
