// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveShared
import BraveShields
import BraveUI
import BraveWallet
import Data
import Foundation
import Preferences
import Shared
import SwiftUI
import Web
import WebKit
import os.log

extension BrowserViewController: TabManagerDelegate {
  func attachTabHelpers(to tab: some TabState) {
    tab.browserData = .init(tab: tab, tabGeneratorAPI: profileController.tabGeneratorAPI)
    tab.pullToRefresh = .init(tab: tab)
    if tab.profile.prefs.isPlaylistAvailable {
      tab.playlist = .init(tab: tab, delegate: self)
    }
    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      tab.youtubeQualityTabHelper = .init(tab: tab)
    }
    SnackBarTabHelper.create(for: tab)
    tab.braveUserAgentExceptions = braveCore.braveUserAgentExceptions
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      tab.translate = .init(tab: tab, delegate: self)
    } else {
      tab.legacyTranslateHelper = .init(tab: tab, delegate: self)
    }
    tab.pageMetadataHelper = .init(tab: tab)
    tab.faviconTabHelper = .init(tab: tab)
    tab.userActivityHelper = .init(tab: tab)
    tab.print = .init(tab: tab, baseViewController: self)
    tab.externalAppURLHelper = .init(tab: tab, browserViewController: self)
    tab.forcePaste = .init(tab: tab)
    tab.aiChatWebUIHelper = .init(
      tab: tab,
      webDelegate: tab.leoTabHelper,
      braveTalkJavascript: braveTalkJitsiCoordinator,
      profileController: profileController
    )
    tab.aiChatWebUIHelper?.attachPrivacySensitiveTabHelpers = { detachedTab, _ in
      detachedTab.detachedPrivacyHelper = .init(
        tab: detachedTab
      )
    }
    tab.aiChatWebUIHelper?.handler = { [weak self] tab, action in
      self?.handleAIChatWebUIPageAction(tab, action: action)
    }
    tab.aiChatWebUIHelper?.tabsForPrivateMode = { [weak self] isPrivate in
      // Technically we will never get a private tab here since AI Chat WebUI is not supported there
      // but in case its called incorrectly, avoid returning any private tabs
      guard let self, !isPrivate else { return [] }
      return tabManager.allTabs.filter { !$0.isPrivate }
    }
    tab.aiChatWebUIHelper?.webDelegateForTab = { detachedTab in
      /// If AIChat created a hidden tab for history or bookmarks, we need to
      /// use it's `AIChatWebDelegate` to fetch content from.
      detachedTab.leoTabHelper
    }
    tab.wallet = .init(tab: tab, braveWalletAPI: profileController.braveWalletAPI)
    tab.wallet?.delegate = self
    tab.walletWebUIHelper = .init(
      tab: tab,
      showApprovePanelUIHandler: { [weak self] tab in
        self?.showApprovePanelUI(tab: tab)
      },
      showWalletBackUpHandler: { [weak self] in
        self?.showWalletBackupUI()
      },
      unlockWalletHandler: { [weak self] in
        self?.unlockWalletUI()
      },
      showOnboardingHandler: { [weak self] isNewWallet in
        self?.showOnboarding(isNewWallet)
      },
      openWalletHomeHandler: { [weak self] in
        self?.openWalletHome()
      },
      scanAddressQRCodeHandler: { [weak self] completion in
        self?.scanAddressQRCode(completion: completion)
      }
    )
    let braveShieldsHelper: BraveShieldsTabHelper = .init(
      tab: tab,
      braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(profile: tab.profile)
    )
    tab.braveShieldsHelper = braveShieldsHelper
    // When `BraveShieldsTabHelper+TabPolicyDecider` is moved to `BraveShields` target,
    // we should add it as a policy decider at initialization.
    tab.addPolicyDecider(braveShieldsHelper)
    // Must be added before `braveSearch`. HttpsUpgradeTabHelper needs first look at a
    // main-frame http navigation so it can attempt the upgrade before BraveSearchTabHelper
    // decides whether to route it into QuickView. BraveSearchTabHelper recognizes a reissued
    // https request via `tab.httpsUpgradeHelper?.pendingUpgrade` and only opens QuickView once
    // `tabDidFinishNavigation` confirms it actually landed on the upgraded (or
    // gracefully-rolled-back) page rather than a failure/interstitial.
    if FeatureList.kBraveHttpsByDefault.enabled {
      tab.httpsUpgradeHelper = .init(
        tab: tab,
        httpsUpgradeExceptionsService: braveCore.httpsUpgradeExceptionsService
      )
    }
    tab.cosmeticFilteringTabHelper = .init(tab: tab)
    tab.logins = .init(tab: tab, passwordAPI: profileController.passwordAPI)
    tab.protectionStats = .init(tab: tab)
    tab.nightMode = .init(tab: tab)
    // reader mode
    tab.readerMode = .init(tab: tab, readerModeCache: ReaderModeScriptHandler.cache(for: tab))
    tab.readerMode?.onStateChanged = { [weak self, weak tab] in
      guard let self, let tab, self.tabManager.selectedTab === tab else { return }
      self.toolbarState.readerModeState = tab.readerMode?.state ?? .unavailable
    }
    tab.readerMode?.onReaderModeDisplayed = { [weak self, weak tab] in
      guard let self, let tab else { return }
      self.showReaderModeBar(animated: true)
      tab.showContent(true)
    }
    tab.readerMode?.onReaderModeToggled = { [weak self] tab in
      tab.playlist?.processPlaylistInfo(item: tab.playlistItem)
      self?.updateTranslateURLBar(tab: tab, state: tab.translationState)
    }

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      tab.requestBlockingTabHelper = .init(tab: tab)
      tab.cosmeticFilteringTabHelper = .init(tab: tab)
      tab.scriptletsTabHelper = .init(tab: tab)
    }

    tab.braveTalk = .init(tab: tab, coordinator: braveTalkJitsiCoordinator)
    tab.braveTalk?.onExitCall = { [weak self] in
      guard let self = self else { return }
      // When we close the call, redirect to Brave Talk home page if the selected tab is still the
      // original talk URL
      if let url = self.tabManager.selectedTab?.visibleURL,
        let currentHost = url.host,
        DomainUserScript.braveTalkHelper.associatedDomains.contains(currentHost)
      {
        var components = URLComponents()
        components.host = currentHost
        components.scheme = url.scheme
        self.select(url: components.url!, isUserDefinedURLNavigation: false)
      }
    }

    tab.braveSearch = .init(tab: tab, rewards: rewards, searchEngines: profile.searchEngines)
    tab.braveSearch?.presentSearchResultClickedInfoBar = { [weak self] in
      guard let self else { return }
      let searchResultClickedInfobar = SearchResultAdClickedInfoBar(
        onLinkPressed: { [weak self] url in
          self?.tabManager.addTabAndSelect(URLRequest(url: url), isPrivate: false)
        }
      )
      show(toast: searchResultClickedInfobar, duration: nil)
    }
    tab.braveSearch?.presentInQuickView = { [weak self] url, tab in
      guard let self else { return }
      let quickViewController = QuickViewController(
        url: url,
        profile: tab.profile,
        syncAPI: profileController.syncAPI,
        sendTabAPI: profileController.sendTabAPI,
        historyAPI: profileController.historyAPI,
        httpsUpgradeExceptionsService: braveCore.httpsUpgradeExceptionsService,
        onOpenInNewTab: { [weak self] request, isPrivateMode in
          guard let self else { return }
          self.tabManager.addTabAndSelect(
            request,
            isPrivate: isPrivateMode
          )
        },
        onOpenInNewWindow: { [weak self] url, isPrivateMode in
          guard let self else { return }
          self.openInNewWindow(url: url, isPrivate: isPrivateMode)
        },
        onAttachTab: { [weak self] tab in
          guard let self else { return }
          let request = tab.visibleURL.map { URLRequest(url: $0) }
          self.tabManager.configureTab(tab, request: request, flushToDisk: false, zombie: true)
          self.tabManager.saveTab(tab, saveOrder: true)
          self.tabManager.selectTab(tab)
        },
        onShowConfirmationAlert: { [weak self] in
          let alert = UIAlertController(
            title: Strings.quickViewConfirmationAlertTitle,
            message: Strings.quickViewConfirmationAlertMessage,
            preferredStyle: .alert
          )
          alert.addAction(
            .init(title: Strings.quickViewConfirmationAlertKeepButtonTitle, style: .default)
          )
          alert.addAction(
            .init(
              title: Strings.quickViewConfirmationAlertTurnOffButtonTitle,
              style: .cancel,
              handler: { _ in
                Preferences.General.openLinkInQuickViewMode.value = false
              }
            )
          )
          self?.present(alert, animated: true)
        }
      )
      if let sheet = quickViewController.sheetPresentationController {
        sheet.prefersGrabberVisible = true

        let customDetentId = "customDetent"
        let customDetent = UISheetPresentationController.Detent.custom(
          identifier: .init(customDetentId)
        ) { context in
          context.maximumDetentValue * 0.95
        }
        sheet.detents = [
          customDetent,
          .large(),
        ]
        sheet.selectedDetentIdentifier = .init(customDetentId)

        sheet.prefersEdgeAttachedInCompactHeight = true
      }
      self.present(quickViewController, animated: true)
    }
    tab.blockedDomainTabHelper = .init(tab: tab)
  }

  func tabManager(
    _ tabManager: TabManager,
    didSelectedTabChange selected: (any TabState)?,
    previous: (any TabState)?
  ) {
    // Remove the old accessibilityLabel. Since this webview shouldn't be visible, it doesn't need it
    // and having multiple views with the same label confuses tests.
    if let previous, previous.isWebViewCreated {
      if let scrollView = previous.webViewProxy?.scrollView {
        toolbarVisibilityViewModel.endScrollViewObservation(scrollView)
      }

      previous.view.endEditing(true)
      previous.view.accessibilityLabel = nil
      previous.view.accessibilityElementsHidden = true
      previous.view.accessibilityIdentifier = nil
      previous.view.removeFromSuperview()
    }

    if let tab = selected {
      if let scrollView = tab.webViewProxy?.scrollView {
        // For tabs being opened by the DOM via window.open a web view may not be created yet and
        // this will instead be observed in tabDidCreateWebView
        toolbarVisibilityViewModel.beginObservingScrollView(scrollView)
      }
      toolbarVisibilityCancellable = toolbarVisibilityViewModel.objectWillChange
        .receive(on: DispatchQueue.main)
        .sink(receiveValue: { [weak self] in
          guard let self = self else { return }
          let (state, progress) = (
            self.toolbarVisibilityViewModel.toolbarState,
            self.toolbarVisibilityViewModel.interactiveTransitionProgress
          )
          self.handleToolbarVisibilityStateChange(state, progress: progress)
        })
      updateURLBar()
      recordScreenTimeUsage(for: tab)

      previous?.shownPromptAlert?.dismiss(animated: false)
      readerModeCache = ReaderModeScriptHandler.cache(for: tab)
      ReaderModeHandler.readerModeCache = readerModeCache

      webViewContainer.addSubview(tab.view)
      tab.view.snp.remakeConstraints { make in
        make.left.right.top.bottom.equalTo(self.webViewContainer)
      }

      // Add ScreenTime above the WebView
      if let screenTimeViewController = screenTimeViewController {
        if screenTimeViewController.parent == nil {
          addChild(screenTimeViewController)
          screenTimeViewController.didMove(toParent: self)
        }

        webViewContainer.addSubview(screenTimeViewController.view)

        screenTimeViewController.view.snp.remakeConstraints {
          $0.edges.equalTo(webViewContainer)
        }
      }

      tab.view.accessibilityLabel = Strings.webContentAccessibilityLabel
      tab.view.accessibilityIdentifier = "contentView"
      tab.view.accessibilityElementsHidden = false
    }

    updateStatusBarOverlayColor()

    removeAllBars()
    if let bars = selected.flatMap(SnackBarTabHelper.from)?.bars {
      for bar in bars {
        showBar(bar, animated: true)
      }
    }

    clearPageZoomDialog()
    updateTabsBarVisibility()

    let shouldShowPlaylistURLBarButton =
      selected?.visibleURL?.isPlaylistSupportedSiteURL == true
      && selected?.playlist?.isPlaylistBlocked(selected?.visibleURL) == false

    if !shouldShowPlaylistURLBarButton {
      if let readerModeState = selected?.readerMode?.state {
        if readerModeState == .active {
          showReaderModeBar(animated: false)
        } else {
          hideReaderModeBar(animated: false)
        }
      }

      updatePlaylistURLBar(
        tab: selected,
        state: selected?.playlistItemState ?? .none,
        item: selected?.playlistItem
      )
    }

    if FeatureList.kBraveTranslateEnabled.enabled, let selectedTab = selected,
      selectedTab.legacyTranslateHelper != nil || selectedTab.translate != nil
    {
      updateTranslateURLBar(tab: selectedTab, state: selectedTab.translationState)
      updatePlaylistURLBar(
        tab: selectedTab,
        state: selectedTab.playlistItemState ?? .none,
        item: selectedTab.playlistItem
      )
    } else {
      toolbarState.translationState = .unavailable
    }

    updateScreenTimeUrl(tabManager.selectedTab?.visibleURL)
    updateInContentHomePanel(selected?.visibleURL as URL?)

    removeWalletNotificationAndClearOrigin()
    let dappSupportedCoins = Array(WalletConstants.supportedCoinTypes(.dapps))
    WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(
      for: dappSupportedCoins
    )
    WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(
      coins: dappSupportedCoins
    )
    updateURLBarWalletButton()

    if #available(iOS 26.0, *) {
      if let topEdgeInteraction {
        topEdgeView.removeInteraction(topEdgeInteraction)
      }
      let interaction = UIScrollEdgeElementContainerInteraction()
      interaction.edge = .top
      interaction.scrollView = selected?.webViewProxy?.scrollView
      topEdgeView.addInteraction(interaction)
      topEdgeInteraction = interaction
    }
  }

  func tabManager(_ tabManager: TabManager, willAddTab tab: some TabState) {
  }

  func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState) {
    tab.addObserver(self)
    tab.delegate = self
    tab.downloadDelegate = self
    attachTabHelpers(to: tab)
    /// Add BVC as the last TabPolicyDecider, so it only executes on requests
    /// that all other policy deciders have decided to allow. This is for
    /// legacy logic that hasn't been migrated to it's own TabPolicyDecider yet
    tab.addPolicyDecider(self)

    SnackBarTabHelper.from(tab: tab)?.delegate = self

    tab.wallet?.walletKeyringService = BraveWallet.KeyringServiceFactory.get(
      privateMode: tab.isPrivate
    )
    updateTabsBarVisibility()
  }

  func tabManager(_ tabManager: TabManager, willRemoveTab tab: some TabState) {
    tab.view.removeFromSuperview()
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState) {
    // tabDelegate is a weak ref (and the tab's webView may not be destroyed yet)
    // so we don't expcitly unset it.
    dismissSearchInput()
    updateTabsBarVisibility()
    tab.removeObserver(self)
    tab.removePolicyDecider(self)
  }

  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
  }

  func show(
    toast: Toast,
    afterWaiting delay: DispatchTimeInterval = SimpleToastUX.toastDelayBefore,
    duration: DispatchTimeInterval? = SimpleToastUX.toastDismissAfter
  ) {
    if let downloadToast = toast as? DownloadToast {
      self.downloadToast = downloadToast
    }

    if let searchResultAdClickedInfoBar = toast as? SearchResultAdClickedInfoBar {
      self.searchResultAdClickedInfoBar = searchResultAdClickedInfoBar
    }

    if let newTabTakeoverInfoBar = toast as? NewTabTakeoverInfoBar {
      self.newTabTakeoverInfoBar = newTabTakeoverInfoBar
    }

    // If BVC isnt visible hold on to this toast until viewDidAppear
    if view.window == nil {
      pendingToast = toast
      return
    }

    if toast is ButtonToast {
      if activeButtonToast != nil {
        activeButtonToast?.dismiss(false, animated: false)
      } else {
        activeButtonToast = toast
      }
    }

    toast.showToast(
      viewController: self,
      delay: delay,
      duration: duration,
      makeConstraints: { make in
        make.left.right.equalTo(self.view)
        make.bottom.equalTo(self.pageOverlayLayoutGuide)
      },
      completion: { [weak self] in
        if toast is ButtonToast {
          self?.activeButtonToast = nil
        }
      }
    )
  }

  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
    guard let toast = toast, !privateBrowsingManager.isPrivateBrowsing else {
      return
    }
    show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
  }
}
