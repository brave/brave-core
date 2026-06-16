// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import Data
import Preferences
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
  private lazy var toolbarHostingController = UIHostingController(
    rootView: QuickViewToolbarView(viewModel: toolbarViewModel)
  )
  private var readerModeBar: ReaderModeBarView?
  private lazy var privateBrowsingManager: PrivateBrowsingManager = {
    let manager = PrivateBrowsingManager()
    manager.isPrivateBrowsing = profile.isOffTheRecord
    return manager
  }()
  private let onOpenInNewTab: ((URLRequest) -> Void)?
  private let onAttachTab: ((any TabState) -> Void)?

  init(
    url: URL,
    profileController: BraveProfileController,
    onOpenInNewTab: ((URLRequest) -> Void)?,
    onAttachTab: ((any TabState) -> Void)?
  ) {
    self.url = url
    self.toolbarViewModel = QuickViewToolbarModel(url: url)
    self.profileController = profileController
    self.onOpenInNewTab = onOpenInNewTab
    self.onAttachTab = onAttachTab
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
    tab.browserData = TabBrowserData(tab: tab)
    tab.readerMode = .init(tab: tab, readerModeCache: ReaderModeScriptHandler.cache(for: tab))
    tab.readerMode?.onStateChanged = { [weak self, weak tab] in
      self?.toolbarViewModel.readerModeState = tab?.readerMode?.state ?? .unavailable
    }
    tab.readerMode?.onReaderModeDisplayed = { [weak self] in
      self?.showReaderModeBar()
    }
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
        self?.currentTab?.readerMode?.toggleReaderMode()
      case .refresh:
        guard let currentTab = self?.currentTab else { return }
        currentTab.reload()
      case .openTab:
        self?.dismiss(animated: true) {
          guard let self, let currentTab = self.currentTab else { return }
          currentTab.removeObserver(self.toolbarViewModel)
          currentTab.removeObserver(self)
          self.onAttachTab?(currentTab)
        }
      case .share:
        guard let self, let visibleURL = self.currentTab?.visibleURL
        else { return }
        let anchorView = self.toolbarHostingController.rootView.shareBackgroundView.uiView
        self.presentShareActivity(
          url: visibleURL,
          tab: currentTab,
          syncAPI: profileController.syncAPI,
          sendTabAPI: profileController.sendTabAPI,
          feedDataSource: nil,
          isBraveNewsAvailable: false,
          source: .init(
            view: anchorView,
            rect: anchorView.bounds,
            arrowDirection: [.down]
          ),
          callbacks: .init(
            onToggleReaderMode: { [weak self] in
              self?.currentTab?.readerMode?.toggleReaderMode()
            },
            onShowSubmitReport: { [weak self] url in
              self?.showSubmitReportView(for: url)
            }
          )
        )
      case .playlist, .translate:
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
    toolbarViewModel.isShieldEnabled = isShieldsEnabled
  }

  private func showReaderModeBar() {
    guard readerModeBar == nil, let currentTab else { return }
    let bar = ReaderModeBarView(privateBrowsingManager: privateBrowsingManager)
    bar.delegate = self
    view.insertSubview(bar, aboveSubview: currentTab.view)
    readerModeBar = bar
    bar.snp.makeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.height.equalTo(UIConstants.toolbarHeight)
    }
    currentTab.view.snp.remakeConstraints {
      $0.top.equalTo(bar.snp.bottom)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
    }
  }

  private func hideReaderModeBar() {
    guard let bar = readerModeBar, let currentTab else { return }
    bar.removeFromSuperview()
    readerModeBar = nil
    currentTab.view.snp.remakeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
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
      guard let self, let currentTab = self.currentTab else { return }
      currentTab.removeObserver(self.toolbarViewModel)
      currentTab.removeObserver(self)
      self.onAttachTab?(currentTab)
      self.onOpenInNewTab?(request)
    }
    return nil
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    if tab.visibleURL?.isInternalURL(for: .readermode) != true {
      hideReaderModeBar()
    }
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

// MARK: - ReaderModeBarViewDelegate
extension QuickViewController: ReaderModeBarViewDelegate {
  func readerModeSettingsTapped(_ view: UIView) {
    var readerModeStyle = defaultReaderModeStyle
    if let encodedString = Preferences.ReaderMode.style.value,
      let style = ReaderModeStyle(encodedString: encodedString)
    {
      readerModeStyle = style
    }
    let vc = ReaderModeStyleViewController(selectedStyle: readerModeStyle)
    vc.delegate = self
    vc.modalPresentationStyle = .popover
    vc.presentationController?.delegate = self
    if let popover = vc.popoverPresentationController {
      popover.backgroundColor = .white
      popover.sourceView = view
      popover.sourceRect = CGRect(
        x: view.bounds.width / 2,
        y: UIConstants.toolbarHeight / 2,
        width: 0,
        height: 0
      )
      popover.permittedArrowDirections = .up
    }
    present(vc, animated: true)
  }
}

// MARK: - ReaderModeStyleViewControllerDelegate

extension QuickViewController: ReaderModeStyleViewControllerDelegate {
  func readerModeStyleViewController(
    _ readerModeStyleViewController: ReaderModeStyleViewController,
    didConfigureStyle style: ReaderModeStyle
  ) {
    Preferences.ReaderMode.style.value = style.encode()
    currentTab?.readerMode?.setStyle(style)
  }
}

// MARK: - UIPopoverPresentationControllerDelegate

extension QuickViewController: UIAdaptivePresentationControllerDelegate {
  public func adaptivePresentationStyle(
    for controller: UIPresentationController,
    traitCollection: UITraitCollection
  ) -> UIModalPresentationStyle {
    return .none
  }
}
