// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Combine
import Data
import Foundation
import Observation
import Shared
import Web

/// The state displayed by the browser toolbars.
///
/// Most of the state is derived from the selected tab and kept up to date by observing the
/// `TabManager` and the selected tab. The remaining writable state is driven by features owned by
/// `BrowserViewController`.
///
/// This state exists to update toolbar UI only. Browser logic must query the underlying source
/// (e.g. the selected tab or its tab helpers) rather than reading it from here.
@Observable
final class BrowserToolbarState {
  @ObservationIgnored private let tabManager: TabManager
  @ObservationIgnored private weak var observedTab: (any TabState)?

  init(tabManager: TabManager) {
    self.tabManager = tabManager

    tabManager.addDelegate(self)
    observeSelectedTab(tabManager.selectedTab, previous: nil)
  }

  // MARK: - Selected Tab State

  /// The URL to display in the location bar
  ///
  /// This may differ from the selected tab's visible URL, such as while an HTTP authentication
  /// prompt is presented for a different origin, and may be `nil` when there is nothing to display, such as on the new
  /// tab page
  var displayedURL: URL? {
    if isDisplayingCrossOriginBasicAuthPrompt {
      return URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
    }
    return visibleURL?.displayURL
  }

  /// The visible URL of the selected tab.
  ///
  /// This isn't actually used directly by toolbar UI, and instead `displayedURL` is exposed, but
  /// we still need a observation tracked version that the read-only var derives from
  private var visibleURL: URL?
  /// The security state of the selected tab
  private(set) var secureContentState: SecureContentState = .unknown
  /// Whether or not the tab is loading
  private(set) var isLoading: Bool = false
  /// The page load progress to display, or `nil` if the progress bar should be hidden
  private(set) var loadingProgress: Float?
  /// Whether or not the selected tab can navigate backwards
  private(set) var canGoBack: Bool = false
  /// Whether or not the selected tab can navigate forwards
  private(set) var canGoForward: Bool = false
  /// The number of tabs open in the current browsing mode
  private(set) var tabCount: Int = 0

  /// Whether or not the selected tab is displaying a web page
  var isWebPage: Bool {
    visibleURL?.isWebPage() ?? false
  }

  /// Whether or not the selected tab is displaying the new tab page
  var isNewTabPage: Bool {
    visibleURL?.isNewTabURL ?? false
  }

  // MARK: - Browser State

  /// Whether or not the selected tab is presenting an HTTP authentication prompt for an origin
  /// other than the one currently visible
  var isDisplayingCrossOriginBasicAuthPrompt: Bool = false
  /// Whether or not a navigation is being resolved before the tab begins loading it, such as a
  /// decentralized DNS lookup
  var isResolvingNavigation: Bool = false
  /// The state of the playlist button in the location bar
  var playlistButtonState: PlaylistURLBarButton.State = .none
  /// The translation state of the displayed page
  var translationState: TranslationState = .unavailable
  /// The state of the wallet button in the location bar
  var walletButtonState: WalletURLBarButton.ButtonState = .inactive
  /// The reader mode state of the selected tab
  var readerModeState: ReaderModeState = .unavailable

  // MARK: - Tab Menu State

  // The following state decides which items are offered in the tabs & add tab menus. Some of it is
  // derived from sources that can't be observed, so it must be read at the time the menus are
  // displayed.

  /// Whether or not the selected tab is private
  var isPrivateTab: Bool {
    observedTab?.isPrivate == true
  }

  /// Whether or not the selected tab can be bookmarked
  var canBookmarkTab: Bool {
    visibleURL?.isWebPage() == true
  }

  /// The number of open tabs in the current browsing mode that can be bookmarked
  var bookmarkableTabCount: Int {
    tabManager.openedWebsitesCount
  }

  /// Whether or not the selected tab can be duplicated
  var canDuplicateTab: Bool {
    return visibleURL?.isWebPage() == true
  }

  /// Whether or not there are recently closed tabs that can be reopened. Recently closed tabs are
  /// only tracked in normal browsing mode.
  var hasRecentlyClosedTabs: Bool {
    !isPrivateTab && RecentlyClosed.first() != nil
  }

  /// Whether or not the selected tab's site data can be shredded
  var canShredSiteData: Bool {
    FeatureList.kBraveShredFeature.enabled
      && visibleURL?.isShredAvailable == true
  }

  // MARK: - Updates

  private func observeSelectedTab(_ tab: (any TabState)?, previous: (any TabState)?) {
    observedTab?.removeObserver(self)
    observedTab = tab
    tab?.addObserver(self)

    updateTabCount()

    guard let tab else {
      visibleURL = nil
      secureContentState = .unknown
      isLoading = false
      loadingProgress = nil
      canGoBack = false
      canGoForward = false
      readerModeState = .unavailable
      isDisplayingCrossOriginBasicAuthPrompt = false
      return
    }

    isDisplayingCrossOriginBasicAuthPrompt =
      tab.browserData?.isDisplayingCrossOriginBasicAuthPrompt == true
    updateNavigationState(from: tab)
    isLoading = tab.isLoading

    if let url = tab.visibleURL, !url.isNewTabURL, !InternalURL.isValid(url: url) {
      // Only update the progress if it differs between tabs so that switching between fully loaded
      // tabs doesn't display the progress animation
      let previousProgress = previous?.estimatedProgress ?? 1.0
      if previousProgress != tab.estimatedProgress {
        loadingProgress = Float(tab.estimatedProgress)
      }
    } else {
      loadingProgress = nil
    }

    let isPlaylistButtonVisible =
      tab.visibleURL?.isPlaylistSupportedSiteURL == true
      && tab.playlist?.isPlaylistBlocked(tab.visibleURL) == false
    // The playlist button takes priority over the reader mode button
    readerModeState =
      isPlaylistButtonVisible ? .unavailable : (tab.readerMode?.state ?? .unavailable)
  }

  private func updateNavigationState(from tab: some TabState) {
    visibleURL = tab.visibleURL
    secureContentState = tab.visibleSecureContentState
    let isForwardItemReaderMode =
      tab.backForwardList?.forwardList.first?.url.isInternalURL(for: .readermode) == true
    canGoForward = tab.canGoForward && !isForwardItemReaderMode
    canGoBack = tab.canGoBack
  }

  private func updateTabCount() {
    tabCount = tabManager.tabsForCurrentMode.count
  }
}

// MARK: - TabManagerDelegate

extension BrowserToolbarState: TabManagerDelegate {
  func tabManager(
    _ tabManager: TabManager,
    didSelectedTabChange selected: (any TabState)?,
    previous: (any TabState)?
  ) {
    observeSelectedTab(selected, previous: previous)
  }

  func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState) {
    // Restored tabs are counted once restoration completes
    if !tabManager.isRestoring {
      updateTabCount()
    }
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState) {
    updateTabCount()
  }

  func tabManagerDidAddTabs(_ tabManager: TabManager) {
    updateTabCount()
  }

  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    updateTabCount()
  }
}

// MARK: - TabObserver

extension BrowserToolbarState: TabObserver {
  func tabDidStartNavigation(_ tab: some TabState) {
    // Hide the reader mode button until the new page has been checked for readability, unless
    // navigating to a reader mode page
    if let url = tab.visibleURL, !url.isInternalURL(for: .readermode) {
      readerModeState = .unavailable
    }
  }

  func tabDidCommitNavigation(_ tab: some TabState) {
    updateNavigationState(from: tab)
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    updateNavigationState(from: tab)

    // Estimated progress may update to 0.1 while the URL is still an internal URL even though a
    // request may be pending for a web page, so update it once the URL changes
    if let url = tab.visibleURL, !url.isNewTabURL, !InternalURL.isValid(url: url), tab.isLoading,
      tab.estimatedProgress > 0
    {
      loadingProgress = Float(tab.estimatedProgress)
    }
  }

  func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    secureContentState = tab.visibleSecureContentState
  }

  func tabDidChangeBackForwardState(_ tab: some TabState) {
    updateNavigationState(from: tab)
  }

  func tabDidStartLoading(_ tab: some TabState) {
    isLoading = tab.isLoading
  }

  func tabDidStopLoading(_ tab: some TabState) {
    isLoading = tab.isLoading
    if tab.estimatedProgress != 1 {
      loadingProgress = 1
    }
  }

  func tabDidChangeLoadProgress(_ tab: some TabState) {
    if let url = tab.visibleURL, !url.isNewTabURL, !InternalURL.isValid(url: url), tab.isLoading {
      loadingProgress = Float(tab.estimatedProgress)
    } else {
      loadingProgress = nil
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
