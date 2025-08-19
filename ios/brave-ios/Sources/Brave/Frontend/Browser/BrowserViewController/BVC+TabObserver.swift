// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import BraveWallet
import Foundation
import Preferences
import Shared
import UIKit
import Web

extension BrowserViewController: TabObserver {
  public func tabDidCreateWebView(_ tab: some TabState) {
    tab.view.frame = webViewContainer.frame

    if let scrollView = tab.webViewProxy?.scrollView {
      if tab.isVisible {
        toolbarVisibilityViewModel.beginObservingScrollView(scrollView)
      }
      if #available(iOS 26.0, *) {
        // We set the content inset + safe area insets ourselves so we normalize this to `always`
        scrollView.contentInsetAdjustmentBehavior = .always
      } else {
        // On iOS 17/18 setting clipsToBounds as false can still let us achieve the visual effect
        // of the web view scrolling under blurred toolbars despite the frame being tied to the
        // toolbars itself.
        scrollView.clipsToBounds = false
      }
    }

    var injectedScripts: [TabContentScript] = [
      ReaderModeScriptHandler(),
      ErrorPageHelper(certStore: profile.certStore),
      BlockedDomainScriptHandler(),
      HTTPBlockedScriptHandler(tabManager: tabManager),
      PrintScriptHandler(browserController: self),
      CustomSearchScriptHandler(),
      DarkReaderScriptHandler(),
      FocusScriptHandler(),
      BraveGetUA(),
      BraveSearchScriptHandler(profile: profile, rewards: rewards),
      ResourceDownloadScriptHandler(),
      DownloadContentScriptHandler(browserController: self),
      AdsMediaReportingScriptHandler(rewards: rewards),
      ReadyStateScriptHandler(),
      DeAmpScriptHandler(),
      SiteStateListenerScriptHandler(),
      CosmeticFiltersScriptHandler(),
      URLPartinessScriptHandler(),
      FaviconScriptHandler(),
      YoutubeQualityScriptHandler(),
      BraveLeoScriptHandler(),
      BraveSkusScriptHandler(),
      RequestBlockingContentScriptHandler(),
    ]

    if let contentBlocker = tab.contentBlocker {
      injectedScripts.append(contentBlocker)
    }

    if profileController.profile.prefs.isPlaylistAvailable {
      injectedScripts.append(contentsOf: [
        PlaylistScriptHandler(tab: tab),
        PlaylistFolderSharingScriptHandler(),
      ])
    }

    if profileController.profile.prefs.isBraveTalkAvailable {
      injectedScripts.append(
        BraveTalkScriptHandler(
          rewards: rewards,
          launchNativeBraveTalk: { [weak self] tab, room, token in
            self?.launchNativeBraveTalk(tab: tab, room: room, token: token)
          }
        )
      )
    }

    if profileController.braveWalletAPI.isAllowed {
      injectedScripts.append(Web3NameServiceScriptHandler())
    }

    // Only add the logins handler and wallet provider if the tab is NOT a private browsing tab
    if !tab.isPrivate {
      injectedScripts += [
        LoginsScriptHandler(profile: profile, passwordAPI: profileController.passwordAPI),
        BraveSearchResultAdScriptHandler(),
      ]
      if profileController.braveWalletAPI.isAllowed {
        injectedScripts += [
          EthereumProviderScriptHandler(),
          SolanaProviderScriptHandler(),
        ]
      }
    }

    if FeatureList.kBraveTranslateEnabled.enabled {
      injectedScripts.append(contentsOf: [
        BraveTranslateScriptLanguageDetectionHandler(),
        BraveTranslateScriptHandler(),
      ])
    }

    // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
    // let spotlightHelper = SpotlightHelper(tab: tab)
    // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

    injectedScripts.forEach {
      tab.browserData?.addContentScript(
        $0,
        name: type(of: $0).scriptName,
        contentWorld: type(of: $0).scriptSandbox
      )
    }

    (tab.browserData?.getContentScript(name: ReaderModeScriptHandler.scriptName)
      as? ReaderModeScriptHandler)?
      .delegate = self
    (tab.browserData?.getContentScript(name: PlaylistScriptHandler.scriptName)
      as? PlaylistScriptHandler)?
      .delegate = self
    (tab.browserData?.getContentScript(name: PlaylistFolderSharingScriptHandler.scriptName)
      as? PlaylistFolderSharingScriptHandler)?.delegate = self
    (tab.browserData?.getContentScript(name: Web3NameServiceScriptHandler.scriptName)
      as? Web3NameServiceScriptHandler)?.delegate = self
  }

  public func tabWillDeleteWebView(_ tab: some TabState) {
    tab.browserData?.cancelQueuedAlerts()
    if let scrollView = tab.webViewProxy?.scrollView {
      toolbarVisibilityViewModel.endScrollViewObservation(scrollView)
    }
    tab.view.removeFromSuperview()
  }

  public func tabDidStartNavigation(_ tab: some TabState) {
    tab.contentBlocker?.clearPageStats()

    let visibleURL = tab.visibleURL

    if tab === tabManager.selectedTab {
      toolbarVisibilityViewModel.toolbarState = .expanded
      clearPageZoomDialog()

      // If we are going to navigate to a new page, refresh the translate status.
      updateTranslateURLBar(tab: tab, state: .unavailable)

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
      selectedTab.visibleURL?.origin != visibleURL?.origin
    {
      // new site has a different origin, hide wallet icon.
      tabManager.selectedTab?.isWalletIconVisible = false
      // new site, reset connected addresses
      tabManager.selectedTab?.browserData?.clearSolanaConnectedAccounts()
      // close wallet panel if it's open
      if let popoverController = self.presentedViewController as? PopoverController,
        popoverController.contentController is WalletPanelHostingController
      {
        self.dismiss(animated: true)
      }
    }

    hideToastsOnNavigationStartIfNeeded(tabManager)
  }

  public func tabDidCommitNavigation(_ tab: some TabState) {
    // Reset the stored http request now that load has committed.
    tab.upgradedHTTPSRequest = nil
    tab.upgradeHTTPSTimeoutTimer?.invalidate()
    tab.upgradeHTTPSTimeoutTimer = nil

    // Clear the current request url and the redirect source url
    // We don't need these values after the request has been comitted
    tab.currentRequestURL = nil
    tab.redirectSourceURL = nil
    tab.isInternalRedirect = false

    // Need to evaluate Night mode script injection after url is set inside the Tab
    tab.nightMode = Preferences.General.nightModeEnabled.value

    // Dismiss any alerts that are showing on page navigation.
    if let alert = tab.shownPromptAlert {
      alert.dismiss(animated: false)
    }

    // Providers need re-initialized when changing origin to align with desktop in
    // `BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame`
    // https://github.com/brave/brave-core/blob/1.52.x/browser/brave_content_browser_client.cc#L608
    if profileController.braveWalletAPI.isAllowed {
      tab.browserData?.clearSolanaConnectedAccounts()

      if let browserData = tab.browserData {
        if let provider = profileController.braveWalletAPI.ethereumProvider(
          with: browserData,
          isPrivateBrowsing: tab.isPrivate
        ) {
          // The Ethereum provider will fetch allowed accounts from it's delegate (the tab)
          // on initialization. Fetching allowed accounts requires the origin; so we need to
          // initialize after `commitedURL` / `url` are updated above
          tab.walletEthProvider = provider
          tab.walletEthProvider?.initialize(eventsListener: browserData)
        }
        if let provider = profileController.braveWalletAPI.solanaProvider(
          with: browserData,
          isPrivateBrowsing: tab.isPrivate
        ) {
          tab.walletSolProvider = provider
          tab.walletSolProvider?.initialize(eventsListener: browserData)
        }
      }
    }

    // Notify of tab changes after navigation completes but before notifying that
    // the tab has loaded, so that any listeners can process the tab changes
    // before the tab is considered loaded.
    rewards.maybeNotifyTabDidChange(
      tab: tab,
      isSelected: tabManager.selectedTab === tab
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

  public func tabDidFinishNavigation(_ tab: some TabState) {
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
      isSelected: tabManager.selectedTab === tab,
      isPrivate: privateBrowsingManager.isPrivateBrowsing
    )
    tab.browserData?.reportPageLoad(to: rewards, redirectChain: tab.redirectChain)
    // Reset `rewardsReportingState` tab property so that listeners
    // can be notified of tab changes when a new navigation happens.
    tab.rewardsReportingState = RewardsTabChangeReportingState()

    Task {
      await tab.browserData?.updateEthereumProperties()
      await tab.browserData?.updateSolanaProperties()
    }

    if tab.visibleURL?.isLocal == false {
      // Set rewards inter site url as new page load url.
      tab.rewardsXHRLoadURL = tab.visibleURL
    }

    if tab.walletEthProvider != nil {
      tab.browserData?.emitEthereumEvent(.connect)
    }

    if let lastCommittedURL = tab.lastCommittedURL {
      maybeRecordBraveSearchDailyUsage(url: lastCommittedURL)
    }

    // Added this method to determine long press menu actions better
    // Since these actions are depending on tabmanager opened WebsiteCount
    updateToolbarUsingTabManager(tabManager)

    recordFinishedPageLoadP3A()
  }

  public func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    let error = error as NSError
    if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
      // load cancelled / user stopped load. Cancel https upgrade fallback timer.
      tab.upgradedHTTPSRequest = nil
      tab.upgradeHTTPSTimeoutTimer?.invalidate()
      tab.upgradeHTTPSTimeoutTimer = nil

      if tab === tabManager.selectedTab {
        if let displayURL = tab.visibleURL?.displayURL {
          updateToolbarCurrentURL(displayURL)
        } else if let url = tab.url, !url.isLocal, !InternalURL.isValid(url: url) {
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
        return
      }

      if !FeatureList.kUseChromiumWebViews.enabled {
        // Only handle error pages ourselves for legacy web views
        ErrorPageHelper(certStore: profile.certStore).loadPage(error, forUrl: url, inTab: tab)
      }
    }
  }

  public func tabRenderProcessDidTerminate(_ tab: some TabState) {
    guard let url = tab.lastCommittedURL else { return }
    if InternalURL.isValid(url: url) {
      // No need to refresh an internal url
      return
    }
    // For now just reload the page when the process crashes
    tab.reload()
  }

  public func tabDidUpdateURL(_ tab: some TabState) {
    if tab.isDisplayingBasicAuthPrompt == true {
      tab.setVirtualURL(
        URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
      )
    }

    if tab === tabManager.selectedTab && !tab.isRestoring {
      updateUIForReaderHomeStateForTab(tab)
    }

    if tab.visibleURL?.origin == tab.previousCommittedURL?.origin {
      // Catch history pushState navigation, but ONLY for same origin navigation,
      // for reasons above about URL spoofing risk.
      navigateInTab(tab: tab)
    } else {
      updateURLBar()

      // If navigation will start from NTP, tab display url will be nil until
      // didCommit is called and it will cause url bar be empty in that period
      // To fix this when tab display url is empty, webview url is used
      if tab === tabManager.selectedTab, tab.visibleURL?.displayURL == nil {
        if let url = tab.url, !url.isLocal, !InternalURL.isValid(url: url) {
          updateToolbarCurrentURL(url.displayURL)
        }
      } else if tab === tabManager.selectedTab, tab.visibleURL?.displayURL?.scheme == "about",
        !tab.isLoading
      {
        if !tab.isRestoring {
          updateUIForReaderHomeStateForTab(tab)
        }

        navigateInTab(tab: tab)
      } else if tab === tabManager.selectedTab, let tabData = tab.browserData,
        tabData.isDisplayingBasicAuthPrompt
      {
        updateToolbarCurrentURL(
          URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
        )
      }
    }

    // Rewards reporting
    if let url = tab.visibleURL, !url.isLocal {
      // Notify Brave Rewards library of the same document navigation.
      if let tab = tabManager.selectedTab,
        let rewardsURL = tab.rewardsXHRLoadURL,
        url.host == rewardsURL.host
      {
        tab.browserData?.reportPageLoad(to: rewards, redirectChain: [url])
      }
    }

    // Update the estimated progress when the URL changes. Estimated progress may update to 0.1 when the url
    // is still an internal URL even though a request may be pending for a web page.
    if tab === tabManager.selectedTab, let url = tab.url,
      !InternalURL.isValid(url: url), tab.estimatedProgress > 0
    {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    }

    Task {
      if self.tabManager.selectedTab === tab {
        self.updateToolbarSecureContentState(tab.visibleSecureContentState)
      }
    }
  }

  public func tabDidChangeLoadProgress(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    if let url = tab.url, !InternalURL.isValid(url: url) {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    } else {
      topToolbar.hideProgressBar()
    }
  }

  public func tabDidStartLoading(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.isLoading
  }

  public func tabDidStopLoading(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.isLoading
    if tab.estimatedProgress != 1 {
      topToolbar.updateProgressBar(1)
    }
  }

  public func tabDidChangeTitle(_ tab: some TabState) {
    // Ensure that the tab title *actually* changed to prevent repeated calls
    // to navigateInTab(tab:).
    guard
      let title = (tab.title?.isEmpty == true ? tab.visibleURL?.absoluteString : tab.title)
    else { return }
    if !title.isEmpty && title != tab.lastTitle {
      navigateInTab(tab: tab)
      tabsBar.updateSelectedTabTitle()
    }
  }

  public func tabDidChangeBackForwardState(_ tab: some TabState) {
    if tab !== tabManager.selectedTab { return }
    updateBackForwardActionStatus(for: tab)
  }

  public func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    if tabManager.selectedTab === tab {
      self.updateToolbarSecureContentState(tab.visibleSecureContentState)
    }
  }

  public func tabDidChangeSampledPageTopColor(_ tab: some TabState) {
    if tabManager.selectedTab === tab {
      updateStatusBarOverlayColor()
    }
  }
}
