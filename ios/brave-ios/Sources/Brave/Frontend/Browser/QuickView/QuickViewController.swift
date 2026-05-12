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
  private let profile: any Profile
  private let toolbarViewModel: QuickViewToolbarModel
  private lazy var toolbarHostingController = UIHostingController(
    rootView: QuickViewToolbarView(viewModel: toolbarViewModel)
  )
  private let onOpenInNewTab: ((URLRequest) -> Void)?

  init(
    url: URL,
    profile: any Profile,
    onOpenInNewTab: ((URLRequest) -> Void)?
  ) {
    self.url = url
    self.toolbarViewModel = QuickViewToolbarModel(url: url)
    self.profile = profile
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
    tab.createWebView()
    tab.addObserver(toolbarViewModel)
    tab.delegate = self
    let braveShieldsTabHelper: BraveShieldsTabHelper = .init(
      tab: tab,
      braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(profile: tab.profile)
    )
    tab.braveShieldsHelper = braveShieldsTabHelper
    tab.addPolicyDecider(braveShieldsTabHelper)
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true
    tab.isVisible = true
    self.currentTab = tab

    guard let currentTab = currentTab else {
      return
    }

    setupUI()

    currentTab.loadRequest(URLRequest(url: url))
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
      case .refresh, .playlist, .readerMode,
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
          isAdancedControlsEnabled: false
        ) { [weak self] action in
          guard let self else { return }
          switch action {
          case .navigate(let target, _):
            guard target == .reportBrokenSite else { return }
            weakPopover?.dismiss(animated: true) {
              self.showSubmitReportView(for: url)
            }
          case .changedShieldSettings:
            self.changedShieldSettings()
          case .shredSiteData:  // no shred in quickview mode
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
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      tab.readerMode = .init(tab: tab)
    } else {
      // content blocker
      if let contentBlocker = tab.contentBlocker {
        tab.browserData?.addContentScript(
          contentBlocker,
          name: ContentBlockerHelper.scriptName,
          contentWorld: ContentBlockerHelper.scriptSandbox
        )
      }
    }
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    refreshShieldStatus(url: tab.visibleURL ?? url)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
