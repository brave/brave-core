// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import BraveNews
import BraveShared
import BraveStrings
import BraveUI
import BraveWallet
import BraveWidgetsModels
import CertificateUtilities
import Data
import Lottie
import Playlist
import Preferences
import Shared
import SpeechRecognition
import Storage
import SwiftUI
import os.log

// MARK: - TopToolbarDelegate

extension BrowserViewController: TopToolbarDelegate {

  func showTabTray(isExternallyPresented: Bool = false) {
    if tabManager.tabsForCurrentMode.isEmpty {
      return
    }
    displayPageZoom(visible: false)

    if tabManager.selectedTab == nil {
      tabManager.selectTab(tabManager.tabsForCurrentMode.first)
    }
    if let tab = tabManager.selectedTab {
      screenshotHelper.takeScreenshot(tab)
    }

    isTabTrayActive = true

    let tabTrayController = TabTrayController(
      isExternallyPresented: isExternallyPresented,
      tabManager: tabManager,
      braveCore: braveCore,
      windowProtection: windowProtection
    ).then {
      $0.delegate = self
      $0.toolbarUrlActionsDelegate = self
    }
    let container = UINavigationController(rootViewController: tabTrayController)
    container.delegate = self

    if !UIAccessibility.isReduceMotionEnabled {
      if !isExternallyPresented {
        container.transitioningDelegate = tabTrayController
      }
      container.modalPresentationStyle = .fullScreen
    }
    present(container, animated: !isExternallyPresented)
  }

  func topToolbarDidPressReload(_ topToolbar: TopToolbarView) {
    if let url = topToolbar.currentURL {
      if let decentralizedDNSHelper = decentralizedDNSHelperFor(url: topToolbar.currentURL) {
        topToolbarDidPressReloadTask?.cancel()
        topToolbarDidPressReloadTask = Task { @MainActor in
          topToolbar.locationView.loading = true
          let result = await decentralizedDNSHelper.lookup(
            domain: url.schemelessAbsoluteDisplayString
          )
          topToolbar.locationView.loading = tabManager.selectedTab?.loading ?? false
          guard !Task.isCancelled else { return }  // user pressed stop, or typed new url
          switch result {
          case .loadInterstitial(let service):
            showWeb3ServiceInterstitialPage(service: service, originalURL: url)
          case .load(let resolvedURL):
            if resolvedURL.isIPFSScheme,
              let resolvedIPFSURL = braveCore.ipfsAPI.resolveGatewayUrl(for: resolvedURL)
            {
              tabManager.selectedTab?.loadRequest(URLRequest(url: resolvedIPFSURL))
            } else {
              tabManager.selectedTab?.loadRequest(URLRequest(url: resolvedURL))
            }
          case .none:
            tabManager.selectedTab?.reload()
          }
        }
      } else {
        tabManager.selectedTab?.reload()
      }
    } else {
      tabManager.selectedTab?.reload()
    }
  }

  func topToolbarDidPressStop(_ topToolbar: TopToolbarView) {
    stopTabToolbarLoading()
  }

  func topToolbarDidLongPressReloadButton(_ topToolbar: TopToolbarView, from button: UIButton) {
    guard let tab = tabManager.selectedTab, let url = tab.url, !url.isLocal,
      !url.isInternalURL(for: .readermode)
    else { return }

    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))

    let toggleActionTitle =
      tab.isDesktopSite == true
      ? Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
    alert.addAction(
      UIAlertAction(
        title: toggleActionTitle,
        style: .default,
        handler: { _ in
          tab.switchUserAgent()
        }
      )
    )

    UIImpactFeedbackGenerator(style: .heavy).vibrate()
    if UIDevice.current.userInterfaceIdiom == .pad {
      alert.popoverPresentationController?.sourceView = self.view
      alert.popoverPresentationController?.sourceRect = self.view.convert(
        button.frame,
        from: button.superview
      )
      alert.popoverPresentationController?.permittedArrowDirections = [.up]
    }
    present(alert, animated: true)
  }

  func topToolbarDidPressTabs(_ topToolbar: TopToolbarView) {
    showTabTray()
  }

  func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView) {
    toggleReaderMode()
  }

  func topToolbarDidPressPlaylistButton(_ urlBar: TopToolbarView) {
    guard let tab = tabManager.selectedTab, let playlistItem = tab.playlistItem else { return }
    let state = urlBar.locationView.playlistButton.buttonState
    switch state {
    case .addToPlaylist:
      addToPlaylist(item: playlistItem) { [weak self] didAddItem in
        guard let self else { return }

        if didAddItem {
          self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: playlistItem)

          DispatchQueue.main.async { [self] in
            let popover = self.createPlaylistPopover(item: playlistItem, tab: tab)
            popover.present(from: self.topToolbar.locationView.playlistButton, on: self)
          }
        }
      }
    case .addedToPlaylist:
      // Shows its own menu
      break
    case .none:
      break
    }
  }

  func topToolbarDidPressPlaylistMenuAction(
    _ urlBar: TopToolbarView,
    action: PlaylistURLBarButton.MenuAction
  ) {
    guard let tab = tabManager.selectedTab, let info = tab.playlistItem else { return }
    switch action {
    case .changeFolders:
      guard let item = PlaylistItem.getItem(uuid: info.tagId) else { return }
      let controller = PlaylistChangeFoldersViewController(item: item)
      self.present(controller, animated: true)
    case .openInPlaylist:
      DispatchQueue.main.async {
        self.openPlaylist(tab: tab, item: info)
      }
    case .remove:
      Task { @MainActor in
        if await PlaylistManager.shared.delete(item: info) {
          self.updatePlaylistURLBar(tab: tab, state: .newItem, item: info)
        }
      }
    case .undoRemove(let originalFolderUUID):
      addToPlaylist(item: info, folderUUID: originalFolderUUID)
    }
  }

  func topToolbarDisplayTextForURL(_ topToolbar: URL?) -> (String?, Bool) {
    // use the initial value for the URL so we can do proper pattern matching with search URLs
    var searchURL = self.tabManager.selectedTab?.currentInitialURL
    if let url = searchURL, InternalURL.isValid(url: url) {
      searchURL = url
    }
    if let query = profile.searchEngines.queryForSearchURL(
      searchURL as URL?,
      forType: privateBrowsingManager.isPrivateBrowsing ? .privateMode : .standard
    ) {
      return (query, true)
    } else {
      return (topToolbar?.absoluteString, false)
    }
  }

  func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView) {
    if let selectedTab = tabManager.selectedTab, favoritesController == nil {
      // Only scroll to top if we are not showing the home view controller
      selectedTab.webView?.scrollView.setContentOffset(CGPoint.zero, animated: true)
    }
  }

  func topToolbar(_ topToolbar: TopToolbarView, didEnterText text: String) {
    if text.isEmpty {
      hideSearchController()
    } else {
      showSearchController()

      Task {
        await searchController?.setSearchQuery(
          query: text,
          showSearchSuggestions: URLBarHelper.shared.shouldShowSearchSuggestions(
            using: topToolbar.locationLastReplacement
          )
        )
      }

      searchLoader?.query = text.lowercased()
    }
  }

  func topToolbar(_ topToolbar: TopToolbarView, didSubmitText text: String) {
    processAddressBar(text: text)
  }

  func processAddressBar(
    text: String,
    isBraveSearchPromotion: Bool = false,
    isUserDefinedURLNavigation: Bool = false
  ) {
    recordURLBarSubmitLocationP3A(from: tabManager.selectedTab)
    processAddressBarTask?.cancel()
    processAddressBarTask = Task { @MainActor in
      if !isBraveSearchPromotion,
        await submitValidURL(text, isUserDefinedURLNavigation: isUserDefinedURLNavigation)
      {
        return
      } else {
        // We couldn't build a URL, so pass it on to the search engine.
        submitSearchText(text, isBraveSearchPromotion: isBraveSearchPromotion)

        if !privateBrowsingManager.isPrivateBrowsing {
          RecentSearch.addItem(type: .text, text: text, websiteUrl: nil)
        }
      }
    }
  }

  @MainActor private func submitValidURL(
    _ text: String,
    isUserDefinedURLNavigation: Bool
  ) async -> Bool {

    if let url = URL(string: text), url.scheme == "brave" || url.scheme == "chrome" {
      topToolbar.leaveOverlayMode()
      return handleChromiumWebUIURL(url)
    }

    guard let fixupURL = URIFixup.getURL(text) else {
      return false
    }

    if fixupURL.scheme == "brave" || fixupURL.scheme == "chrome" {
      topToolbar.leaveOverlayMode()
      return handleChromiumWebUIURL(fixupURL)
    }

    // check text is decentralized DNS supported domain
    if let decentralizedDNSHelper = self.decentralizedDNSHelperFor(url: fixupURL) {
      topToolbar.leaveOverlayMode()
      updateToolbarCurrentURL(fixupURL)
      topToolbar.locationView.loading = true
      let result = await decentralizedDNSHelper.lookup(
        domain: fixupURL.schemelessAbsoluteDisplayString
      )
      topToolbar.locationView.loading = tabManager.selectedTab?.loading ?? false
      guard !Task.isCancelled else { return true }  // user pressed stop, or typed new url
      switch result {
      case .loadInterstitial(let service):
        showWeb3ServiceInterstitialPage(service: service, originalURL: fixupURL)
        return true
      case .load(let resolvedURL):
        if resolvedURL.isIPFSScheme,
          let resolvedIPFSURL = braveCore.ipfsAPI.resolveGatewayUrl(for: resolvedURL)
        {
          finishEditingAndSubmit(resolvedIPFSURL)
        } else {
          finishEditingAndSubmit(resolvedURL)
        }
        return true
      case .none:
        break
      }
    }

    // The user entered a URL, so use it.
    // Determine if url navigation is done from favourites or bookmarks
    // To handle bookmarklets properly
    finishEditingAndSubmit(fixupURL, isUserDefinedURLNavigation: isUserDefinedURLNavigation)
    return true
  }

  /// Handles displaying a Chromium web view for brave:// url that would display WebUI
  func handleChromiumWebUIURL(_ url: URL) -> Bool {
    let supportedPages = [
      "flags",
      "histograms",
      "local-state",
      "version",
      "skus-internals",
      "ads-internals",
    ]
    guard let host = url.host, supportedPages.contains(host) else {
      return false
    }
    let controller = ChromeWebViewController(privateBrowsing: false)
    controller.loadURL(url.absoluteString)
    controller.title = url.host?.capitalizeFirstLetter
    let webView = controller.webView
    webView.isFindInteractionEnabled = true
    controller.navigationItem.rightBarButtonItem = UIBarButtonItem(
      systemItem: .search,
      primaryAction: .init { [weak webView] _ in
        guard let findInteraction = webView?.findInteraction,
          !findInteraction.isFindNavigatorVisible
        else {
          return
        }
        findInteraction.searchText = ""
        findInteraction.presentFindNavigator(showingReplace: false)
      }
    )
    let container = UINavigationController(rootViewController: controller)
    controller.navigationItem.leftBarButtonItem = .init(
      systemItem: .done,
      primaryAction: .init { [unowned container] _ in
        container.dismiss(animated: true)
      }
    )
    self.present(container, animated: true)
    return true
  }

  func topToolbarDidEnterOverlayMode(_ topToolbar: TopToolbarView) {
    updateTabsBarVisibility()
    displayFavoritesController()
  }

  func topToolbarDidLeaveOverlayMode(_ topToolbar: TopToolbarView) {
    hideSearchController()
    hideFavoritesController()
    updateScreenTimeUrl(tabManager.selectedTab?.url)
    updateInContentHomePanel(tabManager.selectedTab?.url as URL?)
    updateTabsBarVisibility()
    if isUsingBottomBar {
      updateViewConstraints()
    }
  }

  func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView) {
    dismissVisibleMenus()
  }

  func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView) {
    presentBraveShieldsView()
  }

  func presentBraveShieldsView() {
    guard let selectedTab = tabManager.selectedTab, var url = selectedTab.url else { return }
    if let internalURL = InternalURL(url) {
      guard let orignalURL = internalURL.url.strippedInternalURL else { return }
      url = orignalURL
    }

    weak var weakPopover: PopoverController?
    let popover = PopoverController(
      contentController: PopoverNavigationController(
        rootViewController: ShieldsPanelViewController(
          url: url,
          tab: selectedTab,
          domain: Domain.getOrCreate(forUrl: url, persistent: !selectedTab.isPrivate)
        ) { [weak self, weak selectedTab] action in
          switch action {
          case .navigate(let target, let dismiss):
            guard let self, let selectedTab else { return }
            if dismiss {
              weakPopover?.dismiss(animated: true) {
                self.navigate(to: target, tab: selectedTab, url: url, on: nil)
              }
            } else {
              navigate(to: target, tab: selectedTab, url: url, on: weakPopover)
            }
          case .changedShieldSettings:
            self?.changedShieldSettings()
          case .shredSiteData:
            weakPopover?.dismiss(animated: true) {
              guard let selectedTab = selectedTab else { return }
              self?.shredData(for: url, in: selectedTab)
            }
          }
        }
      ),
      contentSizeBehavior: .preferredContentSize
    )
    weakPopover = popover
    popover.present(from: topToolbar.shieldsButton, on: self)
  }

  private func navigate(
    to target: ShieldsPanelView.Action.NavigationTarget,
    tab: Tab,
    url: URL,
    on viewController: UIViewController?
  ) {
    let presentingViewController = viewController ?? self
    switch target {
    case .shareStats:
      let activityController =
        ShieldsActivityItemSourceProvider.shared.setupGlobalShieldsActivityController(
          isPrivateBrowsing: tab.isPrivate
        )
      activityController.popoverPresentationController?.sourceView = topToolbar.shieldsButton
      presentingViewController.present(activityController, animated: true, completion: nil)
    case .globalShields:
      showGlobalShieldsSettings()
    case .reportBrokenSite:
      if let internalURL = InternalURL(url), let displayURL = internalURL.displayURL {
        showSubmitReportView(for: displayURL)
      } else {
        showSubmitReportView(for: url)
      }
    }
  }

  private func changedShieldSettings() {
    let currentDomain = self.tabManager.selectedTab?.url?.baseDomain
    let browsers = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene })
      .compactMap({ $0.browserViewController })

    browsers.forEach { browser in
      // Update the shields status immediately
      browser.topToolbar.refreshShieldsStatus()

      // Reload the tabs. This will also trigger an update of the brave icon in `TabLocationView` if
      // the setting changed is the global `.AllOff` shield
      browser.tabManager.allTabs.forEach {
        if $0.url?.baseDomain == currentDomain {
          $0.reload()
        }
      }
    }
  }

  private func showGlobalShieldsSettings() {
    weak var spinner: SpinnerView?
    let controller = UIHostingController(
      rootView: AdvancedShieldsSettingsView(
        settings: AdvancedShieldsSettings(
          profile: self.profile,
          tabManager: self.tabManager,
          feedDataSource: self.feedDataSource,
          debounceService: DebounceServiceFactory.get(privateMode: false),
          braveCore: braveCore,
          rewards: rewards,
          webcompatReporterHandler: WebcompatReporter.ServiceFactory.get(privateMode: false),
          clearDataCallback: { [weak self] isLoading, isHistoryCleared in
            guard let self else { return }
            guard let view = self.navigationController?.view, view.window != nil else {
              assertionFailure()
              return
            }

            if isLoading, spinner == nil {
              let newSpinner = SpinnerView()
              newSpinner.present(on: view)
              spinner = newSpinner
            } else {
              spinner?.dismiss()
              spinner = nil
            }

            if isHistoryCleared {
              // Donate Clear Browser History for suggestions
              let clearBrowserHistoryActivity = ActivityShortcutManager.shared
                .createShortcutActivity(type: .clearBrowsingHistory)
              self.userActivity = clearBrowserHistoryActivity
              clearBrowserHistoryActivity.becomeCurrent()
            }
          }
        )
      )
    )

    controller.rootView.openURLAction = { [unowned self] url in
      openDestinationURL(url)
    }

    let container = SettingsNavigationController(rootViewController: controller)
    container.isModalInPresentation = true
    container.modalPresentationStyle =
      UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    controller.navigationItem.rightBarButtonItem = .init(
      barButtonSystemItem: .done,
      target: container,
      action: #selector(SettingsNavigationController.done)
    )
    self.present(container, animated: true)
  }

  func shredData(for url: URL, in tab: Tab) {
    LottieAnimationView.showShredAnimation(on: view) {
      self.tabManager.shredData(for: url, in: tab)
    }
  }

  func showSubmitReportView(for url: URL) {
    // Strip fragments and query params from url
    var components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    components?.fragment = nil
    components?.queryItems = nil
    guard let cleanedURL = components?.url else { return }

    let viewController = UIHostingController(
      rootView: SubmitReportView(
        url: cleanedURL,
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      )
    )

    viewController.modalPresentationStyle = .popover

    if let popover = viewController.popoverPresentationController {
      popover.sourceView = topToolbar.shieldsButton
      popover.sourceRect = topToolbar.shieldsButton.bounds

      let sheet = popover.adaptiveSheetPresentationController
      sheet.largestUndimmedDetentIdentifier = .medium
      sheet.prefersEdgeAttachedInCompactHeight = true
      sheet.widthFollowsPreferredContentSizeWhenEdgeAttached = true
      sheet.detents = [.medium(), .large()]
      sheet.prefersGrabberVisible = true
    }
    navigationController?.present(viewController, animated: true)
  }

  // TODO: This logic should be fully abstracted away and share logic from current MenuViewController
  // See: https://github.com/brave/brave-ios/issues/1452
  func topToolbarDidTapShortcutButton(_ topToolbar: TopToolbarView) {
    guard
      let shortcut = Preferences.General.toolbarShortcutButton.value.flatMap(WidgetShortcut.init)
    else {
      return
    }
    NavigationPath.handleWidgetShortcut(shortcut, with: self)
  }

  func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView) {
    showBraveRewardsPanel()
  }

  func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView) {
    tabToolbarDidPressMenu(topToolbar)
  }

  func topToolbarDidPressQrCodeButton(_ urlBar: TopToolbarView) {
    scanQRCode()
  }

  func topToolbarDidPressVoiceSearchButton(_ urlBar: TopToolbarView) {
    Task { @MainActor in
      onPendingRequestUpdatedCancellable = speechRecognizer.$finalizedRecognition.sink {
        [weak self] finalizedRecognition in
        guard let self else { return }

        if let finalizedRecognition {
          // Feedback indicating recognition is finalized
          AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))
          UIImpactFeedbackGenerator(style: .medium).vibrate()
          stopVoiceSearch(searchQuery: finalizedRecognition)
        }
      }

      let permissionStatus = await speechRecognizer.askForUserPermission()

      if permissionStatus {
        openVoiceSearch(speechRecognizer: speechRecognizer)
      } else {
        showNoMicrophoneWarning()
      }
    }

    func openVoiceSearch(speechRecognizer: SpeechRecognizer) {
      // Pause active playing in PiP when Audio Search is enabled
      if let pipMediaPlayer = PlaylistCoordinator.shared.mediaPlayer?.pictureInPictureController?
        .playerLayer.player
      {
        pipMediaPlayer.pause()
      }

      voiceSearchViewController = PopupViewController(
        rootView: SpeechToTextInputView(
          speechModel: speechRecognizer,
          disclaimer: Strings.VoiceSearch.screenDisclaimer
        )
      )

      if let voiceSearchController = voiceSearchViewController {
        voiceSearchController.modalTransitionStyle = .crossDissolve
        voiceSearchController.modalPresentationStyle = .overFullScreen
        present(voiceSearchController, animated: true)
      }
    }

    func showNoMicrophoneWarning() {
      let alertController = UIAlertController(
        title: Strings.VoiceSearch.microphoneAccessRequiredWarningTitle,
        message: Strings.VoiceSearch.microphoneAccessRequiredWarningDescription,
        preferredStyle: .alert
      )

      let settingsAction = UIAlertAction(
        title: Strings.settings,
        style: .default
      ) { _ in
        let url = URL(string: UIApplication.openSettingsURLString)!
        UIApplication.shared.open(url, options: [:], completionHandler: nil)
      }

      let cancelAction = UIAlertAction(title: Strings.CancelString, style: .cancel, handler: nil)

      alertController.addAction(settingsAction)
      alertController.addAction(cancelAction)

      present(alertController, animated: true)
    }
  }

  func stopVoiceSearch(searchQuery: String? = nil) {
    voiceSearchViewController?.dismiss(animated: true) {
      if let query = searchQuery {
        self.submitSearchText(query)
      }

      self.speechRecognizer.clearSearch()
    }
  }

  func topToolbarDidTapWalletButton(_ urlBar: TopToolbarView) {
    guard let selectedTab = tabManager.selectedTab else {
      return
    }
    // System components sit on top so we want to dismiss it
    selectedTab.webView?.findInteraction?.dismissFindNavigator()
    presentWalletPanel(from: selectedTab.getOrigin(), with: selectedTab.tabDappStore)
  }

  private func hideSearchController() {
    if let searchController = searchController {
      searchController.willMove(toParent: nil)
      searchController.view.removeFromSuperview()
      searchController.removeFromParent()
      self.searchController = nil
      searchLoader = nil
      favoritesController?.view.isHidden = false
    }
  }

  private func showSearchController() {
    if searchController != nil { return }

    // Setting up data source for SearchSuggestions
    let tabType = TabType.of(tabManager.selectedTab)
    let searchDataSource = SearchSuggestionDataSource(
      forTabType: tabType,
      searchEngines: profile.searchEngines
    )

    // Setting up controller for SearchSuggestions
    searchController = SearchViewController(
      with: searchDataSource,
      browserColors: privateBrowsingManager.browserColors
    )
    searchController?.isUsingBottomBar = isUsingBottomBar
    guard let searchController = searchController else { return }
    searchController.setupSearchEngineList()
    searchController.searchDelegate = self
    searchController.profile = self.profile

    searchLoader = SearchLoader(
      historyAPI: braveCore.historyAPI,
      bookmarkManager: bookmarkManager,
      tabManager: tabManager
    )
    searchLoader?.addListener(searchController)
    searchLoader?.autocompleteSuggestionHandler = { [weak self] completion in
      self?.topToolbar.setAutocompleteSuggestion(completion)
    }

    addChild(searchController)
    if let favoritesController = favoritesController {
      view.insertSubview(searchController.view, aboveSubview: favoritesController.view)
    } else {
      view.insertSubview(searchController.view, belowSubview: header)
    }
    searchController.view.snp.makeConstraints {
      $0.edges.equalTo(view)
    }
    searchController.didMove(toParent: self)
    searchController.view.setNeedsLayout()
    searchController.view.layoutIfNeeded()

    favoritesController?.view.isHidden = true
  }

  func insertFavoritesControllerView(favoritesController: FavoritesViewController) {
    if let ntpController = self.activeNewTabPageViewController, ntpController.parent != nil {
      view.insertSubview(favoritesController.view, aboveSubview: ntpController.view)
    } else {
      // Two different behaviors here:
      // 1. For bottom bar we do not want to show the status bar color
      // 2. For top bar we do so it matches the address bar background
      let subview = isUsingBottomBar ? statusBarOverlay : footer
      view.insertSubview(favoritesController.view, aboveSubview: subview)
    }
  }

  private func displayFavoritesController() {
    if favoritesController == nil {
      let favoritesController = FavoritesViewController(
        privateBrowsingManager: privateBrowsingManager,
        bookmarkAction: { [weak self] bookmark, action in
          self?.handleFavoriteAction(favorite: bookmark, action: action)
        },
        recentSearchAction: { [weak self] recentSearch, shouldSubmitSearch in
          guard let self = self else { return }

          let submitSearch = { [weak self] (text: String) in
            if let fixupURL = URIFixup.getURL(text) {
              self?.finishEditingAndSubmit(fixupURL)
              return
            }

            self?.submitSearchText(text)
          }

          if let recentSearch = recentSearch,
            let searchType = RecentSearchType(rawValue: recentSearch.searchType)
          {
            if shouldSubmitSearch {
              recentSearch.update(dateAdded: Date())
            }

            switch searchType {
            case .text, .website:
              if let text = recentSearch.text {
                self.topToolbar.setLocation(text, search: false)
                self.topToolbar(self.topToolbar, didEnterText: text)

                if shouldSubmitSearch {
                  submitSearch(text)
                }
              }
            case .qrCode:
              if let text = recentSearch.text {
                self.topToolbar.setLocation(text, search: false)
                self.topToolbar(self.topToolbar, didEnterText: text)

                if shouldSubmitSearch {
                  submitSearch(text)
                }
              } else if let websiteUrl = recentSearch.websiteUrl {
                self.topToolbar.setLocation(websiteUrl, search: false)
                self.topToolbar(self.topToolbar, didEnterText: websiteUrl)

                if shouldSubmitSearch {
                  submitSearch(websiteUrl)
                }
              }
            }
          } else if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs,
            let searchQuery = UIPasteboard.general.string
              ?? UIPasteboard.general.url?.absoluteString
          {

            self.topToolbar.setLocation(searchQuery, search: false)
            self.topToolbar(self.topToolbar, didEnterText: searchQuery)

            if shouldSubmitSearch {
              submitSearch(searchQuery)
            }
          }
        }
      )
      self.favoritesController = favoritesController

      addChild(favoritesController)
      insertFavoritesControllerView(favoritesController: favoritesController)
      favoritesController.didMove(toParent: self)

      favoritesController.view.snp.makeConstraints {
        $0.leading.trailing.equalTo(pageOverlayLayoutGuide)
        $0.top.bottom.equalTo(view)
      }
      favoritesController.view.setNeedsLayout()
      favoritesController.view.layoutIfNeeded()
    }
    guard let favoritesController = favoritesController else { return }
    favoritesController.view.alpha = 0.0
    let animator = UIViewPropertyAnimator(duration: 0.2, dampingRatio: 1.0) {
      favoritesController.view.alpha = 1
    }
    animator.addCompletion { _ in
      self.webViewContainer.accessibilityElementsHidden = true
      UIAccessibility.post(notification: .screenChanged, argument: nil)
    }
    animator.startAnimation()
  }

  private func hideFavoritesController() {
    guard let controller = favoritesController else { return }
    self.favoritesController = nil
    UIView.animate(
      withDuration: 0.1,
      delay: 0,
      options: [.beginFromCurrentState],
      animations: {
        controller.view.alpha = 0.0
      },
      completion: { _ in
        controller.willMove(toParent: nil)
        controller.view.removeFromSuperview()
        controller.removeFromParent()
        self.webViewContainer.accessibilityElementsHidden = false
        UIAccessibility.post(notification: .screenChanged, argument: nil)
      }
    )
  }

  func openAddBookmark() {
    guard let selectedTab = tabManager.selectedTab,
      let selectedUrl = selectedTab.url,
      !(selectedUrl.isLocal || selectedUrl.isInternalURL(for: .readermode))
    else {
      return
    }

    let bookmarkUrl = selectedUrl.decodeEmbeddedInternalURL(for: .readermode) ?? selectedUrl

    let mode = BookmarkEditMode.addBookmark(
      title: selectedTab.displayTitle,
      url: bookmarkUrl.absoluteString
    )

    let addBookMarkController = AddEditBookmarkTableViewController(
      bookmarkManager: bookmarkManager,
      mode: mode,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
    presentSettingsNavigation(with: addBookMarkController, cancelEnabled: true)
  }

  func presentSettingsNavigation(with controller: UIViewController, cancelEnabled: Bool = false) {
    let navigationController = SettingsNavigationController(rootViewController: controller)
    navigationController.modalPresentationStyle = .formSheet

    let cancelBarbutton = UIBarButtonItem(
      barButtonSystemItem: .cancel,
      target: navigationController,
      action: #selector(SettingsNavigationController.done)
    )

    let doneBarbutton = UIBarButtonItem(
      barButtonSystemItem: .done,
      target: navigationController,
      action: #selector(SettingsNavigationController.done)
    )

    navigationController.navigationBar.topItem?.leftBarButtonItem =
      cancelEnabled ? cancelBarbutton : nil

    navigationController.navigationBar.topItem?.rightBarButtonItem = doneBarbutton

    present(navigationController, animated: true)
  }
}

extension BrowserViewController: ToolbarDelegate {
  func tabToolbarDidPressSearch(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
  }

  func tabToolbarDidPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    tabManager.selectedTab?.goBack()
    resetExternalAlertProperties(tabManager.selectedTab)
    recordNavigationActionP3A(isNavigationActionForward: false)
  }

  func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    UIImpactFeedbackGenerator(style: .heavy).vibrate()
    showBackForwardList()
  }

  func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    tabManager.selectedTab?.goForward()
    resetExternalAlertProperties(tabManager.selectedTab)
    recordNavigationActionP3A(isNavigationActionForward: true)
  }

  func tabToolbarDidPressShare() {
    navigationHelper.openShareSheet()
  }

  func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol) {
    let selectedTabURL: URL? = {
      guard let url = tabManager.selectedTab?.url else { return nil }

      if let internalURL = InternalURL(url) {
        if internalURL.isErrorPage {
          return internalURL.originalURLFromErrorPage
        }
        if internalURL.isReaderModePage {
          return internalURL.extractedUrlParam
        }
        return nil
      }
      return url
    }()

    displayPageZoom(visible: false)

    var activities: [UIActivity] = []
    if let url = selectedTabURL, let tab = tabManager.selectedTab {
      activities = makeShareActivities(
        for: url,
        tab: tab,
        sourceView: view,
        sourceRect: self.view.convert(
          self.topToolbar.menuButton.frame,
          from: self.topToolbar.menuButton.superview
        ),
        arrowDirection: .up
      )
    }
    let initialHeight: CGFloat = selectedTabURL != nil ? 470 : 500
    let menuController = MenuViewController(
      initialHeight: initialHeight,
      content: { menuController in
        let isShownOnWebPage = selectedTabURL != nil
        VStack(spacing: 6) {
          if isShownOnWebPage {
            featuresMenuSection(menuController)
          } else {
            privacyFeaturesMenuSection(menuController)
          }
          Divider()
          destinationMenuSection(menuController, isShownOnWebPage: isShownOnWebPage)
          if let tabURL = selectedTabURL {
            Divider()
            PageActionsMenuSection(
              browserViewController: self,
              tabURL: tabURL,
              activities: activities
            )
          }
        }
        .navigationBarHidden(true)
      }
    )
    presentPanModal(
      menuController,
      sourceView: tabToolbar.menuButton,
      sourceRect: tabToolbar.menuButton.bounds
    )
    if menuController.modalPresentationStyle == .popover {
      menuController.popoverPresentationController?.popoverLayoutMargins = .init(equalInset: 4)
      menuController.popoverPresentationController?.permittedArrowDirections = [.up, .down]
    }
  }

  func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    recordCreateTabAction(location: .toolbar)
    self.openBlankNewTab(
      attemptLocationFieldFocus: false,
      isPrivate: privateBrowsingManager.isPrivateBrowsing
    )
  }

  func tabToolbarDidLongPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    UIImpactFeedbackGenerator(style: .heavy).vibrate()
    showBackForwardList()
  }

  func tabToolbarDidPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    showTabTray()
  }

  func topToolbarDidTapSecureContentState(_ urlBar: TopToolbarView) {
    guard let tab = tabManager.selectedTab, let url = tab.url,
      let secureContentStateButton = urlBar.locationView.secureContentStateButton
    else { return }
    let hasCertificate =
      (tab.webView?.serverTrust ?? (try? ErrorPageHelper.serverTrust(from: url))) != nil
    let pageSecurityView = PageSecurityView(
      displayURL: urlBar.locationView.urlDisplayLabel.text ?? url.absoluteDisplayString,
      secureState: tab.lastKnownSecureContentState,
      hasCertificate: hasCertificate,
      presentCertificateViewer: { [weak self] in
        self?.dismiss(animated: true)
        self?.displayPageCertificateInfo()
      }
    )
    let popoverController = PopoverController(content: pageSecurityView)
    popoverController.present(from: secureContentStateButton, on: self)
  }

  func showBackForwardList() {
    if let backForwardList = tabManager.selectedTab?.webView?.backForwardList {
      let backForwardViewController = BackForwardListViewController(
        profile: profile,
        backForwardList: backForwardList
      )
      backForwardViewController.tabManager = tabManager
      backForwardViewController.bvc = self
      backForwardViewController.modalPresentationStyle = .overCurrentContext
      backForwardViewController.backForwardTransitionDelegate = BackForwardListAnimator()
      backForwardViewController.toolbarUrlActionsDelegate = self
      self.present(backForwardViewController, animated: true, completion: nil)
    }
  }

  func tabToolbarDidSwipeToChangeTabs(
    _ tabToolbar: ToolbarProtocol,
    direction: UISwipeGestureRecognizer.Direction
  ) {
    let tabs = tabManager.tabsForCurrentMode
    guard let selectedTab = tabManager.selectedTab,
      let index = tabs.firstIndex(where: { $0 === selectedTab })
    else { return }
    let newTabIndex = index + (direction == .left ? -1 : 1)
    if newTabIndex >= 0 && newTabIndex < tabs.count {
      tabManager.selectTab(tabs[newTabIndex])
    }
  }

  func stopTabToolbarLoading() {
    tabManager.selectedTab?.stop()
    processAddressBarTask?.cancel()
    topToolbarDidPressReloadTask?.cancel()
    topToolbar.locationView.loading = tabManager.selectedTab?.loading ?? false
  }
}

extension BrowserViewController: UIContextMenuInteractionDelegate {
  public func contextMenuInteraction(
    _ interaction: UIContextMenuInteraction,
    configurationForMenuAtLocation location: CGPoint
  ) -> UIContextMenuConfiguration? {
    let configuration = UIContextMenuConfiguration(identifier: nil, previewProvider: nil) {
      [unowned self] _ in
      let actionMenus: [UIMenu?] = [
        makePasteMenu(), makeCopyMenu(), makeReloadMenu(),
      ]

      return UIMenu(children: actionMenus.compactMap({ $0 }))
    }
    configuration.preferredMenuElementOrder = .priority
    return configuration
  }

  /// Create the "Request Destop Site" / "Request Mobile Site" menu if the tab has a webpage loaded
  private func makeReloadMenu() -> UIMenu? {
    guard let tab = tabManager.selectedTab, let url = tab.url, url.isWebPage() else { return nil }
    let reloadTitle =
      tab.isDesktopSite == true
      ? Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
    let reloadIcon = tab.isDesktopSite == true ? "leo.smartphone" : "leo.monitor"
    let reloadAction = UIAction(
      title: reloadTitle,
      image: UIImage(braveSystemNamed: reloadIcon),
      handler: UIAction.deferredActionHandler { [weak tab] _ in
        tab?.switchUserAgent()
      }
    )

    return UIMenu(options: .displayInline, children: [reloadAction])
  }

  /// Create the "Paste"  and "Paste and Go" menu if there is anything on the `UIPasteboard`
  private func makePasteMenu() -> UIMenu? {
    guard UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs else { return nil }

    let children: [UIAction] = [
      UIAction(
        identifier: .pasteAndGo,
        handler: UIAction.deferredActionHandler { _ in
          if let pasteboardContents = UIPasteboard.general.string {
            self.topToolbar(self.topToolbar, didSubmitText: pasteboardContents)
          }
        }
      ),
      UIAction(
        identifier: .paste,
        handler: UIAction.deferredActionHandler { _ in
          if let pasteboardContents = UIPasteboard.general.string {
            self.topToolbar.enterOverlayMode(pasteboardContents, pasted: true, search: true)
          }
        }
      ),
    ]

    return UIMenu(options: .displayInline, children: children)
  }

  /// Create the "Copy Link" and "Copy Clean Link" menu if there is any URL loaded on the tab.
  ///
  /// - Note: "Copy Clean Link" will be included even if no cleaning is done to the url.
  private func makeCopyMenu() -> UIMenu? {
    let tab = tabManager.selectedTab
    guard let url = self.topToolbar.currentURL else { return nil }

    let children: [UIAction] = [
      UIAction(
        title: Strings.copyLinkActionTitle,
        image: UIImage(systemName: "doc.on.doc"),
        handler: UIAction.deferredActionHandler { _ in
          UIPasteboard.general.url = url as URL
        }
      ),
      UIAction(
        title: Strings.copyCleanLink,
        image: UIImage(braveSystemNamed: "leo.broom"),
        handler: UIAction.deferredActionHandler { _ in
          let service = URLSanitizerServiceFactory.get(privateMode: tab?.isPrivate ?? true)
          let cleanedURL = service?.sanitizeURL(url) ?? url
          UIPasteboard.general.url = cleanedURL
        }
      ),
    ]

    return UIMenu(options: .displayInline, children: children)
  }
}

// MARK: UINavigationControllerDelegate

extension BrowserViewController: UINavigationControllerDelegate {
  public func navigationControllerSupportedInterfaceOrientations(
    _ navigationController: UINavigationController
  ) -> UIInterfaceOrientationMask {
    return navigationController.visibleViewController?.supportedInterfaceOrientations
      ?? navigationController.supportedInterfaceOrientations
  }

  public func navigationControllerPreferredInterfaceOrientationForPresentation(
    _ navigationController: UINavigationController
  ) -> UIInterfaceOrientation {
    return navigationController.visibleViewController?.preferredInterfaceOrientationForPresentation
      ?? navigationController.preferredInterfaceOrientationForPresentation
  }
}
