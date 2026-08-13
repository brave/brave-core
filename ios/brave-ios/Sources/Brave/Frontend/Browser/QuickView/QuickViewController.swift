// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import CertificateUtilities
import Combine
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
  private let profile: any Profile
  private let syncAPI: BraveSyncAPI
  private let sendTabAPI: BraveSendTabAPI
  private let historyAPI: BraveHistoryAPI
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
  private let toolbarVisibilityViewModel = ToolbarVisibilityViewModel(
    estimatedTransitionDistance: 110
  )
  private var toolbarVisibilityCancellable: AnyCancellable?
  private let onOpenInNewTab: ((URLRequest, Bool) -> Void)?
  private let onOpenInNewWindow: ((URL, Bool) -> Void)?
  private let onAttachTab: ((any TabState) -> Void)?

  private var preKeyboardToolbarState: ToolbarVisibilityViewModel.ToolbarState?
  private var toolbarHeightConstraint: Constraint?
  private var toolbarFullHeight: CGFloat = 0
  private var toolbarBottomConstraint: NSLayoutConstraint?

  private var isKeyboardVisible: Bool = false
  private let customKeyboardLayoutGuide = UILayoutGuide()
  private var keyboardGuideHiddenConstraints: [NSLayoutConstraint] = []
  private var keyboardGuideVisibleConstraints:
    (
      leading: NSLayoutConstraint, top: NSLayoutConstraint, width: NSLayoutConstraint,
      height: NSLayoutConstraint
    )?

  init(
    url: URL,
    profile: any Profile,
    syncAPI: BraveSyncAPI,
    sendTabAPI: BraveSendTabAPI,
    historyAPI: BraveHistoryAPI,
    onOpenInNewTab: ((URLRequest, Bool) -> Void)?,
    onOpenInNewWindow: ((URL, Bool) -> Void)?,
    onAttachTab: ((any TabState) -> Void)?
  ) {
    self.url = url
    self.profile = profile
    self.syncAPI = syncAPI
    self.sendTabAPI = sendTabAPI
    self.historyAPI = historyAPI
    self.toolbarViewModel = QuickViewToolbarModel(
      url: url,
      isPrivate: profile.isOffTheRecord
    )
    self.onOpenInNewTab = onOpenInNewTab
    self.onOpenInNewWindow = onOpenInNewWindow
    self.onAttachTab = onAttachTab
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .pageSheet
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    // only want to measure toolbar height once and when keyboard is not visible
    guard !isKeyboardVisible, toolbarFullHeight == 0 else { return }
    let toolbarHeight = toolbarHostingController.view.intrinsicContentSize.height
    if toolbarHeight > 0 {
      toolbarFullHeight = toolbarHeight
      let collapsedHeight = QuickViewToolbarView.collapsedHeight(compatibleWith: traitCollection)
      toolbarVisibilityViewModel.transitionDistance = toolbarHeight - collapsedHeight
      if toolbarHeightConstraint == nil {
        toolbarHostingController.view.snp.makeConstraints {
          toolbarHeightConstraint = $0.height.equalTo(toolbarHeight).constraint
        }
      } else {
        toolbarHeightConstraint?.update(offset: toolbarHeight)
      }
    }
  }

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
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      let braveShieldsHelper: BraveShieldsTabHelper = .init(
        tab: tab,
        braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(profile: tab.profile)
      )
      tab.braveShieldsHelper = braveShieldsHelper
      tab.addPolicyDecider(braveShieldsHelper)
      tab.requestBlockingTabHelper = .init(tab: tab)
      tab.cosmeticFilteringTabHelper = .init(tab: tab)
    }
    tab.protectionStats = .init(tab: tab)
    tab.readerMode = .init(tab: tab, readerModeCache: ReaderModeScriptHandler.cache(for: tab))
    tab.readerMode?.onStateChanged = { [weak self, weak tab] in
      self?.toolbarViewModel.readerModeState = tab?.readerMode?.state ?? .unavailable
    }
    tab.readerMode?.onReaderModeDisplayed = { [weak self] in
      self?.showReaderModeBar()
    }
    tab.historyTabHelper = .init(tab: tab, historyAPI: historyAPI)
    tab.createWebView()
    tab.delegate = self
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true
    tab.isVisible = true
    self.currentTab = tab

    updateViewModel()

    setupUI()

    currentTab?.loadRequest(URLRequest(url: url))

    toolbarVisibilityCancellable = toolbarVisibilityViewModel.objectWillChange
      .receive(on: DispatchQueue.main)
      .sink { [weak self] _ in
        self?.handleToolbarVisibilityStateChange()
      }

    KeyboardHelper.defaultHelper.addDelegate(self)
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
          currentTab.historyTabHelper = nil
          self.onAttachTab?(currentTab)
        }
      case .share:
        guard let self, let visibleURL = self.currentTab?.visibleURL
        else { return }
        let anchorView = self.toolbarHostingController.rootView.shareBackgroundView.uiView
        self.presentShareActivity(
          url: visibleURL,
          tab: currentTab,
          syncAPI: syncAPI,
          sendTabAPI: sendTabAPI,
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
      case .sslStatus:
        self?.presentSSLStatusView()
      case .playlist, .translate:
        break
      }
    }
    toolbarViewModel.onTappedCollapsedBarTopArea = { [weak self] in
      guard let self else { return }
      if self.isKeyboardVisible {
        self.view.endEditing(true)
      } else {
        self.toolbarVisibilityViewModel.toolbarState = .expanded
      }
    }
  }

  private func setupUI() {
    guard let currentTab = currentTab else {
      return
    }
    let colors: BrowserColors = profile.isOffTheRecord ? .privateMode : .standard
    view.backgroundColor = colors.chromeBackground
    view.addSubview(currentTab.view)

    toolbarHostingController.view.backgroundColor = colors.chromeBackground
    toolbarHostingController.safeAreaRegions = []
    addChild(toolbarHostingController)
    view.addSubview(toolbarHostingController.view)
    toolbarHostingController.view.translatesAutoresizingMaskIntoConstraints = false
    toolbarHostingController.didMove(toParent: self)

    view.addLayoutGuide(customKeyboardLayoutGuide)

    // Hidden state: zero-height at safe-area bottom}
    keyboardGuideHiddenConstraints = [
      customKeyboardLayoutGuide.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      customKeyboardLayoutGuide.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      customKeyboardLayoutGuide.topAnchor.constraint(
        equalTo: view.safeAreaLayoutGuide.bottomAnchor
      ),
      customKeyboardLayoutGuide.bottomAnchor.constraint(
        equalTo: view.safeAreaLayoutGuide.bottomAnchor
      ),
    ]
    NSLayoutConstraint.activate(keyboardGuideHiddenConstraints)

    // Visible state: frame injected from keyboard notification
    keyboardGuideVisibleConstraints = (
      customKeyboardLayoutGuide.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      customKeyboardLayoutGuide.topAnchor.constraint(equalTo: view.topAnchor),
      customKeyboardLayoutGuide.widthAnchor.constraint(equalToConstant: 0),
      customKeyboardLayoutGuide.heightAnchor.constraint(equalToConstant: 0)
    )

    currentTab.view.snp.makeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
    }
    toolbarHostingController.view.snp.makeConstraints {
      $0.leading.trailing.equalTo(view)
    }
    toolbarBottomConstraint = toolbarHostingController.view.bottomAnchor.constraint(
      equalTo: customKeyboardLayoutGuide.topAnchor
    )
    toolbarBottomConstraint?.isActive = true
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

  private func presentSSLStatusView() {
    guard let tab = currentTab else { return }
    let hasCertificate = tab.serverTrust != nil
    let pageSecurityView = PageSecurityView(
      displayURL: toolbarViewModel.url.absoluteDisplayString,
      secureState: tab.visibleSecureContentState,
      hasCertificate: hasCertificate,
      presentCertificateViewer: { [weak self] in
        self?.dismiss(animated: true)
        self?.displayPageCertificateInfo()
      }
    )
    let popoverController = PopoverController(content: pageSecurityView)
    popoverController.present(
      from: toolbarHostingController.rootView.sslStatusBackgroundView.uiView,
      on: self
    )
  }

  private func displayPageCertificateInfo() {
    guard let tab = currentTab, let trust = tab.serverTrust else { return }
    let host = tab.visibleURL?.host

    Task.detached {
      let serverCertificates: [SecCertificate] =
        SecTrustCopyCertificateChain(trust) as? [SecCertificate] ?? []

      if let serverCertificate = serverCertificates.first,
        let certificate = BraveCertificateModel(certificate: serverCertificate)
      {

        var errorDescription: String?

        do {
          try await BraveCertificateUtils.evaluateTrust(trust, for: host)
        } catch {
          errorDescription = error.localizedDescription
          if let range = errorDescription?.range(of: "“\(certificate.subjectName.commonName)” ")
            ?? errorDescription?.range(of: "\"\(certificate.subjectName.commonName)\" ")
          {
            errorDescription =
              errorDescription?.replacingCharacters(in: range, with: "").capitalizeFirstLetter
          }
        }

        await MainActor.run { [errorDescription] in
          tab.dismissFindInteraction()
          let certificateViewController = CertificateViewController(
            certificate: certificate,
            evaluationError: errorDescription
          )
          certificateViewController.modalPresentationStyle = .pageSheet
          certificateViewController.sheetPresentationController?.detents = [.medium(), .large()]
          self.present(certificateViewController, animated: true)
        }
      }
    }
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
          $0.cosmeticFilteringTabHelper?.resetSelectorsCache()
        }
      }
    }

    // Update shield status, reload the this tab, reset selectors cache in quickview mode
    refreshShieldStatus(url: currentTab?.visibleURL ?? url)
    currentTab?.reload()
    currentTab?.cosmeticFilteringTabHelper?.resetSelectorsCache()
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

  private func openNewTab(with request: URLRequest, inPrivateMode: Bool) {
    dismiss(animated: true) { [weak self] in
      guard let self else { return }
      self.currentTab?.removeObserver(self.toolbarViewModel)
      self.currentTab?.removeObserver(self)
      self.onOpenInNewTab?(request, inPrivateMode)
    }
  }

  private func handleToolbarVisibilityStateChange() {
    guard !isKeyboardVisible else {
      return
    }

    let state = toolbarVisibilityViewModel.toolbarState
    let progress = toolbarVisibilityViewModel.interactiveTransitionProgress
    let maxOffset = toolbarVisibilityViewModel.transitionDistance

    if let p = progress {
      let collapseProgress: CGFloat
      switch state {
      case .expanded:
        collapseProgress = p
      case .collapsed:
        collapseProgress = 1 - p
      }
      toolbarViewModel.collapseProgress = collapseProgress
      toolbarBottomConstraint?.constant = maxOffset * collapseProgress
      view.layoutIfNeeded()
      return
    }

    let targetOffset: CGFloat = state == .expanded ? 0 : maxOffset
    toolbarViewModel.collapseProgress = state == .expanded ? 0 : 1
    toolbarBottomConstraint?.constant = targetOffset
    let animator = toolbarVisibilityViewModel.toolbarChangePropertyAnimator
    animator.addAnimations { self.view.layoutIfNeeded() }
    animator.startAnimation()
  }

  private func updateKeyboardGuide(intersectionHeight: CGFloat) {
    guard let constraint = keyboardGuideVisibleConstraints else { return }
    constraint.top.constant = view.bounds.height - intersectionHeight
    constraint.width.constant = view.bounds.width
    constraint.height.constant = intersectionHeight
  }

  private func setKeyboardGuideVisible(_ visible: Bool) {
    if visible {
      NSLayoutConstraint.deactivate(keyboardGuideHiddenConstraints)
      if let constraint = keyboardGuideVisibleConstraints {
        NSLayoutConstraint.activate([
          constraint.leading, constraint.top, constraint.width, constraint.height,
        ])
      }
    } else {
      if let constraint = keyboardGuideVisibleConstraints {
        NSLayoutConstraint.deactivate([
          constraint.leading, constraint.top, constraint.width, constraint.height,
        ])
      }
      NSLayoutConstraint.activate(keyboardGuideHiddenConstraints)
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
      currentTab.historyTabHelper = nil
      self.onAttachTab?(currentTab)
      self.onOpenInNewTab?(request, profile.isOffTheRecord)
    }
    return nil
  }

  func tab(
    _ tab: some TabState,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration? {
    guard let url = linkURL, url.isWebPage() else {
      return UIContextMenuConfiguration(identifier: nil, previewProvider: nil, actionProvider: nil)
    }

    let actionProvider: UIContextMenuActionProvider = { [weak self] _ -> UIMenu? in
      guard let self else { return nil }
      var actions = [UIAction]()

      if let currentTab = self.currentTab {
        let isPrivate = currentTab.isPrivate

        if !isPrivate {
          let openNewTabAction = UIAction(
            title: Strings.openNewTabButtonTitle,
            image: UIImage(braveSystemNamed: "leo.browser.mobile-tab-new")
          ) { [weak self] _ in
            self?.openNewTab(with: URLRequest(url: url), inPrivateMode: false)
          }

          openNewTabAction.accessibilityLabel = "linkContextMenu.openInNewTab"
          actions.append(openNewTabAction)
        }

        let openNewPrivateTabAction = UIAction(
          title: Strings.openNewPrivateTabButtonTitle,
          image: UIImage(braveSystemNamed: "leo.product.private-window")
        ) { [weak self] _ in
          guard let self else { return }
          if !isPrivate, Preferences.Privacy.privateBrowsingLock.value {
            self.askForLocalAuthentication { [weak self] success, error in
              if success {
                self?.openNewTab(with: URLRequest(url: url), inPrivateMode: true)
              }
            }
          } else {
            self.openNewTab(with: URLRequest(url: url), inPrivateMode: true)
          }
        }
        openNewPrivateTabAction.accessibilityLabel = "linkContextMenu.openInNewPrivateTab"

        actions.append(openNewPrivateTabAction)

        if UIApplication.shared.supportsMultipleScenes {
          if !isPrivate {
            let openNewWindowAction = UIAction(
              title: Strings.openInNewWindowTitle,
              image: UIImage(braveSystemNamed: "leo.window.tab-new")
            ) { [weak self] _ in
              self?.onOpenInNewWindow?(url, false)
            }

            openNewWindowAction.accessibilityLabel = "linkContextMenu.openInNewWindow"
            actions.append(openNewWindowAction)
          }

          let openNewPrivateWindowAction = UIAction(
            title: Strings.openInNewPrivateWindowTitle,
            image: UIImage(braveSystemNamed: "leo.window.tab-private")
          ) { [weak self] _ in
            guard let self else { return }
            if !isPrivate, Preferences.Privacy.privateBrowsingLock.value {
              self.askForLocalAuthentication { [weak self] success, error in
                if success {
                  self?.onOpenInNewWindow?(url, true)
                }
              }
            } else {
              self.onOpenInNewWindow?(url, true)
            }
          }

          openNewPrivateWindowAction.accessibilityLabel = "linkContextMenu.openInNewPrivateWindow"
          actions.append(openNewPrivateWindowAction)
        }

        let copyAction = UIAction(
          title: Strings.copyLinkActionTitle,
          image: UIImage(braveSystemNamed: "leo.copy"),
          handler: UIAction.deferredActionHandler { _ in
            UIPasteboard.general.url = url as URL
          }
        )
        copyAction.accessibilityLabel = "linkContextMenu.copyLink"
        actions.append(copyAction)

        let copyCleanLinkAction = UIAction(
          title: Strings.copyCleanLink,
          image: UIImage(braveSystemNamed: "leo.copy.clean"),
          handler: UIAction.deferredActionHandler { _ in
            let service = URLSanitizerServiceFactory.get(privateMode: currentTab.isPrivate)
            let cleanedURL = service?.sanitize(url: url) ?? url
            UIPasteboard.general.url = cleanedURL
          }
        )
        copyCleanLinkAction.accessibilityLabel = "linkContextMenu.copyCleanLink"
        actions.append(copyCleanLinkAction)

        let shareAction = UIAction(
          title: Strings.shareLinkActionTitle,
          image: UIImage(braveSystemNamed: "leo.share.macos")
        ) { [weak self] _ in
          guard let self else { return }
          let anchorView = self.toolbarHostingController.rootView.shareBackgroundView.uiView
          self.presentShareActivity(
            url: url,
            tab: currentTab,
            syncAPI: self.syncAPI,
            sendTabAPI: self.sendTabAPI,
            feedDataSource: nil,
            isBraveNewsAvailable: false,
            source: .init(
              view: anchorView,
              rect: anchorView.bounds,
              arrowDirection: .any
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
        }
        shareAction.accessibilityLabel = "linkContextMenu.share"
        actions.append(shareAction)

        let linkPreview = Preferences.General.enableLinkPreview.value

        let linkPreviewTitle =
          linkPreview ? Strings.hideLinkPreviewsActionTitle : Strings.showLinkPreviewsActionTitle
        let linkPreviewAction = UIAction(
          title: linkPreviewTitle,
          image: UIImage(braveSystemNamed: linkPreview ? "leo.eye.off" : "leo.eye.on")
        ) { _ in
          Preferences.General.enableLinkPreview.value.toggle()
        }

        actions.append(linkPreviewAction)
      }

      let formattedURL = URLFormatter.formatURL(
        url.absoluteString,
        formatTypes: [.omitDefaults, .omitTrivialSubdomains],
        unescapeOptions: .normal
      )
      return UIMenu(title: formattedURL, children: actions)
    }

    let linkPreview: UIContextMenuContentPreviewProvider? = { [unowned self, weak tab] in
      guard let tab else { return nil }
      return LinkPreviewViewController(
        url: url,
        for: tab,
        policyDecider: currentTab?.detachedPrivacyHelper,
        tabDelegate: self,
        downloadDelegate: nil
      )
    }

    let linkPreviewProvider = Preferences.General.enableLinkPreview.value ? linkPreview : nil
    return UIContextMenuConfiguration(
      identifier: nil,
      previewProvider: linkPreviewProvider,
      actionProvider: actionProvider
    )
  }

  func tabWebViewDidClose(_ tab: some TabState) {
    dismiss(animated: true)
  }
}

// MARK: - TabObserver

extension QuickViewController: TabObserver {
  func tabDidCreateWebView(_ tab: some TabState) {
    if !FeatureList.kUseProfileWebViewConfiguration.enabled,
      let detachedTabPrivacyHelper = DetachedTabPrivacyHelper(tab: tab)
    {
      tab.detachedPrivacyHelper = detachedTabPrivacyHelper
    }
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    if let scrollView = tab.webViewProxy?.scrollView {
      toolbarVisibilityViewModel.beginObservingScrollView(scrollView)
    }
    if tab.visibleURL?.isInternalURL(for: .readermode) != true {
      hideReaderModeBar()
    }
    toolbarVisibilityViewModel.toolbarState = .expanded
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    refreshShieldStatus(url: tab.visibleURL ?? url)
    if tab.visibleURL?.isInternalURL(for: .readermode) != true {
      hideReaderModeBar()
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    if let scrollView = tab.webViewProxy?.scrollView {
      toolbarVisibilityViewModel.endScrollViewObservation(scrollView)
    }
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

// MARK: - KeyboardHelperDelegate

extension QuickViewController: KeyboardHelperDelegate {
  func keyboardHelper(
    _ keyboardHelper: Shared.KeyboardHelper,
    keyboardWillShowWithState state: Shared.KeyboardState
  ) {
    let height = state.intersectionHeightForView(view)
    let isForWebContent = currentTab?.webViewProxy?.isKeyboardVisible == true
    let isForFindInPage = currentTab?.isFindNavigatorVisible == true
    guard height > 0, isForWebContent || isForFindInPage || state.isLocal else { return }

    updateKeyboardGuide(intersectionHeight: height)
    setKeyboardGuideVisible(true)

    if !isKeyboardVisible {
      // First show: capture pre-keyboard toolbar state
      preKeyboardToolbarState = toolbarVisibilityViewModel.toolbarState
      isKeyboardVisible = true
      toolbarVisibilityViewModel.isEnabled = false
      toolbarViewModel.collapseProgress = 1
      toolbarBottomConstraint?.constant = toolbarVisibilityViewModel.transitionDistance
    }
    // hw -> sw: guide already updated to full keyboard frame above

    UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
      self.view.layoutIfNeeded()
    }.startAnimation()
  }

  func keyboardHelper(
    _ keyboardHelper: Shared.KeyboardHelper,
    keyboardWillHideWithState state: Shared.KeyboardState
  ) {
    let height = state.intersectionHeightForView(view)
    // Always update the guide frame (handles sw -> hw repositioning)
    updateKeyboardGuide(intersectionHeight: height)

    if currentTab?.isFindNavigatorVisible == true {
      // Defer one run loop: isFindNavigatorVisible is unreliable synchronously
      guard isKeyboardVisible else { return }
      DispatchQueue.main.async { [weak self] in
        guard let self else { return }
        if self.currentTab?.isFindNavigatorVisible == true {
          // sw -> hw while find-in-page: guide updated, stay visible
          self.setKeyboardGuideVisible(true)
        } else {
          self.restoreToolbarAfterKeyboardDismiss(state: state)
        }
      }
      return
    }

    // sw -> hw for regular web content: height > 0 means keyboard still partially present
    if height > 0, currentTab?.webViewProxy?.isKeyboardVisible == true {
      setKeyboardGuideVisible(true)
      UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
        self.view.layoutIfNeeded()
      }.startAnimation()
      return
    }

    restoreToolbarAfterKeyboardDismiss(state: state)
  }

  private func restoreToolbarAfterKeyboardDismiss(state: KeyboardState) {
    guard isKeyboardVisible else { return }
    isKeyboardVisible = false
    setKeyboardGuideVisible(false)

    let restoredOffset: CGFloat
    switch preKeyboardToolbarState {
    case .collapsed:
      restoredOffset = toolbarVisibilityViewModel.transitionDistance
      toolbarViewModel.collapseProgress = 1
    default:
      restoredOffset = 0
      toolbarViewModel.collapseProgress = 0
    }
    toolbarBottomConstraint?.constant = restoredOffset

    let animator = UIViewPropertyAnimator(
      duration: state.animationDuration,
      curve: state.animationCurve
    ) {
      self.view.layoutIfNeeded()
    }
    animator.addCompletion { _ in self.toolbarVisibilityViewModel.isEnabled = true }
    animator.startAnimation()
  }
}
