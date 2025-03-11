// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import BraveWallet
import Foundation
import Preferences
import Shared

extension BrowserViewController: TabObserver {
  func tabDidStartNavigation(_ tab: Tab) {
    tab.contentBlocker.clearPageStats()

    let visibleURL = tab.url

    if tab === tabManager.selectedTab {
      toolbarVisibilityViewModel.toolbarState = .expanded
      clearPageZoomDialog()

      // If we are going to navigate to a new page, refresh the translate status.
      topToolbar.updateTranslateButtonState(
        tabManager.selectedTab?.translationState ?? .unavailable
      )

      // If we are going to navigate to a new page, hide the reader mode button. Unless we
      // are going to a about:reader page. Then we keep it on screen: it will change status
      // (orange color) as soon as the page has loaded.
      if let url = visibleURL {
        if !url.isInternalURL(for: .readermode) {
          topToolbar.updateReaderModeState(.unavailable)
          hideReaderModeBar(animated: false)
        }
      }
    }

    // check if web view is loading a different origin than the one currently loaded
    if let selectedTab = tabManager.selectedTab,
      selectedTab.url?.origin != visibleURL?.origin
    {
      // new site has a different origin, hide wallet icon.
      tabManager.selectedTab?.isWalletIconVisible = false
      // new site, reset connected addresses
      tabManager.selectedTab?.clearSolanaConnectedAccounts()
      // close wallet panel if it's open
      if let popoverController = self.presentedViewController as? PopoverController,
        popoverController.contentController is WalletPanelHostingController
      {
        self.dismiss(animated: true)
      }
    }

    hideToastsOnNavigationStartIfNeeded(tabManager)

    // Reset redirect chain
    tab.redirectChain = []
    if let url = visibleURL {
      tab.redirectChain.append(url)
    }
  }

  func tabDidCommitNavigation(_ tab: Tab) {
    // Reset the stored http request now that load has committed.
    tab.upgradedHTTPSRequest = nil
    tab.upgradeHTTPSTimeoutTimer?.invalidate()
    tab.upgradeHTTPSTimeoutTimer = nil

    // Need to evaluate Night mode script injection after url is set inside the Tab
    tab.nightMode = Preferences.General.nightModeEnabled.value
    tab.clearSolanaConnectedAccounts()

    // Dismiss any alerts that are showing on page navigation.
    if let alert = tab.shownPromptAlert {
      alert.dismiss(animated: false)
    }

    // Providers need re-initialized when changing origin to align with desktop in
    // `BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame`
    // https://github.com/brave/brave-core/blob/1.52.x/browser/brave_content_browser_client.cc#L608
    if let provider = braveCore.braveWalletAPI.ethereumProvider(
      with: tab,
      isPrivateBrowsing: tab.isPrivate
    ) {
      // The Ethereum provider will fetch allowed accounts from it's delegate (the tab)
      // on initialization. Fetching allowed accounts requires the origin; so we need to
      // initialize after `commitedURL` / `url` are updated above
      tab.walletEthProvider = provider
      tab.walletEthProvider?.initialize(eventsListener: tab)
    }
    if let provider = braveCore.braveWalletAPI.solanaProvider(
      with: tab,
      isPrivateBrowsing: tab.isPrivate
    ) {
      tab.walletSolProvider = provider
      tab.walletSolProvider?.initialize(eventsListener: tab)
    }

    // Notify of tab changes after navigation completes but before notifying that
    // the tab has loaded, so that any listeners can process the tab changes
    // before the tab is considered loaded.
    rewards.maybeNotifyTabDidChange(
      tab: tab,
      isSelected: tabManager.selectedTab == tab
    )
    rewards.maybeNotifyTabDidLoad(tab: tab)

    // The toolbar and url bar changes can not be
    // on different tab than selected. Or the webview
    // previews and etc will effect the status
    guard tabManager.selectedTab === tab else {
      return
    }

    updateUIForReaderHomeStateForTab(tab)
    updateBackForwardActionStatus(for: tab)
  }

  func tabDidFinishNavigation(_ tab: Tab) {
    if !Preferences.Privacy.privateBrowsingOnly.value
      && (!tab.isPrivate || Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      tabManager.preserveScreenshot(for: tab)
      tabManager.saveTab(tab)
    }

    // Inject app's IAP receipt for Brave SKUs if necessary
    if !tab.isPrivate {
      Task { @MainActor in
        await BraveSkusAccountLink.injectLocalStorage(tab: tab)
      }
    }

    // Second attempt to inject results to the BraveSearch.
    // This will be called if we got fallback results faster than
    // the page navigation.
    if let braveSearchManager = tab.braveSearchManager {
      // Fallback results are ready before navigation finished,
      // they must be injected here.
      if !braveSearchManager.fallbackQueryResultsPending {
        tab.injectResults()
      }
    } else {
      // If not applicable, null results must be injected regardless.
      // The website waits on us until this is called with either results or null.
      tab.injectResults()
    }

    navigateInTab(tab: tab)
    rewards.reportTabUpdated(
      tab: tab,
      isSelected: tabManager.selectedTab == tab,
      isPrivate: privateBrowsingManager.isPrivateBrowsing
    )
    tab.reportPageLoad(to: rewards, redirectChain: tab.redirectChain)
    // Reset `rewardsReportingState` tab property so that listeners
    // can be notified of tab changes when a new navigation happens.
    tab.rewardsReportingState = RewardsTabChangeReportingState()

    Task {
      await tab.updateEthereumProperties()
      await tab.updateSolanaProperties()
    }

    if tab.url?.isLocal == false {
      // Set rewards inter site url as new page load url.
      tab.rewardsXHRLoadURL = tab.url
    }

    if tab.walletEthProvider != nil {
      tab.emitEthereumEvent(.connect)
    }

    if let lastCommittedURL = tab.committedURL {
      maybeRecordBraveSearchDailyUsage(url: lastCommittedURL)
    }

    // Added this method to determine long press menu actions better
    // Since these actions are depending on tabmanager opened WebsiteCount
    updateToolbarUsingTabManager(tabManager)

    recordFinishedPageLoadP3A()
  }

  func tab(_ tab: Tab, didFailNavigationWithError error: any Error) {
    let error = error as NSError
    if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
      // load cancelled / user stopped load. Cancel https upgrade fallback timer.
      tab.upgradedHTTPSRequest = nil
      tab.upgradeHTTPSTimeoutTimer?.invalidate()
      tab.upgradeHTTPSTimeoutTimer = nil

      if tab === tabManager.selectedTab {
        if let displayURL = tab.url?.displayURL {
          updateToolbarCurrentURL(displayURL)
        } else if let url = tab.visibleURL, !url.isLocal, !InternalURL.isValid(url: url) {
          updateToolbarCurrentURL(url.displayURL)
        }
        updateWebViewPageZoom(tab: tab)
      }
      return
    }

    if let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL {
      // Check for invalid upgrade to https
      if url.scheme == "https",  // verify failing url was https
        let response = handleInvalidHTTPSUpgrade(
          tab: tab,
          responseURL: url
        )
      {
        // load original or strict mode interstitial
        tab.loadRequest(response)
      }
    }
  }

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
