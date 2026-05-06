// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import Data
import Shared
import SnapKit
import SwiftUI
import UIKit
import Web
import WebKit

class QuickViewController: UIViewController {
  private let url: URL
  private var currentTab: (any TabState)?
  private var profileController: BraveProfileController
  private var profile: any Profile { profileController.profile }
  private let toolbarViewModel: QuickViewToolbarModel
  private var readerModeHandler: ReaderModeScriptHandler?
  private lazy var toolbarHostingController = UIHostingController(
    rootView: QuickViewToolbarView(viewModel: toolbarViewModel)
  )
  private let onOpenInNewTab: ((URLRequest) -> Void)?

  init(
    url: URL,
    profileController: BraveProfileController,
    onOpenInNewTab: ((URLRequest) -> Void)?
  ) {
    self.url = url
    self.toolbarViewModel = QuickViewToolbarModel(url: url)
    self.profileController = profileController
    self.onOpenInNewTab = onOpenInNewTab
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .fullScreen
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    super.viewDidLoad()

    var initialConfiguration: WKWebViewConfiguration?
    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      initialConfiguration =
        profile.isOffTheRecord
        ? TabManager.privateConfiguration : TabManager.defaultConfiguration
    }
    let tab = TabStateFactory.create(
      with: .init(profile: profile, initialConfiguration: initialConfiguration)
    )
    tab.addObserver(toolbarViewModel)
    tab.addObserver(self)
    tab.createWebView()
    tab.delegate = self
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true
    tab.isVisible = true
    self.currentTab = tab

    updateViewModel()

    setupUI()

    currentTab?.loadRequest(URLRequest(url: url))
  }

  private func updateViewModel() {
    // update shield button status
    refreshShieldStatus(url: currentTab?.visibleURL ?? url)
    // update action buttons
    toolbarViewModel.onActionButton = { [weak self] button in
      switch button {
      case .close:
        self?.dismiss(animated: true)
      case .back:
        guard let currentTab = self?.currentTab else { return }
        currentTab.goBack()
      case .forward:
        guard let currentTab = self?.currentTab else { return }
        currentTab.goForward()
      case .shield:
        self?.presentBraveShieldsView()
      case .readerMode:
        self?.toggleReaderMode()
      case .refresh, .playlist,
        .translate, .share, .openTab:
        break
      }
    }
  }

  private func setupUI() {
    guard let currentTab = currentTab else {
      return
    }

    let colors: any BrowserColors = profile.isOffTheRecord ? .privateMode : .standard
    view.backgroundColor = colors.chromeBackground
    view.addSubview(currentTab.view)

    toolbarHostingController.view.backgroundColor = .clear
    addChild(toolbarHostingController)
    view.addSubview(toolbarHostingController.view)
    toolbarHostingController.didMove(toParent: self)

    currentTab.view.snp.makeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
    }
    toolbarHostingController.view.snp.makeConstraints {
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(view.safeAreaLayoutGuide.snp.bottom)
    }
  }

  private func presentBraveShieldsView() {
    guard let tab = currentTab, var url = tab.visibleURL else { return }
    if let internalURL = InternalURL(url) {
      guard let originalURL = internalURL.url.strippedInternalURL else { return }
      url = originalURL
    }
    if !url.isWebPage(includeDataURIs: false) {
      return
    }

    weak var weakPopover: PopoverController?
    let popover = PopoverController(
      contentController: PopoverNavigationController(
        rootViewController: ShieldsPanelViewController(
          url: url,
          tab: tab,
          domain: Domain.getOrCreate(forUrl: url, persistent: !tab.isPrivate),
          isAdvancedControlsEnabled: false
        ) { [weak self] action in
          guard let self else { return }
          switch action {
          case .navigate(let target, _):
            switch target {
            case .reportBrokenSite:
              weakPopover?.dismiss(animated: true) {
                self.showSubmitReportView(for: url)
              }
            case .shareStats:
              weakPopover?.dismiss(animated: true) {
                let activityController =
                  ShieldsActivityItemSourceProvider.shared.setupGlobalShieldsActivityController(
                    isPrivateBrowsing: tab.isPrivate
                  )
                self.present(activityController, animated: true, completion: nil)
              }
            case .globalShields:  // not available in quickview mode
              break
            }
          case .changedShieldSettings:
            self.changedShieldSettings()
          case .shredSiteData:  // not available in quickview mode
            break
          }
        }
      ),
      contentSizeBehavior: .preferredContentSize
    )
    weakPopover = popover
    popover.present(
      from: toolbarHostingController.rootView.shieldBackgroundView.uiView,
      on: self
    )
  }

  private func showSubmitReportView(for url: URL) {
    guard let currentTab else { return }
    // Strip fragments and query params from url
    var components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    components?.fragment = nil
    components?.queryItems = nil
    guard let cleanedURL = components?.url else { return }

    let viewController = UIHostingController(
      rootView: SubmitReportView(
        url: cleanedURL,
        isPrivateBrowsing: profile.isOffTheRecord,
        tab: currentTab
      )
    )

    viewController.modalPresentationStyle = .popover

    if let popover = viewController.popoverPresentationController {
      popover.sourceView = toolbarHostingController.rootView.shieldBackgroundView.uiView
      popover.sourceRect = toolbarHostingController.rootView.shieldBackgroundView.uiView.bounds

      let sheet = popover.adaptiveSheetPresentationController
      sheet.largestUndimmedDetentIdentifier = .medium
      sheet.prefersEdgeAttachedInCompactHeight = true
      sheet.widthFollowsPreferredContentSizeWhenEdgeAttached = true
      sheet.detents = [.medium(), .large()]
      sheet.prefersGrabberVisible = true
    }
    present(viewController, animated: true)
  }

  private func changedShieldSettings() {
    let currentDomain = currentTab?.visibleURL?.baseDomain
    let browsers = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene })
      .compactMap({ $0.browserViewController })

    // Update shield status, reload the this tab, reset selectors cache for all
    // browser in regular tabs, since same domain can be visited in regular tabs
    browsers.forEach { browser in
      browser.topToolbar.refreshShieldsStatus()
      browser.tabManager.allTabs.forEach {
        if $0.visibleURL?.baseDomain == currentDomain {
          $0.reload()
          $0.contentBlocker?.resetSelectorsCache()
        }
      }
    }

    // Update shield status, reload the this tab, reset selectors cache in quickview mode
    refreshShieldStatus(url: currentTab?.visibleURL ?? url)
    currentTab?.reload()
    currentTab?.contentBlocker?.resetSelectorsCache()
  }

  private func refreshShieldStatus(url: URL) {
    let isShieldsEnabled = currentTab?.braveShieldsHelper?.isBraveShieldsEnabled(for: url) ?? false
    toolbarViewModel.updateShieldingState(isShieldsEnabled)
  }

  private func toggleReaderMode() {
    guard let tab = currentTab else { return }
    let isActive: Bool
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      isActive = tab.readerMode?.state == .active
    } else {
      isActive = readerModeHandler?.state == .active
    }
    isActive ? disableReaderMode() : enableReaderMode()
  }

  private func enableReaderMode() {
    guard let tab = currentTab,
      let backForwardList = tab.backForwardList,
      let currentURL = backForwardList.currentItem?.url,
      !InternalURL.isValid(url: currentURL)
    else { return }

    let headers =
      (tab.responses?[currentURL] as? HTTPURLResponse)?.allHeaderFields as? [String: String]
    guard
      let readerModeURL = currentURL.encodeEmbeddedInternalURL(for: .readermode, headers: headers)
    else { return }

    let readerModeCache = ReaderModeScriptHandler.cache(for: tab)

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      if let readabilityResult = tab.readerMode?.readabilityResult {
        Task { @MainActor in
          try? await readerModeCache.put(currentURL, readabilityResult)
          tab.loadRequest(PrivilegedRequest(url: readerModeURL) as URLRequest)
        }
      }
    } else {
      tab.evaluateJavaScript(
        functionName: "\(readerModeNamespace).readerize",
        contentWorld: ReaderModeScriptHandler.scriptSandbox
      ) { [weak tab] (object, error) in
        guard let tab else { return }
        if let readabilityResult = ReadabilityResult(object: object as AnyObject?) {
          Task { @MainActor in
            try? await readerModeCache.put(currentURL, readabilityResult)
            tab.loadRequest(PrivilegedRequest(url: readerModeURL) as URLRequest)
          }
        }
      }
    }
  }

  private func disableReaderMode() {
    guard let tab = currentTab,
      let backForwardList = tab.backForwardList,
      let currentURL = backForwardList.currentItem?.url,
      let originalURL = currentURL.decodeEmbeddedInternalURL(for: .readermode)
    else { return }

    tab.loadRequest(URLRequest(url: originalURL))
  }

  private func checkReaderMode(for tab: some TabState) {
    guard let url = tab.visibleURL,
      !url.isNewTabURL,
      !InternalURL.isValid(url: url) || url.isInternalURL(for: .readermode),
      !url.isFileURL
    else { return }

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      if let readerMode = tab.readerMode {
        Task { @MainActor [weak self] in
          await readerMode.checkReadability()
          self?.toolbarViewModel.updateReaderModeState(readerMode.state)
        }
      }
    } else {
      tab.evaluateJavaScript(
        functionName: "\(readerModeNamespace).checkReadability",
        contentWorld: ReaderModeScriptHandler.scriptSandbox
      )
    }
  }
}

// MARK: - TabDelegate
extension QuickViewController: TabDelegate {
  func tab(
    _ tab: some TabState,
    createNewTabWithRequest request: URLRequest,
    isUserInitiated: Bool
  ) -> (any TabState)? {
    // window.open should open in a regular tab
    dismiss(animated: true) { [weak self] in
      self?.onOpenInNewTab?(request)
    }
    return nil
  }
}

// MARK: - TabObserver
extension QuickViewController: TabObserver {
  func tabDidCreateWebView(_ tab: some TabState) {
    if let detachedTabPrivacyHelper = DetachedTabPrivacyHelper(
      tab: tab,
      profileController: profileController
    ) {
      tab.detachedPrivacyHelper = detachedTabPrivacyHelper
    }
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    refreshShieldStatus(url: tab.visibleURL ?? url)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
