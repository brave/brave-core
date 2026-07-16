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
  let toolbarVisibilityViewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 110)
  private var toolbarVisibilityCancellable: AnyCancellable?
  private var toolbarBottomConstraint: Constraint?
  private let onOpenInNewTab: ((URLRequest) -> Void)?
  private let onAttachTab: ((any TabState) -> Void)?

  private var isKeyboardVisible: Bool = false
  private var preKeyboardToolbarState: ToolbarVisibilityViewModel.ToolbarState?
  private var toolbarHeightConstraint: Constraint?
  private var toolbarFullHeight: CGFloat = 0

  init(
    url: URL,
    profile: any Profile,
    syncAPI: BraveSyncAPI,
    sendTabAPI: BraveSendTabAPI,
    onOpenInNewTab: ((URLRequest) -> Void)?,
    onAttachTab: ((any TabState) -> Void)?
  ) {
    self.url = url
    self.profile = profile
    self.syncAPI = syncAPI
    self.sendTabAPI = sendTabAPI
    self.toolbarViewModel = QuickViewToolbarModel(
      url: url,
      isPrivate: profile.isOffTheRecord
    )
    self.onOpenInNewTab = onOpenInNewTab
    self.onAttachTab = onAttachTab
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .fullScreen
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
      toolbarVisibilityViewModel.transitionDistance =
        toolbarHeight - collapsedHeight
        - (traitCollection.userInterfaceIdiom == .pad ? view.safeAreaInsets.bottom : 0)
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
  }

  private func setupUI() {
    guard let currentTab = currentTab else {
      return
    }
    let colors: BrowserColors = profile.isOffTheRecord ? .privateMode : .standard
    view.backgroundColor = colors.chromeBackground
    view.addSubview(currentTab.view)

    toolbarHostingController.view.backgroundColor = colors.chromeBackground
    addChild(toolbarHostingController)
    view.addSubview(toolbarHostingController.view)
    toolbarHostingController.sizingOptions = [.intrinsicContentSize]
    toolbarHostingController.didMove(toParent: self)

    currentTab.view.snp.makeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
    }
    toolbarHostingController.view.snp.makeConstraints {
      $0.leading.trailing.equalTo(view)
      toolbarBottomConstraint = $0.bottom.equalTo(view.safeAreaLayoutGuide.snp.bottom).constraint
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

  private func handleToolbarVisibilityStateChange() {
    guard !isKeyboardVisible else { return }

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
      toolbarBottomConstraint?.update(offset: maxOffset * collapseProgress)
      view.layoutIfNeeded()
      return
    }
    // snapped state
    let targetOffset: CGFloat = state == .expanded ? 0 : maxOffset
    toolbarViewModel.collapseProgress = state == .expanded ? 0 : 1
    toolbarBottomConstraint?.update(offset: targetOffset)
    let animator = toolbarVisibilityViewModel.toolbarChangePropertyAnimator
    animator.addAnimations { self.view.layoutIfNeeded() }
    animator.startAnimation()
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
}

// MARK: - TabObserver

extension QuickViewController: TabObserver {
  func tabDidCreateWebView(_ tab: some TabState) {
    if let detachedTabPrivacyHelper = DetachedTabPrivacyHelper(
      tab: tab
    ) {
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
    let keyboardHeight = state.intersectionHeightForView(view)
    guard keyboardHeight > 0, state.isLocal, !isKeyboardVisible
    else { return }
    isKeyboardVisible = true
    preKeyboardToolbarState = toolbarVisibilityViewModel.toolbarState

    toolbarVisibilityViewModel.isEnabled = false
    toolbarViewModel.collapseProgress = 1

    let collapsedHeight = QuickViewToolbarView.collapsedHeight(compatibleWith: traitCollection)
    let offset = view.safeAreaInsets.bottom - keyboardHeight
    UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
      self.toolbarBottomConstraint?.update(offset: offset)
      self.toolbarHeightConstraint?.update(offset: collapsedHeight)
      self.view.layoutIfNeeded()
    }.startAnimation()
  }

  func keyboardHelper(
    _ keyboardHelper: Shared.KeyboardHelper,
    keyboardWillHideWithState state: Shared.KeyboardState
  ) {
    guard isKeyboardVisible else { return }
    isKeyboardVisible = false

    let restoredOffset: CGFloat
    switch preKeyboardToolbarState {
    case .collapsed:
      restoredOffset = toolbarVisibilityViewModel.transitionDistance
      toolbarViewModel.collapseProgress = 1
    default:
      restoredOffset = 0
      toolbarViewModel.collapseProgress = 0
    }

    let animator = UIViewPropertyAnimator(
      duration: state.animationDuration,
      curve: state.animationCurve
    ) {
      self.toolbarBottomConstraint?.update(offset: restoredOffset)
      self.toolbarHeightConstraint?.update(offset: self.toolbarFullHeight)
      self.view.layoutIfNeeded()
    }
    animator.addCompletion { _ in
      self.isKeyboardVisible = false
      self.toolbarVisibilityViewModel.isEnabled = true
    }
    animator.startAnimation()
  }
}
