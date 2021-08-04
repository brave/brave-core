// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import BraveRewards
import Storage
import Data
import SwiftUI

// MARK: - TopToolbarDelegate

extension BrowserViewController: TopToolbarDelegate {
    
    func showTabTray() {
        if tabManager.tabsForCurrentMode.isEmpty {
            return
        }
        updateFindInPageVisibility(visible: false)
        
        let tabTrayController = TabTrayController(tabManager: tabManager, profile: profile, tabTrayDelegate: self)
        
        if tabManager.selectedTab == nil {
            tabManager.selectTab(tabManager.tabsForCurrentMode.first)
        }
        if let tab = tabManager.selectedTab {
            screenshotHelper.takeScreenshot(tab)
        }
        
        isTabTrayActive = true
        
        navigationController?.pushViewController(tabTrayController, animated: true)
        self.tabTrayController = tabTrayController
    }
    
    func topToolbarDidPressReload(_ topToolbar: TopToolbarView) {
        tabManager.selectedTab?.reload()
    }
    
    func topToolbarDidPressStop(_ topToolbar: TopToolbarView) {
        tabManager.selectedTab?.stop()
    }
    
    func topToolbarDidLongPressReloadButton(_ topToolbar: TopToolbarView, from button: UIButton) {
        guard let tab = tabManager.selectedTab else { return }
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        
        let toggleActionTitle = tab.isDesktopSite == true ?
            Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
        alert.addAction(UIAlertAction(title: toggleActionTitle, style: .default, handler: { _ in
            tab.switchUserAgent()
        }))
        
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        if UIDevice.current.userInterfaceIdiom == .pad {
            alert.popoverPresentationController?.sourceView = self.view
            alert.popoverPresentationController?.sourceRect = self.view.convert(button.frame, from: button.superview)
            alert.popoverPresentationController?.permittedArrowDirections = [.up]
        }
        present(alert, animated: true)
    }

    func topToolbarDidPressTabs(_ topToolbar: TopToolbarView) {
        showTabTray()
    }

    func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView) {
        if let tab = tabManager.selectedTab {
            if let readerMode = tab.getContentScript(name: "ReaderMode") as? ReaderMode {
                switch readerMode.state {
                case .available:
                    enableReaderMode()
                case .active:
                    disableReaderMode()
                case .unavailable:
                    break
                }
            }
        }
    }

    func topToolbarDidLongPressReaderMode(_ topToolbar: TopToolbarView) -> Bool {
        // Maybe we want to add something here down the road
        return false
    }

    func locationActions(for topToolbar: TopToolbarView) -> [AccessibleAction] {
        if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs {
            return [pasteGoAction, pasteAction, copyAddressAction]
        } else {
            return [copyAddressAction]
        }
    }

    func topToolbarDisplayTextForURL(_ topToolbar: URL?) -> (String?, Bool) {
        // use the initial value for the URL so we can do proper pattern matching with search URLs
        var searchURL = self.tabManager.selectedTab?.currentInitialURL
        if searchURL?.isErrorPageURL ?? true {
            searchURL = topToolbar
        }
        if let query = profile.searchEngines.queryForSearchURL(searchURL as URL?) {
            return (query, true)
        } else {
            return (topToolbar?.absoluteString, false)
        }
    }

    func topToolbarDidLongPressLocation(_ topToolbar: TopToolbarView) {
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        for action in locationActions(for: topToolbar) {
            alert.addAction(action.alertAction(style: .default))
        }
        
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        
        let setupPopover = { [unowned self] in
            if let popoverPresentationController = alert.popoverPresentationController {
                popoverPresentationController.sourceView = topToolbar
                popoverPresentationController.sourceRect = topToolbar.frame
                popoverPresentationController.permittedArrowDirections = .any
                popoverPresentationController.delegate = self
            }
        }
        
        setupPopover()
        
        if alert.popoverPresentationController != nil {
            displayedPopoverController = alert
            updateDisplayedPopoverProperties = setupPopover
        }
        
        self.present(alert, animated: true)
    }

    func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView) {
        if let selectedTab = tabManager.selectedTab, favoritesController == nil {
            // Only scroll to top if we are not showing the home view controller
            selectedTab.webView?.scrollView.setContentOffset(CGPoint.zero, animated: true)
        }
    }

    func topToolbarLocationAccessibilityActions(_ topToolbar: TopToolbarView) -> [UIAccessibilityCustomAction]? {
        return locationActions(for: topToolbar).map { $0.accessibilityCustomAction }
    }

    func topToolbar(_ topToolbar: TopToolbarView, didEnterText text: String) {
        if text.isEmpty {
            hideSearchController()
        } else {
            showSearchController()
            searchController?.searchQuery = text
            searchLoader?.query = text
        }
    }

    func topToolbar(_ topToolbar: TopToolbarView, didSubmitText text: String) {
        // TopToolBar Submit Text is Typed URL Visit Type
        // This visit type will be used while adding History
        // And it will determine either to sync the data or not
        processAddressBar(text: text, visitType: .typed)
    }

    func processAddressBar(text: String, visitType: VisitType) {
        if let fixupURL = URIFixup.getURL(text) {
            // The user entered a URL, so use it.
            finishEditingAndSubmit(fixupURL, visitType: visitType)
            
            return
        }

        // We couldn't build a URL, so pass it on to the search engine.
        submitSearchText(text)
        
        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            RecentSearch.addItem(type: .text, text: text, websiteUrl: nil)
        }
    }

    func submitSearchText(_ text: String) {
        let engine = profile.searchEngines.defaultEngine()

        if let searchURL = engine.searchURLForQuery(text) {
            // We couldn't find a matching search keyword, so do a search query.
            finishEditingAndSubmit(searchURL, visitType: .typed)
        } else {
            // We still don't have a valid URL, so something is broken. Give up.
            print("Error handling URL entry: \"\(text)\".")
            assertionFailure("Couldn't generate search URL: \(text)")
        }
    }

    func topToolbarDidEnterOverlayMode(_ topToolbar: TopToolbarView) {
        if .blankPage == NewTabAccessors.getNewTabPage() {
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        } else {
            if let toast = clipboardBarDisplayHandler?.clipboardToast {
                toast.removeFromSuperview()
            }
            displayFavoritesController()
        }
    }

    func topToolbarDidLeaveOverlayMode(_ topToolbar: TopToolbarView) {
        hideSearchController()
        hideFavoritesController()
        updateInContentHomePanel(tabManager.selectedTab?.url as URL?)
    }

    func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView) {
        dismissVisibleMenus()
    }
    
    func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView) {
        presentBraveShieldsViewController()
    }
    
    func presentBraveShieldsViewController() {
        guard let selectedTab = tabManager.selectedTab, var url = selectedTab.url else { return }
        if url.isErrorPageURL, let originalURL = url.originalURLFromErrorURL {
            url = originalURL
        }
        if url.isLocalUtility {
            return
        }
        let shields = ShieldsViewController(tab: selectedTab)
        shields.shieldsSettingsChanged = { [unowned self] _ in
            // Update the shields status immediately
            self.topToolbar.refreshShieldsStatus()
            
            // Reload this tab. This will also trigger an update of the brave icon in `TabLocationView` if
            // the setting changed is the global `.AllOff` shield
            self.tabManager.selectedTab?.reload()
            
            // In 1.6 we "reload" the whole web view state, dumping caches, etc. (reload():BraveWebView.swift:495)
            // BRAVE TODO: Port over proper tab reloading with Shields
        }
        shields.showGlobalShieldsSettings = { [unowned self] vc in
            vc.dismiss(animated: true) {
                let shieldsAndPrivacy = BraveShieldsAndPrivacySettingsController(
                    profile: self.profile,
                    tabManager: self.tabManager,
                    feedDataSource: self.feedDataSource
                )
                let container = SettingsNavigationController(rootViewController: shieldsAndPrivacy)
                container.isModalInPresentation = true
                container.modalPresentationStyle =
                    UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
                shieldsAndPrivacy.navigationItem.rightBarButtonItem = .init(
                    barButtonSystemItem: .done,
                    target: container,
                    action: #selector(SettingsNavigationController.done)
                )
                self.present(container, animated: true)
            }
        }
        let container = PopoverNavigationController(rootViewController: shields)
        let popover = PopoverController(contentController: container, contentSizeBehavior: .preferredContentSize)
        popover.present(from: topToolbar.locationView.shieldsButton, on: self)
    }
    
    // TODO: This logic should be fully abstracted away and share logic from current MenuViewController
    // See: https://github.com/brave/brave-ios/issues/1452
    func topToolbarDidTapBookmarkButton(_ topToolbar: TopToolbarView) {
        showBookmarkController()
    }
    
    func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView) {
        showBraveRewardsPanel()
    }
    
    func topToolbarDidLongPressBraveRewardsButton(_ topToolbar: TopToolbarView) {
        showRewardsDebugSettings()
    }
    
    func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView) {
        tabToolbarDidPressMenu(topToolbar)
    }
    
    func topToolbarDidPressQrCodeButton(_ urlBar: TopToolbarView) {
        if RecentSearchQRCodeScannerController.hasCameraPermissions {
            let qrCodeController = RecentSearchQRCodeScannerController { [weak self] string in
                guard let self = self else { return }
                
                if let url = URIFixup.getURL(string) {
                    self.didScanQRCodeWithURL(url)
                } else {
                    self.didScanQRCodeWithText(string)
                }
            }
            
            let navigationController = UINavigationController(rootViewController: qrCodeController)
            navigationController.modalPresentationStyle =
                UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
            
            self.present(navigationController, animated: true, completion: nil)
        } else {
            let alert = UIAlertController(title: Strings.scanQRCodeViewTitle, message: Strings.scanQRCodePermissionErrorMessage, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.scanQRCodeErrorOKButton, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }
    }
    
    private func hideSearchController() {
        if let searchController = searchController {
            searchController.willMove(toParent: nil)
            searchController.view.removeFromSuperview()
            searchController.removeFromParent()
            self.searchController = nil
            searchLoader = nil
        }
    }
    
    private func showSearchController() {
        if searchController != nil { return }

        let tabType = TabType.of(tabManager.selectedTab)
        searchController = SearchViewController(forTabType: tabType)
        
        guard let searchController = searchController else { return }

        searchController.searchEngines = profile.searchEngines
        searchController.searchDelegate = self
        searchController.profile = self.profile

        searchLoader = SearchLoader()
        searchLoader?.addListener(searchController)
        searchLoader?.autocompleteSuggestionHandler = { [weak self] completion in
            self?.topToolbar.setAutocompleteSuggestion(completion)
        }

        addChild(searchController)
        view.addSubview(searchController.view)
        searchController.view.snp.makeConstraints { make in
            make.top.equalTo(self.topToolbar.snp.bottom)
            make.left.right.bottom.equalTo(self.view)
            return
        }
        
        searchController.didMove(toParent: self)
    }
    
    private func displayFavoritesController() {
        if favoritesController == nil {
            let tabType = TabType.of(tabManager.selectedTab)
            let favoritesController = FavoritesViewController(tabType: tabType, action: { [weak self] bookmark, action in
                self?.handleFavoriteAction(favorite: bookmark, action: action)
            }, recentSearchAction: { [weak self] recentSearch, shouldSubmitSearch in
                guard let self = self else { return }
                
                let submitSearch = { [weak self] (text: String) in
                    if let fixupURL = URIFixup.getURL(text) {
                        self?.finishEditingAndSubmit(fixupURL, visitType: .unknown)
                        return
                    }
                    
                    self?.submitSearchText(text)
                }
                
                if let recentSearch = recentSearch,
                   let searchType = RecentSearchType(rawValue: recentSearch.searchType) {
                    if shouldSubmitSearch {
                        recentSearch.update(dateAdded: Date())
                    }
                    
                    switch searchType {
                    case .text:
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
                    case .website:
                        if let websiteUrl = recentSearch.websiteUrl {
                            self.topToolbar.setLocation(websiteUrl, search: false)
                            self.topToolbar(self.topToolbar, didEnterText: websiteUrl)
                            
                            if shouldSubmitSearch {
                                submitSearch(websiteUrl)
                            }
                        }
                    }
                } else if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs,
                          let searchQuery = UIPasteboard.general.string ?? UIPasteboard.general.url?.absoluteString {
                    
                    self.topToolbar.setLocation(searchQuery, search: false)
                    self.topToolbar(self.topToolbar, didEnterText: searchQuery)
                    
                    if shouldSubmitSearch {
                        submitSearch(searchQuery)
                    }
                }
            })
            self.favoritesController = favoritesController
            
            addChild(favoritesController)
            view.addSubview(favoritesController.view)
            favoritesController.didMove(toParent: self)
            
            favoritesController.view.snp.makeConstraints {
                $0.top.leading.trailing.equalTo(pageOverlayLayoutGuide)
                $0.bottom.equalTo(view)
            }
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
        UIView.animate(withDuration: 0.1, delay: 0, options: [.beginFromCurrentState], animations: {
            controller.view.alpha = 0.0
        }, completion: { _ in
            controller.willMove(toParent: nil)
            controller.view.removeFromSuperview()
            controller.removeFromParent()
            self.webViewContainer.accessibilityElementsHidden = false
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        })
    }
    
    private func showBookmarkController() {
        let bookmarkViewController = BookmarksViewController(
            folder: Bookmarkv2.lastVisitedFolder(),
            isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
        
        bookmarkViewController.toolbarUrlActionsDelegate = self
        
        presentSettingsNavigation(with: bookmarkViewController)
    }
    
    func openAddBookmark() {
        guard let selectedTab = tabManager.selectedTab,
              let selectedUrl = selectedTab.url,
              !(selectedUrl.isLocal || selectedUrl.isReaderModeURL) else {
            return
        }

        let bookmarkUrl = selectedUrl.decodeReaderModeURL ?? selectedUrl

        let mode = BookmarkEditMode.addBookmark(title: selectedTab.displayTitle, url: bookmarkUrl.absoluteString)

        let addBookMarkController = AddEditBookmarkTableViewController(mode: mode)

        presentSettingsNavigation(with: addBookMarkController, cancelEnabled: true)
    }
    
    private func presentSettingsNavigation(with controller: UIViewController, cancelEnabled: Bool = false) {
        let navigationController = SettingsNavigationController(rootViewController: controller)
        navigationController.modalPresentationStyle = .formSheet
        
        let cancelBarbutton = UIBarButtonItem(
            barButtonSystemItem: .cancel,
            target: navigationController,
            action: #selector(SettingsNavigationController.done))
        
        let doneBarbutton = UIBarButtonItem(
            barButtonSystemItem: .done,
            target: navigationController,
            action: #selector(SettingsNavigationController.done))
        
        navigationController.navigationBar.topItem?.leftBarButtonItem = cancelEnabled ? cancelBarbutton : nil
        
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
    }

    func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        showBackForwardList()
    }
    
    func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        tabManager.selectedTab?.goForward()
    }
    
    func tabToolbarDidPressShare() {
        func share(url: URL) {
            presentActivityViewController(
                url,
                tab: url.isFileURL ? nil : tabManager.selectedTab,
                sourceView: view,
                sourceRect: view.convert(topToolbar.menuButton.frame, from: topToolbar.menuButton.superview),
                arrowDirection: [.up]
            )
        }
        
        guard let tab = tabManager.selectedTab, let url = tab.url else { return }
        
        if let temporaryDocument = tab.temporaryDocument {
            temporaryDocument.getURL().uponQueue(.main, block: { tempDocURL in
                // If we successfully got a temp file URL, share it like a downloaded file,
                // otherwise present the ordinary share menu for the web URL.
                if tempDocURL.isFileURL {
                    share(url: tempDocURL)
                } else {
                    share(url: url)
                }
            })
        } else {
            share(url: url)
        }
    }
    
    func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol) {
        let selectedTabURL: URL? = {
            guard let url = tabManager.selectedTab?.url, !url.isLocal || url.isReaderModeURL else { return nil }
            return url
        }()
        var activities: [UIActivity] = []
        if let url = selectedTabURL, let tab = tabManager.selectedTab {
            activities = shareActivities(for: url, tab: tab, sourceView: view, sourceRect: self.view.convert(self.topToolbar.menuButton.frame, from: self.topToolbar.menuButton.superview), arrowDirection: .up)
        }
        let initialHeight: CGFloat = selectedTabURL != nil ? 470 : 370
        let menuController = MenuViewController(initialHeight: initialHeight, content: { menuController in
            VStack(spacing: 6) {
                featuresMenuSection(menuController)
                Divider()
                destinationMenuSection(menuController)
                if let tabURL = selectedTabURL {
                    Divider()
                    activitiesMenuSection(menuController, tabURL: tabURL, activities: activities)
                }
            }
        })
        presentPanModal(menuController, sourceView: tabToolbar.menuButton, sourceRect: tabToolbar.menuButton.bounds)
        if menuController.modalPresentationStyle == .popover {
            menuController.popoverPresentationController?.popoverLayoutMargins = .init(equalInset: 4)
            menuController.popoverPresentationController?.permittedArrowDirections = [.up]
        }
    }
    
    func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }
    
    func tabToolbarDidLongPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        showAddTabContextMenu(sourceView: toolbar ?? topToolbar, button: button)
    }
    
    private func addTabAlertActions() -> [UIAlertAction] {
        var actions: [UIAlertAction] = []
        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            let newPrivateTabAction = UIAlertAction(title: Strings.newPrivateTabTitle,
                                                    style: .default,
                                                    handler: { [unowned self] _ in
                self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: true)
            })
            actions.append(newPrivateTabAction)
        }
        let bottomActionTitle = PrivateBrowsingManager.shared.isPrivateBrowsing ? Strings.newPrivateTabTitle : Strings.newTabTitle
        actions.append(UIAlertAction(title: bottomActionTitle, style: .default, handler: { [unowned self] _ in
            self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
        }))
        return actions
    }
    
    func showAddTabContextMenu(sourceView: UIView, button: UIButton) {
        let alertController = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alertController.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        addTabAlertActions().forEach(alertController.addAction)
        alertController.popoverPresentationController?.sourceView = sourceView
        alertController.popoverPresentationController?.sourceRect = button.frame
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        present(alertController, animated: true)
    }
    
    func tabToolbarDidLongPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        showBackForwardList()
    }
    
    func tabToolbarDidPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        showTabTray()
    }
    
    func tabToolbarDidLongPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        guard self.presentedViewController == nil else {
            return
        }
        let controller = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden) ||
            (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil) {
            addTabAlertActions().forEach(controller.addAction)
        }
        
        if tabManager.tabsForCurrentMode.count > 1 {
            controller.addAction(UIAlertAction(title: String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count), style: .destructive, handler: { _ in
                self.tabManager.removeAll()
            }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        }
        controller.addAction(UIAlertAction(title: Strings.closeTabTitle, style: .destructive, handler: { _ in
            if let tab = self.tabManager.selectedTab {
                self.tabManager.removeTab(tab)
            }
        }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        controller.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil), accessibilityIdentifier: "toolbarTabButtonLongPress.cancel")
        controller.popoverPresentationController?.sourceView = toolbar ?? topToolbar
        controller.popoverPresentationController?.sourceRect = button.frame
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        present(controller, animated: true, completion: nil)
    }
    
    func showBackForwardList() {
        if let backForwardList = tabManager.selectedTab?.webView?.backForwardList {
            let backForwardViewController = BackForwardListViewController(profile: profile, backForwardList: backForwardList)
            backForwardViewController.tabManager = tabManager
            backForwardViewController.bvc = self
            backForwardViewController.modalPresentationStyle = .overCurrentContext
            backForwardViewController.backForwardTransitionDelegate = BackForwardListAnimator()
            self.present(backForwardViewController, animated: true, completion: nil)
        }
    }
    
    func tabToolbarDidSwipeToChangeTabs(_ tabToolbar: ToolbarProtocol, direction: UISwipeGestureRecognizer.Direction) {
        let tabs = tabManager.tabsForCurrentMode
        guard let selectedTab = tabManager.selectedTab, let index = tabs.firstIndex(where: { $0 === selectedTab }) else { return }
        let newTabIndex = index + (direction == .left ? -1 : 1)
        if newTabIndex >= 0 && newTabIndex < tabs.count {
            tabManager.selectTab(tabs[newTabIndex])
        }
    }
}
