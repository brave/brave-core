// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import UIKit

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
  private weak var tab: Tab?
  private lazy var refreshControl = UIRefreshControl(
    frame: .zero,
    primaryAction: .init(handler: { [weak self] _ in
      self?.handleRefresh()
    })
  )

  init(tab: Tab) {
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

  func tabDidUpdateURL(_ tab: Tab) {
    updatePullToRefreshVisibility()
  }

  func tab(_ tab: Tab, didCreateWebView webView: UIView) {
    updatePullToRefreshVisibility()
  }

  func tabDidStopLoading(_ tab: Tab) {
    refreshControl.endRefreshing()
  }

  func tabWillBeDestroyed(_ tab: Tab) {
    tab.removeObserver(self)
  }

  func preferencesDidChange(for key: String) {
    updatePullToRefreshVisibility()
  }
}
