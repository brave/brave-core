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
import BrowserMenu
import CertificateUtilities
import Data
import Lottie
import Onboarding
import OrderedCollections
import Playlist
import Preferences
import Shared
import SpeechRecognition
import Storage
import SwiftUI
import Web
import os.log

// MARK: - TopToolbarDelegate

extension BrowserViewController: TopToolbarDelegate, SearchContainerViewControllerDelegate {

  func showTabTray() {
    if tabManager.tabsForCurrentMode.isEmpty {
      return
    }
    clearPageZoomDialog()

    if tabManager.selectedTab == nil {
      tabManager.selectTab(tabManager.tabsForCurrentMode.first)
    }
    if let tab = tabManager.selectedTab {
      screenshotHelper.takeScreenshot(tab)
    }

    isTabTrayActive = true

    let tabTrayController = TabGridHostingController(
      tabManager: tabManager,
      historyModel: HistoryModel(
        api: self.profileController.historyAPI,
        tabManager: self.tabManager,
        toolbarUrlActionsDelegate: self,
        dismiss: { [weak self] in self?.dismiss(animated: true) },
        askForAuthentication: self.askForLocalAuthentication,
        serpMetrics: SerpMetricsServiceFactory.get(profile: self.profileController.profile)
      ),
      openTabsModel: profileController.openTabsAPI,
      toolbarUrlActionsDelegate: self,
      profileController: profileController,
      windowProtection: windowProtection,
      didAddTab: { [weak self] in
        if Preferences.General.openKeyboardOnNTPSelection.value {
          self?.focusURLBar()
        }
      }
    )
    tabTrayController.modalPresentationStyle = .fullScreen
    if !UIAccessibility.isReduceMotionEnabled {
      tabTrayController.transitioningDelegate = tabTrayController
    }
    present(tabTrayController, animated: true)
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
          topToolbar.locationView.loading = tabManager.selectedTab?.isLoading == true
          guard !Task.isCancelled else { return }  // user pressed stop, or typed new url
          switch result {
          case .loadInterstitial(let service):
            showWeb3ServiceInterstitialPage(service: service, originalURL: url)
          case .load(let resolvedURL):
            if resolvedURL.isIPFSScheme,
              let resolvedIPFSURL = profileController.ipfsAPI.resolveGatewayUrl(for: resolvedURL)
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
    guard let tab = tabManager.selectedTab, let url = tab.visibleURL, !url.isLocal,
      !url.isInternalURL(for: .readermode)
    else { return }

    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))

    let toggleActionTitle =
      tab.currentUserAgentType == .desktop
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
    tabManager.selectedTab?.readerMode?.toggleReaderMode()
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
    let searchURL = self.tabManager.selectedTab?.currentInitialURL
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
    if let selectedTab = tabManager.selectedTab, searchContainer == nil {
      // Only scroll to top if we are not showing the home view controller
      selectedTab.webViewProxy?.scrollView?.setContentOffset(CGPoint.zero, animated: true)
    }
  }

  func topToolbarDidRequestSearchInput(
    _ topToolbar: TopToolbarView,
    initialText: String?,
    pasted: Bool,
    search: Bool
  ) {
    presentSearchInput(initialText: initialText, pasted: pasted, search: search)
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

  func topToolbarDidPressTranslateButton(_ urlBar: TopToolbarView) {
    guard let tab = tabManager.selectedTab else { return }

    if let translateTabHelper = tab.translate {
      translateTabHelper.toggleTranslation()
    }

    if let translateHelper = tab.legacyTranslateHelper {
      translateHelper.presentUI(on: self)

      if translateHelper.translationState == .active {
        translateHelper.revertTranslation()
      } else if translateHelper.translationState != .active {
        translateHelper.startTranslation(canShowToast: true)
      }
    }
  }

  func topToolbarIsShieldsEnabled(_ topToolbar: TopToolbarView, for url: URL?) -> Bool {
    guard let url, let currentTab = self.tabManager.selectedTab else { return false }
    return currentTab.braveShieldsHelper?.isBraveShieldsEnabled(for: url) ?? false
  }

  @MainActor private func submitValidURL(
    _ text: String,
    isUserDefinedURLNavigation: Bool
  ) async -> Bool {

    if let url = URL(string: text), url.scheme == "brave" || url.scheme == "chrome" {
      dismissSearchInput()
      finishEditingAndSubmit(url, isUserDefinedURLNavigation: isUserDefinedURLNavigation)
      return true
    }

    guard let fixupURL = URIFixup.getURL(text) else {
      return false
    }

    // check text is decentralized DNS supported domain
    if let decentralizedDNSHelper = self.decentralizedDNSHelperFor(url: fixupURL) {
      dismissSearchInput()
      updateToolbarCurrentURL(fixupURL)
      topToolbar.locationView.loading = true
      let result = await decentralizedDNSHelper.lookup(
        domain: fixupURL.schemelessAbsoluteDisplayString
      )
      topToolbar.locationView.loading = tabManager.selectedTab?.isLoading == true
      guard !Task.isCancelled else { return true }  // user pressed stop, or typed new url
      switch result {
      case .loadInterstitial(let service):
        showWeb3ServiceInterstitialPage(service: service, originalURL: fixupURL)
        return true
      case .load(let resolvedURL):
        if resolvedURL.isIPFSScheme,
          let resolvedIPFSURL = profileController.ipfsAPI.resolveGatewayUrl(for: resolvedURL)
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

  var isSearchContainerVisible: Bool {
    searchContainer?.parent == self
  }

  /// Presents the fullscreen search input over the browser. The toolbar and web view remain static.
  ///
  /// - Parameters:
  ///   - initialText: The text to seed the input field with (the current URL or search query).
  ///   - pasted: Whether the `initialText` was pasted (avoids highlighting the whole entry).
  ///   - search: Whether `initialText` should be treated as a search query.
  func presentSearchInput(initialText: String?, pasted: Bool, search: Bool) {
    if let searchContainer {
      // Already editing - just seed the existing input (e.g. from a QR scan) and refocus.
      searchContainer.applyExternalQuery(initialText ?? "", search: search)
      searchContainer.inputBar.becomeFirstResponder()
      return
    }

    let isPrivate = tabManager.selectedTab?.isPrivate ?? false
    let container = SearchContainerViewController(
      tabManager: tabManager,
      bookmarkManager: bookmarkManager,
      historyAPI: profileController.historyAPI,
      searchEngines: profile.searchEngines,
      privateBrowsingManager: privateBrowsingManager,
      speechRecognizer: speechRecognizer,
      isAIChatAvailable: !isPrivate && Preferences.AIChat.leoInQuickSearchBarEnabled.value
        && AIChatUtils.isAIChatEnabled(for: profileController.profile.prefs),
      isPlaylistAvailable: profileController.profile.prefs.isPlaylistAvailable,
      searchDelegate: self,
      delegate: self,
      bookmarkAction: { [weak self] bookmark, action in
        self?.handleFavoriteAction(favorite: bookmark, action: action)
      },
      recentSearchAction: { [weak self] recentSearch, shouldSubmitSearch in
        self?.handleRecentSearch(recentSearch, shouldSubmitSearch: shouldSubmitSearch)
      }
    )
    container.isUsingBottomBar = isUsingBottomBar
    self.searchContainer = container

    // Presented fullscreen above the toolbar so the toolbar and web view remain static, and so the
    // browser is never visible behind it (including with a hardware keyboard connected).
    addChild(container)
    view.addSubview(container.view)
    container.view.snp.makeConstraints {
      $0.edges.equalTo(view)
    }
    container.didMove(toParent: self)
    container.view.setNeedsLayout()
    container.view.layoutIfNeeded()

    container.view.alpha = 0
    UIViewPropertyAnimator.runningPropertyAnimator(withDuration: 0.1, delay: 0) {
      container.view.alpha = 1
    }

    webViewContainer.accessibilityElementsHidden = true
    UIAccessibility.post(notification: .screenChanged, argument: nil)

    // Pasted content is treated as a search query so suggestions appear immediately, and is not
    // selected (the cursor stays at the end).
    container.beginEditing(
      with: initialText,
      search: pasted ? true : search,
      selectAll: !pasted
    )
  }

  /// Dismisses the fullscreen search input and restores the browsing UI.
  func dismissSearchInput() {
    guard isSearchContainerVisible else { return }

    if let searchContainer {
      searchContainer.inputBar.resignFirstResponder()
      UIViewPropertyAnimator.runningPropertyAnimator(withDuration: 0.05, delay: 0) {
        searchContainer.view.alpha = 0
      } completion: { _ in
        searchContainer.willMove(toParent: nil)
        searchContainer.view.removeFromSuperview()
        searchContainer.removeFromParent()
      }
    }
    searchContainer = nil

    webViewContainer.accessibilityElementsHidden = false
    UIAccessibility.post(notification: .screenChanged, argument: nil)

    updateScreenTimeUrl(tabManager.selectedTab?.visibleURL)
    updateInContentHomePanel(tabManager.selectedTab?.visibleURL as URL?)
    updateTabsBarVisibility()
    topToolbar.updateViewsForToolbarChanges()
    activeNewTabPageViewController?.searchContainerDidDismiss()
  }

  // MARK: - SearchContainerViewControllerDelegate

  func searchContainer(_ container: SearchContainerViewController, didSubmitText text: String) {
    processAddressBar(text: text)
  }

  func searchContainerDidCancel(_ container: SearchContainerViewController) {
    dismissSearchInput()
  }

  func searchContainerDidTapPasteAndGo(_ container: SearchContainerViewController) {
    topToolbarDidPressPasteAndGoButton(topToolbar)
  }

  func searchContainerDidTapQRCode(_ container: SearchContainerViewController) {
    scanQRCode()
  }

  func searchContainerDidTapVoiceSearch(_ container: SearchContainerViewController) {
    topToolbarDidPressVoiceSearchButton(topToolbar)
  }

  func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView) {
  }

  func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView) {
    presentBraveShieldsView()
  }

  func presentBraveShieldsView() {
    guard let selectedTab = tabManager.selectedTab, var url = selectedTab.visibleURL else { return }
    if let internalURL = InternalURL(url) {
      guard let orignalURL = internalURL.url.strippedInternalURL else { return }
      url = orignalURL
    }
    if !url.isWebPage(includeDataURIs: false) {
      return
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
    tab: some TabState,
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
    let currentDomain = self.tabManager.selectedTab?.visibleURL?.baseDomain
    let browsers = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene })
      .compactMap({ $0.browserViewController })

    browsers.forEach { browser in
      // Update the shields status immediately
      browser.topToolbar.refreshShieldsStatus()

      // Reload the tabs. This will also trigger an update of the brave icon in `TabLocationView` if
      // the setting changed is the global `.AllOff` shield
      browser.tabManager.allTabs.forEach {
        if $0.visibleURL?.baseDomain == currentDomain {
          $0.reload()
          // Domain specific shield setting changed, reset selectors cache.
          $0.cosmeticFilteringTabHelper?.resetSelectorsCache()
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
          braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(
            profile: profileController.profile
          ),
          braveCore: profileController,
          p3aUtils: braveCore.p3aUtils,
          localState: braveCore.localState,
          rewards: rewards,
          braveStats: profileController.braveStats,
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
    container.modalPresentationStyle =
      UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    controller.navigationItem.rightBarButtonItem = .doneButton(
      target: container,
      action: #selector(SettingsNavigationController.done)
    )
    self.present(container, animated: true)
  }

  func shredData(for url: URL, in tab: some TabState) {
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
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing,
        tab: tabManager.selectedTab
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

  func topToolbarAvailableShortcutButtons(
    _ topToolbar: TopToolbarView
  ) -> OrderedSet<WidgetShortcut> {
    return WidgetShortcut.eligibleButtonShortcuts(
      prefs: profileController.profile.prefs,
      isWalletAvailable: profileController.braveWalletAPI.isAllowed
    )
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

      let permissionStatus = await SpeechRecognizer.requestPermission()

      if permissionStatus {
        openVoiceSearch(speechRecognizer: speechRecognizer)
      } else {
        showNoMicrophoneWarning()
      }
    }

    func openVoiceSearch(speechRecognizer: SpeechRecognizer) {
      // Pause active playing in PiP when Audio Search is enabled
      if PlaylistCoordinator.shared.isPictureInPictureActive {
        PlaylistCoordinator.shared.pauseAllPlayback()
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

  func topToolbarDidPressPasteAndGoButton(_ urlBar: TopToolbarView) {
    if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs,
      let searchQuery = UIPasteboard.general.string
        ?? UIPasteboard.general.url?.absoluteString
    {
      if let fixupURL = URIFixup.getURL(searchQuery) {
        finishEditingAndSubmit(fixupURL)
        return
      }

      self.submitSearchText(searchQuery)
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
    guard let selectedTab = tabManager.selectedTab,
      let tabDappStore = selectedTab.wallet?.tabDappStore,
      let origin = selectedTab.lastCommittedURL?.origin
    else {
      return
    }
    // System components sit on top so we want to dismiss it
    selectedTab.dismissFindInteraction()
    presentWalletPanel(from: origin, with: tabDappStore)
  }

  /// Handles selection of a recent search in the favorites screen: seeds the input field and, when
  /// requested, navigates. Passed to the search container as its favorites `recentSearchAction`.
  private func handleRecentSearch(_ recentSearch: RecentSearch?, shouldSubmitSearch: Bool) {
    let submitSearch = { [weak self] (text: String) in
      if let fixupURL = URIFixup.getURL(text) {
        self?.finishEditingAndSubmit(fixupURL)
        return
      }

      self?.submitSearchText(text)
    }

    guard let recentSearch = recentSearch,
      let searchType = RecentSearchType(rawValue: recentSearch.searchType)
    else { return }

    if shouldSubmitSearch {
      recentSearch.update(dateAdded: Date())
    }

    switch searchType {
    case .text:
      if let text = recentSearch.text {
        searchContainer?.applyExternalQuery(text, search: true)

        if shouldSubmitSearch {
          submitSearch(text)
        }
      }
    case .website:
      if let text = recentSearch.text {
        searchContainer?.applyExternalQuery(text, search: true)

        if shouldSubmitSearch {
          if let urlString = recentSearch.websiteUrl,
            let url = URL(string: urlString)
          {
            finishEditingAndSubmit(url)
          } else {
            submitSearch(text)
          }
        }
      }
    case .qrCode:
      if let text = recentSearch.text {
        searchContainer?.applyExternalQuery(text, search: true)

        if shouldSubmitSearch {
          submitSearch(text)
        }
      } else if let websiteUrl = recentSearch.websiteUrl {
        searchContainer?.applyExternalQuery(websiteUrl, search: true)

        if shouldSubmitSearch {
          submitSearch(websiteUrl)
        }
      }
    }
  }

  func openAddBookmark() {
    guard let selectedTab = tabManager.selectedTab,
      let selectedUrl = selectedTab.visibleURL,
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

    let doneBarbutton = UIBarButtonItem.doneButton(
      target: navigationController,
      action: #selector(SettingsNavigationController.done)
    )
    navigationController.navigationBar.topItem?.leftBarButtonItem =
      cancelEnabled ? cancelBarbutton : nil

    navigationController.navigationBar.topItem?.rightBarButtonItem = doneBarbutton

    present(navigationController, animated: true)
  }

  func presentMenu(from tabToolbar: ToolbarProtocol) {
    /// The selected tab's url, or the extracted url from
    /// error page or reader mode page
    let selectedTabURL: URL? = {
      guard let url = tabManager.selectedTab?.visibleURL else { return nil }

      if let internalURL = InternalURL(url) {
        if internalURL.isReaderModePage {
          return internalURL.extractedUrlParam
        }
        return nil
      }
      if url.isNewTabURL {
        return nil
      }
      return url
    }()

    clearPageZoomDialog()

    var activities: [UIActivity] = []
    if let url = selectedTabURL, let tab = tabManager.selectedTab {
      activities = makeShareActivities(
        url: url,
        tab: tab,
        syncAPI: profileController.syncAPI,
        sendTabAPI: profileController.sendTabAPI,
        feedDataSource: feedDataSource,
        isBraveNewsAvailable: profileController.profile.prefs.isBraveNewsAvailable,
        source: .init(
          view: view,
          rect: view.convert(
            topToolbar.menuButton.frame,
            from: topToolbar.menuButton.superview
          ),
          arrowDirection: .up
        ),
        callbacks: .init(
          onToggleReaderMode: { tab.readerMode?.toggleReaderMode() },
          onDisplayPageZoom: { [weak self] in self?.displayPageZoomDialog() },
          onAddSearchEngine: { [weak self] in
            guard let self else { return }
            self.evaluateWebsiteSupportOpenSearchEngine(in: tab)
            self.addCustomSearchEngineForFocusedElement()
          },
          onDisplayCertificate: { [weak self] in self?.displayPageCertificateInfo() },
          onShowSubmitReport: { [weak self] url in self?.showSubmitReportView(for: url) },
          onCleanUp: { [weak self] in self?.showQueuedAlertIfAvailable() }
        )
      )
    }

    presentBrowserMenu(
      from: tabToolbar.menuButton,
      activities: activities,
      tab: tabManager.selectedTab,
      pageURL: selectedTabURL
    )
  }
}

extension BrowserViewController: ToolbarDelegate {
  func tabToolbarDidPressSearch(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
  }

  func tabToolbarDidPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    tabManager.selectedTab?.goBack()
    tabManager.selectedTab?.browserData?.resetExternalAlertProperties()
    recordNavigationActionP3A(isNavigationActionForward: false)
  }

  func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    UIImpactFeedbackGenerator(style: .heavy).vibrate()
    showBackForwardList()
  }

  func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    tabManager.selectedTab?.goForward()
    tabManager.selectedTab?.browserData?.resetExternalAlertProperties()
    recordNavigationActionP3A(isNavigationActionForward: true)
  }

  func tabToolbarDidPressShare() {
    navigationHelper.openShareSheet()
  }

  func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol) {
    presentMenu(from: tabToolbar)
  }

  func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
    recordCreateTabAction(location: .toolbar)
    self.openBlankNewTab(
      attemptLocationFieldFocus: Preferences.General.openKeyboardOnNTPSelection.value,
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
    guard let tab = tabManager.selectedTab, let url = tab.visibleURL
    else { return }
    let hasCertificate = tab.serverTrust != nil
    let pageSecurityView = PageSecurityView(
      displayURL: urlBar.locationView.urlDisplayLabel.text ?? url.absoluteDisplayString,
      secureState: tab.visibleSecureContentState,
      hasCertificate: hasCertificate,
      presentCertificateViewer: { [weak self] in
        self?.dismiss(animated: true)
        self?.displayPageCertificateInfo()
      }
    )
    let popoverController = PopoverController(content: pageSecurityView)
    popoverController.present(from: urlBar.locationView.secureContentStateButton, on: self)
  }

  func showBackForwardList() {
    if let backForwardList = tabManager.selectedTab?.backForwardList {
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
    tabManager.selectedTab?.stopLoading()
    processAddressBarTask?.cancel()
    topToolbarDidPressReloadTask?.cancel()
    topToolbar.locationView.loading = tabManager.selectedTab?.isLoading == true
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
    guard let tab = tabManager.selectedTab, let url = tab.visibleURL, url.isWebPage() else {
      return nil
    }
    let reloadTitle =
      tab.currentUserAgentType == .desktop
      ? Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
    let reloadIcon = tab.currentUserAgentType == .desktop ? "leo.smartphone" : "leo.monitor"
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
            self.processAddressBar(text: pasteboardContents)
          }
        }
      ),
      UIAction(
        identifier: .paste,
        handler: UIAction.deferredActionHandler { _ in
          if let pasteboardContents = UIPasteboard.general.string {
            self.presentSearchInput(initialText: pasteboardContents, pasted: true, search: true)
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
        image: UIImage(braveSystemNamed: "leo.copy"),
        handler: UIAction.deferredActionHandler { _ in
          UIPasteboard.general.url = url as URL
        }
      ),
      UIAction(
        title: Strings.copyCleanLink,
        image: UIImage(braveSystemNamed: "leo.copy.clean"),
        handler: UIAction.deferredActionHandler { _ in
          let service = URLSanitizerServiceFactory.get(privateMode: tab?.isPrivate ?? true)
          let cleanedURL = service?.sanitize(url: url) ?? url
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
