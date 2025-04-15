// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
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
    tab.browserData = .init(tab: tab, tabGeneratorAPI: braveCore.tabGeneratorAPI)
    tab.browserData?.miscDelegate = self
    tab.pullToRefresh = .init(tab: tab)
    tab.playlist = .init(tab: tab)
    SnackBarTabHelper.create(for: tab)
    tab.braveUserAgentExceptions = braveCore.braveUserAgentExceptions
    tab.translateHelper = .init(tab: tab, delegate: self)
    tab.pageMetadataHelper = .init(tab: tab)
    tab.faviconTabHelper = .init(tab: tab)
    tab.userActivityHelper = .init(tab: tab)
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

    toolbar?.setSearchButtonState(url: selected?.visibleURL)
    if let tab = selected, case let webView = tab.view,
      let scrollView = tab.webViewProxy?.scrollView
    {
      toolbarVisibilityViewModel.beginObservingScrollView(scrollView)
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

      if let url = tab.visibleURL, !InternalURL.isValid(url: url) {
        let previousEstimatedProgress = previous?.estimatedProgress ?? 1.0
        let selectedEstimatedProgress = tab.estimatedProgress

        // Progress should be updated only if there's a difference between tabs.
        // Otherwise we do nothing, so switching between fully loaded tabs won't show the animation.
        if previousEstimatedProgress != selectedEstimatedProgress {
          topToolbar.updateProgressBar(Float(selectedEstimatedProgress))
        }
      } else {
        topToolbar.hideProgressBar()
      }

      previous?.shownPromptAlert?.dismiss(animated: false)
      readerModeCache = ReaderModeScriptHandler.cache(for: tab)
      ReaderModeHandler.readerModeCache = readerModeCache

      webViewContainer.addSubview(webView)
      webView.snp.remakeConstraints { make in
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

      webView.accessibilityLabel = Strings.webContentAccessibilityLabel
      webView.accessibilityIdentifier = "contentView"
      webView.accessibilityElementsHidden = false
    }

    updateToolbarUsingTabManager(tabManager)
    updateStatusBarOverlayColor()

    removeAllBars()
    if let bars = selected.flatMap(SnackBarTabHelper.from)?.bars {
      for bar in bars {
        showBar(bar, animated: true)
      }
    }

    clearPageZoomDialog()
    updateTabsBarVisibility()

    if let tab = selected {
      topToolbar.locationView.loading = tab.isLoading
      updateBackForwardActionStatus(for: tab)
      navigationToolbar.updateForwardStatus(tab.canGoForward)
    }

    let shouldShowPlaylistURLBarButton = selected?.visibleURL?.isPlaylistSupportedSiteURL == true

    if let readerMode = selected?.browserData?.getContentScript(
      name: ReaderModeScriptHandler.scriptName
    )
      as? ReaderModeScriptHandler,
      !shouldShowPlaylistURLBarButton
    {
      topToolbar.updateReaderModeState(readerMode.state)
      if readerMode.state == .active {
        showReaderModeBar(animated: false)
      } else {
        hideReaderModeBar(animated: false)
      }

      updatePlaylistURLBar(
        tab: selected,
        state: selected?.playlistItemState ?? .none,
        item: selected?.playlistItem
      )
    } else {
      topToolbar.updateReaderModeState(.unavailable)
    }

    if FeatureList.kBraveTranslateEnabled.enabled, let selectedTab = selected,
      selectedTab.translateHelper != nil
    {
      updateTranslateURLBar(tab: selectedTab, state: selectedTab.translationState ?? .unavailable)
      updatePlaylistURLBar(
        tab: selectedTab,
        state: selectedTab.playlistItemState ?? .none,
        item: selectedTab.playlistItem
      )
    } else {
      topToolbar.updateTranslateButtonState(.unavailable)
    }

    updateScreenTimeUrl(tabManager.selectedTab?.visibleURL)
    updateInContentHomePanel(selected?.visibleURL as URL?)

    notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
    WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.eth, .sol])
    WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [.eth, .sol])
    updateURLBarWalletButton()
  }

  func tabManager(_ tabManager: TabManager, willAddTab tab: some TabState) {
  }

  func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState) {
    // If we are restoring tabs then we update the count once at the end
    if !tabManager.isRestoring {
      updateToolbarUsingTabManager(tabManager)
    }
    tab.addObserver(self)
    tab.addPolicyDecider(self)
    tab.delegate = self
    tab.downloadDelegate = self
    tab.certificateStore = profile.certStore
    attachTabHelpers(to: tab)

    SnackBarTabHelper.from(tab: tab)?.delegate = self

    tab.walletKeyringService = BraveWallet.KeyringServiceFactory.get(privateMode: tab.isPrivate)
    updateTabsBarVisibility()
  }

  func tabManager(_ tabManager: TabManager, willRemoveTab tab: some TabState) {
    tab.view.removeFromSuperview()
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState) {
    updateToolbarUsingTabManager(tabManager)
    // tabDelegate is a weak ref (and the tab's webView may not be destroyed yet)
    // so we don't expcitly unset it.
    topToolbar.leaveOverlayMode(didCancel: true)
    updateTabsBarVisibility()
    tab.removeObserver(self)
    tab.removePolicyDecider(self)

    if !privateBrowsingManager.isPrivateBrowsing {
      rewards.reportTabClosed(tabId: Int(tab.rewardsId ?? 0))
    }
  }

  func tabManagerDidAddTabs(_ tabManager: TabManager) {
    updateToolbarUsingTabManager(tabManager)
  }

  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    updateToolbarUsingTabManager(tabManager)
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
        make.bottom.equalTo(self.webViewContainer)
      },
      completion: {
        if toast is ButtonToast {
          self.activeButtonToast = nil
        }
      }
    )
  }

  func hideToastsOnNavigationStartIfNeeded(_ tabManager: TabManager) {
    if tabManager.selectedTab?.braveSearchResultAdManager == nil {
      searchResultAdClickedInfoBar?.dismiss(false)
      searchResultAdClickedInfoBar = nil
    }
  }

  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
    guard let toast = toast, !privateBrowsingManager.isPrivateBrowsing else {
      return
    }
    show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
  }

  func updateToolbarUsingTabManager(_ tabManager: TabManager) {
    // Update Tab Count on Tab-Tray Button
    let count = tabManager.tabsForCurrentMode.count
    toolbar?.updateTabCount(count)
    topToolbar.updateTabCount(count)

    // Update Actions for Tab-Tray Button
    var newTabMenuChildren: [UIAction] = []
    var addTabMenuChildren: [UIAction] = []

    if !privateBrowsingManager.isPrivateBrowsing {
      let openNewPrivateTab = UIAction(
        title: Strings.Hotkey.newPrivateTabTitle,
        image: UIImage(systemName: "plus.square.fill.on.square.fill"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          if Preferences.Privacy.privateBrowsingLock.value {
            self.askForLocalAuthentication { [weak self] success, error in
              if success {
                self?.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
              }
            }
          } else {
            self.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
          }
        }
      )

      if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden == true)
        || (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil)
      {
        newTabMenuChildren.append(openNewPrivateTab)
      }

      addTabMenuChildren.append(openNewPrivateTab)
    }

    let openNewTab = UIAction(
      title: privateBrowsingManager.isPrivateBrowsing
        ? Strings.Hotkey.newPrivateTabTitle : Strings.Hotkey.newTabTitle,
      image: privateBrowsingManager.isPrivateBrowsing
        ? UIImage(systemName: "plus.square.fill.on.square.fill")
        : UIImage(systemName: "plus.square.on.square"),
      handler: UIAction.deferredActionHandler { [unowned self] _ in
        self.openBlankNewTab(
          attemptLocationFieldFocus: false,
          isPrivate: privateBrowsingManager.isPrivateBrowsing
        )
      }
    )

    if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden)
      || (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil)
    {
      newTabMenuChildren.append(openNewTab)
    }
    addTabMenuChildren.append(openNewTab)

    var bookmarkMenuChildren: [UIAction] = []

    let containsWebPage = tabManager.selectedTab?.containsWebPage == true
    let containsBookmarkablePage = tabManager.openedWebsitesCount > 1

    // Show bookmark actions if current page is a webpage
    if containsWebPage {
      let bookmarkActiveTab = UIAction(
        title: Strings.addToMenuItem,
        image: UIImage(systemName: "book"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.openAddBookmark()
        }
      )
      bookmarkMenuChildren.append(bookmarkActiveTab)

      // To show bookmark all there should be more than 1 bookmarkable tab
      if containsBookmarkablePage {
        let bookmarkAllTabs = UIAction(
          title: String.localizedStringWithFormat(
            Strings.bookmarkAllTabsTitle,
            tabManager.openedWebsitesCount
          ),
          image: UIImage(systemName: "book"),
          handler: UIAction.deferredActionHandler { [unowned self] _ in
            let mode = BookmarkEditMode.addFolderUsingTabs(
              title: Strings.savedTabsFolderTitle,
              tabList: tabManager.tabsForCurrentMode
            )
            let addBookMarkController = AddEditBookmarkTableViewController(
              bookmarkManager: bookmarkManager,
              mode: mode,
              isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
            )

            presentSettingsNavigation(with: addBookMarkController, cancelEnabled: true)
          }
        )
        bookmarkMenuChildren.append(bookmarkAllTabs)
      }
    }

    var duplicateTabMenuChildren: [UIAction] = []

    if containsWebPage, let selectedTab = tabManager.selectedTab, let url = selectedTab.fetchedURL {
      let duplicateActiveTab = UIAction(
        title: Strings.duplicateActiveTab,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { [weak selectedTab] _ in
          guard let selectedTab = selectedTab else { return }

          tabManager.addTabAndSelect(
            URLRequest(url: url),
            afterTab: selectedTab,
            isPrivate: selectedTab.isPrivate
          )
        }
      )

      duplicateTabMenuChildren.append(duplicateActiveTab)
    }

    var recentlyClosedMenuChildren: [UIAction] = []

    // Recently Closed Actions are only in normal mode
    if !privateBrowsingManager.isPrivateBrowsing {
      let viewRecentlyClosedTabs = UIAction(
        title: Strings.RecentlyClosed.viewRecentlyClosedTab,
        image: UIImage(braveSystemNamed: "leo.browser.mobile-recent-tabs"),
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let self = self else { return }

          if privateBrowsingManager.isPrivateBrowsing {
            return
          }

          var recentlyClosedTabsView = RecentlyClosedTabsView(tabManager: tabManager)
          recentlyClosedTabsView.onRecentlyClosedSelected = { [weak self] recentlyClosed in
            self?.tabManager.addAndSelectRecentlyClosed(recentlyClosed)

            // After opening the Recently Closed in a new tab delete it from list
            RecentlyClosed.remove(with: recentlyClosed.url)
          }

          recentlyClosedTabsView.onClearAllRecentlyClosed = { [weak self] in
            // After clearing tabs need to remove button actions from the tab bar controls
            guard let self = self else { return }

            DispatchQueue.main.async {
              self.updateToolbarUsingTabManager(tabManager)
            }
          }

          self.present(UIHostingController(rootView: recentlyClosedTabsView), animated: true)
        }
      )
      // Fetch last item in Recently Closed
      if let recentlyClosedTab = RecentlyClosed.all().first {
        recentlyClosedMenuChildren.append(viewRecentlyClosedTabs)
        let reopenLastClosedTab = UIAction(
          title: Strings.RecentlyClosed.recentlyClosedReOpenLastActionTitle,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-tab-ntp"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self = self else { return }

            if privateBrowsingManager.isPrivateBrowsing {
              return
            }

            self.tabManager.addAndSelectRecentlyClosed(recentlyClosedTab)
            RecentlyClosed.remove(with: recentlyClosedTab.url)
          }
        )

        recentlyClosedMenuChildren.append(reopenLastClosedTab)
      }
    }

    var closeTabMenuChildren: [UIAction] = []

    let closeActiveTab = UIAction(
      title: String(format: Strings.Hotkey.closeTabTitle),
      image: UIImage(systemName: "xmark"),
      attributes: .destructive,
      handler: UIAction.deferredActionHandler { [unowned self] _ in
        if let tab = tabManager.selectedTab {
          if topToolbar.locationView.readerModeState == .active {
            hideReaderModeBar(animated: false)
          }

          // Add the tab information to recently closed before removing
          tabManager.addTabToRecentlyClosed(tab)
          tabManager.removeTab(tab)
        }
      }
    )

    closeTabMenuChildren.append(closeActiveTab)

    var closeAllTabMenuChildren: [UIAction] = []

    if FeatureList.kBraveShredFeature.enabled,
      let url = tabManager.selectedTab?.visibleURL,
      url.isShredAvailable
    {
      let shredDataAction = UIAction(
        title: Strings.Shields.shredSiteData,
        image: UIImage(braveSystemNamed: "leo.shred.data"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let tab = self?.tabManager.selectedTab, let url = tab.visibleURL else { return }
          let alert = UIAlertController.shredDataAlert(url: url) { _ in
            self?.shredData(for: url, in: tab)
          }

          self?.present(alert, animated: true)
        }
      )
      closeAllTabMenuChildren.append(shredDataAction)
    }

    if tabManager.tabsForCurrentMode.count > 1 {
      func showCloseTabWarning(isActiveTabIncluded: Bool, _ completion: @escaping () -> Void) {
        let alert = UIAlertController(
          title: nil,
          message: isActiveTabIncluded
            ? Strings.closeAllTabsPrompt : Strings.closeAllOtherTabsPrompt,
          preferredStyle: .actionSheet
        )
        let cancelAction = UIAlertAction(title: Strings.CancelString, style: .cancel)
        let closedTabsTitle =
          isActiveTabIncluded
          ? String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count)
          : Strings.closeAllOtherTabsTitle
        let closeAllAction = UIAlertAction(title: closedTabsTitle, style: .destructive) { _ in
          completion()
        }
        alert.addAction(closeAllAction)
        alert.addAction(cancelAction)

        if let popoverPresentation = alert.popoverPresentationController {
          let tabsButton = toolbar?.tabsButton ?? topToolbar.tabsButton
          popoverPresentation.sourceView = tabsButton
          popoverPresentation.sourceRect =
            .init(x: tabsButton.frame.width / 2, y: tabsButton.frame.height, width: 1, height: 1)
        }

        present(alert, animated: true)
      }

      let closeAllOtherTabs = UIAction(
        title: Strings.closeAllOtherTabsTitle,
        image: UIImage(systemName: "xmark"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let self = self else { return }

          showCloseTabWarning(isActiveTabIncluded: false) {
            if !self.privateBrowsingManager.isPrivateBrowsing {
              // Add the tab information to recently closed before removing
              self.tabManager.addAllTabsToRecentlyClosed(isActiveTabIncluded: false)
            }

            self.tabManager.removeAllForCurrentMode(isActiveTabIncluded: false)
          }
        }
      )

      let closeAllTabs = UIAction(
        title: String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count),
        image: UIImage(systemName: "xmark"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let self = self else { return }

          showCloseTabWarning(isActiveTabIncluded: true) {
            if !self.privateBrowsingManager.isPrivateBrowsing {
              // Add the tab information to recently closed before removing
              self.tabManager.addAllTabsToRecentlyClosed(isActiveTabIncluded: true)
            }

            self.tabManager.removeAllForCurrentMode()
          }
        }
      )

      closeAllTabMenuChildren.append(closeAllOtherTabs)
      closeAllTabMenuChildren.append(closeAllTabs)
    }

    let newTabMenu = UIMenu(title: "", options: .displayInline, children: newTabMenuChildren)
    let addTabMenu = UIMenu(title: "", options: .displayInline, children: addTabMenuChildren)
    let bookmarkMenu = UIMenu(title: "", options: .displayInline, children: bookmarkMenuChildren)
    let duplicateTabMenu = UIMenu(
      title: "",
      options: .displayInline,
      children: duplicateTabMenuChildren
    )
    let recentlyClosedMenu = UIMenu(
      title: "",
      options: .displayInline,
      children: recentlyClosedMenuChildren
    )
    let closeAllTabMenu = UIMenu(
      title: "",
      options: .displayInline,
      children: closeAllTabMenuChildren
    )
    let closeTabMenu = UIMenu(title: "", options: .displayInline, children: closeTabMenuChildren)

    let tabButtonMenuActionList = [
      closeTabMenu, closeAllTabMenu, recentlyClosedMenu, duplicateTabMenu, bookmarkMenu, newTabMenu,
    ]
    let addTabMenuActionList = [addTabMenu]

    toolbar?.tabsButton.menu = UIMenu(title: "", identifier: nil, children: tabButtonMenuActionList)
    toolbar?.searchButton.menu = UIMenu(title: "", identifier: nil, children: addTabMenuActionList)

    topToolbar.tabsButton.menu = UIMenu(
      title: "",
      identifier: nil,
      children: tabButtonMenuActionList
    )
    toolbar?.searchButton.menu = UIMenu(title: "", identifier: nil, children: addTabMenuActionList)

    // Update Actions for Add-Tab Button
    topToolbar.addTabButton.menu = UIMenu(
      title: "",
      identifier: nil,
      children: addTabMenuActionList
    )
    toolbar?.addTabButton.menu = UIMenu(title: "", identifier: nil, children: addTabMenuActionList)
  }
}
