// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared

extension BrowserViewController: TabObserver {

  func tabDidUpdateURL(_ tab: Tab) {
    if tab === tabManager.selectedTab && !tab.restoring {
      updateUIForReaderHomeStateForTab(tab)
    }

    if tab.url?.origin == tab.previousComittedURL?.origin {
      // Catch history pushState navigation, but ONLY for same origin navigation,
      // for reasons above about URL spoofing risk.
      navigateInTab(tab: tab)
    } else {
      updateURLBar()

      // If navigation will start from NTP, tab display url will be nil until
      // didCommit is called and it will cause url bar be empty in that period
      // To fix this when tab display url is empty, webview url is used
      if tab === tabManager.selectedTab, tab.url?.displayURL == nil {
        if let url = tab.visibleURL, !url.isLocal, !InternalURL.isValid(url: url) {
          updateToolbarCurrentURL(url.displayURL)
        }
      } else if tab === tabManager.selectedTab, tab.url?.displayURL?.scheme == "about",
        !tab.loading
      {
        if !tab.restoring {
          updateUIForReaderHomeStateForTab(tab)
        }

        navigateInTab(tab: tab)
      } else if tab === tabManager.selectedTab, tab.isDisplayingBasicAuthPrompt {
        updateToolbarCurrentURL(
          URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
        )
      }
    }

    // Rewards reporting
    if let url = tab.url, !url.isLocal {
      // Notify Brave Rewards library of the same document navigation.
      if let tab = tabManager.selectedTab,
        let rewardsURL = tab.rewardsXHRLoadURL,
        url.host == rewardsURL.host
      {
        tab.reportPageLoad(to: rewards, redirectChain: [url])
      }
    }

    // Update the estimated progress when the URL changes. Estimated progress may update to 0.1 when the url
    // is still an internal URL even though a request may be pending for a web page.
    if tab === tabManager.selectedTab, let url = tab.visibleURL,
      !InternalURL.isValid(url: url), tab.estimatedProgress > 0
    {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    }

    Task {
      await tab.updateSecureContentState()
      if self.tabManager.selectedTab === tab {
        self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
      }
    }
  }

  func tabDidChangeLoadProgress(_ tab: Tab) {
    guard tab === tabManager.selectedTab else { return }
    if let url = tab.visibleURL, !InternalURL.isValid(url: url) {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    } else {
      topToolbar.hideProgressBar()
    }
  }

  func tabDidStartLoading(_ tab: Tab) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.loading
  }

  func tabDidStopLoading(_ tab: Tab) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.loading
    if tab.estimatedProgress != 1 {
      topToolbar.updateProgressBar(1)
    }
  }

  func tabDidChangeTitle(_ tab: Tab) {
    // Ensure that the tab title *actually* changed to prevent repeated calls
    // to navigateInTab(tab:).
    guard
      let title = (tab.title?.isEmpty == true ? tab.url?.absoluteString : tab.title)
    else { return }
    if !title.isEmpty && title != tab.lastTitle {
      navigateInTab(tab: tab)
      tabsBar.updateSelectedTabTitle()
    }
  }

  func tabDidChangeBackForwardState(_ tab: Tab) {
    if tab !== tabManager.selectedTab { return }
    updateBackForwardActionStatus(for: tab)
  }

  func tabDidChangeVisibleSecurityState(_ tab: Tab) {
    if tabManager.selectedTab === tab {
      self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
    }
  }

  func tabDidChangeSampledPageTopColor(_ tab: Tab) {
    if tabManager.selectedTab === tab {
      updateStatusBarOverlayColor()
    }
  }
}
