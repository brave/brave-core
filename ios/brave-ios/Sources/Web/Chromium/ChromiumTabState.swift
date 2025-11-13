// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import FaviconModels
import Foundation
import OrderedCollections
import Shared
import WebKit
import os

class ChromiumTabState: TabState, TabStateImpl {
  init(
    id: UUID,
    configuration: CWVWebViewConfiguration,
    wkConfiguration: WKWebViewConfiguration?
  ) {
    self.id = id
    self.cwvConfiguration = configuration
    self.wkConfiguration = wkConfiguration
    if let wkConfiguration {
      assert(
        configuration.isPersistent == wkConfiguration.websiteDataStore.isPersistent,
        "Persistance of configurations must match"
      )
    }
    self.navigationHandler = .init(tab: self)
    self.uiHandler = .init(tab: self)
  }

  var webView: BraveWebView?
  var cwvConfiguration: CWVWebViewConfiguration
  var wkConfiguration: WKWebViewConfiguration?
  private var containerView: CWVContainerView = .init()
  private var navigationHandler: TabCWVNavigationHandler?
  private var uiHandler: TabCWVUIHandler?
  private var webViewObservations: [AnyCancellable] = []
  private var virtualURL: URL?

  private func webViewURLDidChange(_ newURL: URL?) {
    if newURL != nil {
      virtualURL = nil
    }
    observers.forEach {
      $0.tabDidUpdateURL(self)
    }
  }

  private func webViewLastCommittedURLChanged(_ previousValue: URL?) {
    previousCommittedURL = previousValue
  }

  private func webViewIsLoadingDidChange(_ isLoading: Bool) {
    observers.forEach {
      if isLoading {
        $0.tabDidStartLoading(self)
      } else {
        $0.tabDidStopLoading(self)
      }
    }
  }

  private func webViewProgressDidChange() {
    observers.forEach {
      $0.tabDidChangeLoadProgress(self)
    }
  }

  private func webviewBackForwardStateDidChange() {
    backForwardList = webView.flatMap {
      ChromiumBackForwardList(webView: $0)
    }
    observers.forEach {
      $0.tabDidChangeBackForwardState(self)
    }
  }

  private func webViewTitleDidChange() {
    if let webView = webView?.internalWebView, let url = webView.url,
      webView.configuration.preferences.isFraudulentWebsiteWarningEnabled,
      webView.responds(to: Selector(("_safeBrowsingWarning"))),
      webView.value(forKey: "_safeBrowsingWarning") != nil
    {
      self.virtualURL = url  // We can update the URL whenever showing an interstitial warning
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }

    observers.forEach {
      $0.tabDidChangeTitle(self)
    }
  }

  private func webViewSecurityStateDidChange() {
    observers.forEach {
      $0.tabDidChangeVisibleSecurityState(self)
    }
  }

  private func webViewSampledPageTopColorDidChange() {
    observers.forEach {
      $0.tabDidChangeSampledPageTopColor(self)
    }
  }

  private func attachWebObservers() {
    guard let webView else { return }

    let keyValueObservations = [
      webView.observe(
        \.visibleURL,
        options: [.new],
        changeHandler: { [weak self] _, change in
          guard let newValue = change.newValue else { return }
          self?.webViewURLDidChange(newValue)
        }
      ),
      webView.observe(
        \.lastCommittedURL,
        options: [.old],
        changeHandler: { [weak self] _, change in
          guard let oldValue = change.oldValue else { return }
          self?.webViewLastCommittedURLChanged(oldValue)
        }
      ),
      webView.observe(
        \.isLoading,
        options: [.new],
        changeHandler: { [weak self] _, change in
          guard let newValue = change.newValue else { return }
          self?.webViewIsLoadingDidChange(newValue)
        }
      ),
      webView.observe(
        \.estimatedProgress,
        changeHandler: { [weak self] _, _ in
          self?.webViewProgressDidChange()
        }
      ),
      webView.observe(
        \.canGoBack,
        changeHandler: { [weak self] _, _ in
          self?.webviewBackForwardStateDidChange()
        }
      ),
      webView.observe(
        \.canGoForward,
        changeHandler: { [weak self] _, _ in
          self?.webviewBackForwardStateDidChange()
        }
      ),
      webView.observe(
        \.title,
        changeHandler: { [weak self] _, _ in
          self?.webViewTitleDidChange()
        }
      ),
      webView.observe(
        \.visibleSSLStatus,
        changeHandler: { [weak self] _, _ in
          self?.webViewSecurityStateDidChange()
        }
      ),
    ]
    webViewObservations.append(
      contentsOf: keyValueObservations.map { observation in .init { observation.invalidate() } }
    )
    if let webView = webView.internalWebView {
      let sampledPageTopColorObservation =
        StringKeyPathObserver<WKWebView, UIColor>(
          object: webView,
          keyPath: "_sampl\("edPageTopC")olor",
          changeHandler: { [weak self] _ in
            self?.webViewSampledPageTopColorDidChange()
          }
        )
      webViewObservations.append(
        .init { sampledPageTopColorObservation.invalidate() }
      )
    }
  }

  private func detachWebObservers() {
    webViewObservations.removeAll()
  }

  // MARK: - Tab

  var id: UUID
  var isPrivate: Bool {
    !cwvConfiguration.isPersistent
  }

  var data: TabDataValues {
    get { _data.withLock { $0 } }
    set { _data.withLock { $0 = newValue } }
  }
  private var _data: OSAllocatedUnfairLock<TabDataValues> = .init(uncheckedState: .init())

  var view: UIView {
    containerView
  }
  var opener: (any TabState)?
  var isVisible: Bool = false {
    didSet {
      containerView.webView = isVisible ? webView : nil
      for observer in observers {
        if isVisible {
          observer.tabWasShown(self)
        } else {
          observer.tabWasHidden(self)
        }
      }
    }
  }
  var lastActiveTime: Date? {
    webView?.lastActiveTime
  }

  var webViewProxy: (any WebViewProxy)? {
    webView
  }
  var isWebViewCreated: Bool {
    webView != nil
  }
  func createWebView() {
    if webView != nil {
      return
    }
    CWVWebView.webInspectorEnabled = true
    var createdWKWebView: WKWebView?
    let webView = BraveWebView(
      frame: .init(width: 1, height: 1),
      configuration: cwvConfiguration,
      wkConfiguration: wkConfiguration,
      createdWKWebView: &createdWKWebView
    )
    webView.navigationDelegate = navigationHandler
    webView.uiDelegate = uiHandler
    webView.allowsBackForwardNavigationGestures = true
    webView.allowsLinkPreview = true

    self.webView = webView

    if isVisible {
      containerView.webView = webView
    }

    attachWebObservers()

    if createdWKWebView != nil {
      // CWVWebView only creates the underlying WKWebView if you pass in a WKWebViewConfiguration.
      // When a new web view is created via window.open we must wait until WebState creates the
      // underlying web view using the configuration passed by WebKit
      //
      // See: `TabCWVUIHandler.webView(_:createWebViewWith:for)` and
      //      `TabCWVUIHandler.webViewDidCreateNewWebView`
      didCreateWebView()
    }
  }

  func deleteWebView() {
    observers.forEach {
      $0.tabWillDeleteWebView(self)
    }
    detachWebObservers()
    webView?.removeFromSuperview()
    webView = nil
  }

  weak var delegate: TabDelegate?
  weak var downloadDelegate: TabDownloadDelegate?

  var visibleSecureContentState: SecureContentState {
    guard let lastCommittedURL = lastCommittedURL else { return .unknown }

    let isAppSpecificURL =
      lastCommittedURL.scheme == "brave" || lastCommittedURL.scheme == "chrome"
      || InternalURL.isValid(url: lastCommittedURL)
    if isAppSpecificURL {
      if let internalURL = InternalURL(lastCommittedURL), internalURL.isAboutHomeURL {
        // New Tab Page is a special case, should be treated as `unknown` instead of `localhost`
        return .unknown
      }
      return .localhost
    }

    guard let visibleSSLStatus = webView?.visibleSSLStatus else { return .unknown }
    switch visibleSSLStatus.securityStyle {
    case .authenticated:
      if !visibleSSLStatus.hasOnlySecureContent {
        return .mixedContent
      }
      return .secure
    case .authenticationBroken:
      return .invalidCertificate
    case .unauthenticated:
      return .missingSSL
    case .unknown:
      return .unknown
    @unknown default:
      return .unknown
    }
  }
  var serverTrust: SecTrust? {
    return webView?.visibleSSLStatus?.certificate?.createServerTrust()
  }
  var favicon: Favicon?
  var url: URL? {
    visibleURL
  }
  var visibleURL: URL? {
    virtualURL ?? webView?.visibleURL
  }
  var lastCommittedURL: URL? {
    webView?.lastCommittedURL
  }
  var previousCommittedURL: URL?
  var contentsMimeType: String? {
    webView?.contentsMIMEType
  }
  var title: String? {
    webView?.title
  }
  var isLoading: Bool {
    webView?.isLoading ?? false
  }
  var estimatedProgress: Double {
    webView?.estimatedProgress ?? 0
  }
  var sessionData: Data? {
    guard let webView else { return nil }
    let archiver = NSKeyedArchiver(requiringSecureCoding: false)
    webView.encodeRestorableState(with: archiver)
    return archiver.encodedData
  }

  func restore(using sessionData: Data) throws {
    if sessionData.isEmpty {
      throw TabRestorationError.invalidData
    }
    guard let webView, CWVWebView.isRestoreDataValid(sessionData) else {
      Logger.module.error("Failed to restore web view with invalid session data")
      throw TabRestorationError.invalidData
    }
    do {
      isRestoring = true
      let coder = try NSKeyedUnarchiver(forReadingFrom: sessionData)
      coder.requiresSecureCoding = false
      webView.decodeRestorableState(with: coder)
    } catch {
      Logger.module.error("Failed to restore web view with session data: \(error)")
      isRestoring = false
      throw TabRestorationError.invalidData
    }
  }

  var canGoBack: Bool {
    webView?.canGoBack ?? false
  }
  var canGoForward: Bool {
    webView?.canGoForward ?? false
  }
  var backForwardList: (any BackForwardListProxy)?

  var redirectChain: [URL] = []

  var currentInitialURL: URL? {
    webView?.originalRequestURLForLastCommitedNavigation
  }

  var isRestoring: Bool = false

  var currentUserAgentType: UserAgentType {
    if let webView {
      return .init(webView.currentItemUserAgentType())
    }
    if let delegate, let url {
      let type = delegate.tab(self, defaultUserAgentTypeForURL: url)
      if type != .automatic, type != .none {
        return type
      }
    }
    return .mobile
  }

  func loadRequest(_ request: URLRequest) {
    webView?.load(request)
  }

  func setVirtualURL(_ url: URL?) {
    virtualURL = url
  }

  func reload() {
    webView?.reload()
  }

  func reloadWithUserAgentType(_ userAgentType: UserAgentType) {
    webView?.reload(withUserAgentType: .init(userAgentType))
  }

  func stopLoading() {
    webView?.stopLoading()
  }

  func goBack() {
    webView?.goBack()
  }

  func goForward() {
    webView?.goForward()
  }

  func goToBackForwardListItem(_ item: any BackForwardListItemProxy) {
    guard let item = (item as? ChromiumBackForwardList.Item)?.item else { return }
    webView?.go(to: item)
  }

  var canTakeSnapshot: Bool {
    webView?.canTakeSnapshot() ?? false
  }

  func takeSnapshot(rect: CGRect, handler: @escaping (UIImage?) -> Void) {
    guard let webView else {
      handler(nil)
      return
    }
    webView.takeSnapshot(with: rect, completionHandler: handler)
  }

  @MainActor func createFullPagePDF() async throws -> Data? {
    return await webView?.createFullPagePDF()
  }

  func presentFindInteraction(with text: String) {
    webView?.findInPageController.findString(inPage: text)
  }

  func dismissFindInteraction() {
    webView?.findInPageController.stopFindInPage()
  }

  func evaluateJavaScriptUnsafe(_ javascript: String) {
    webView?.evaluateJavaScript(javascript)
  }

  func loadHTMLString(_ html: String, baseURL: URL?) {
    webView?.internalWebView?.loadHTMLString(html, baseURL: baseURL)
  }

  @MainActor func evaluateJavaScript(
    functionName: String,
    args: [Any],
    frame: WKFrameInfo?,
    contentWorld: WKContentWorld,
    escapeArgs: Bool,
    asFunction: Bool
  ) async throws -> Any? {
    try await webView?.internalWebView?.evaluateJavaScript(
      functionName: functionName,
      args: args,
      frame: frame,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )
  }

  @MainActor func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any],
    in frame: WKFrameInfo?,
    contentWorld: WKContentWorld
  ) async throws -> Any? {
    try await webView?.internalWebView?.callAsyncJavaScript(
      functionBody,
      arguments: arguments,
      in: frame,
      contentWorld: contentWorld
    )
  }

  var configuration: WKWebViewConfiguration {
    if let configuration = webView?.internalWebView?.configuration {
      return configuration
    }
    return wkConfiguration ?? .init()
  }

  var dataForDisplayedPDF: Data? {
    return webView?.internalWebView?.dataForDisplayedPDF
  }

  var sampledPageTopColor: UIColor? {
    return webView?.internalWebView?.sampledPageTopColor
  }

  var viewPrintFormatter: UIViewPrintFormatter? {
    // We can technically get the print formatter from `WebState::GetView()` as that returns
    // the underlying CRWContainerView which exposes the underlying `WKWebView`'s viewPrintFormatter
    // but for now this is enough.
    return webView?.internalWebView?.viewPrintFormatter()
  }

  var viewScale: CGFloat {
    get {
      webView?.internalWebView?.viewScale ?? 1
    }
    set {
      webView?.internalWebView?.viewScale = newValue
    }
  }

  func clearBackForwardList() {
    webView?.internalWebView?.backForwardList.clear()
  }

  func updateScripts() {
    webView?.updateScripts()
  }

  // MARK: - TabImpl

  var observers: OrderedSet<AnyTabObserver> = []
  var policyDeciders: OrderedSet<AnyTabPolicyDecider> = []
}

extension CWVWebView: WebViewProxy {}

extension CWVUserAgentType {
  init(_ userAgentType: UserAgentType) {
    switch userAgentType {
    case .none: self = .none
    case .automatic: self = .automatic
    case .desktop: self = .desktop
    case .mobile: self = .mobile
    }
  }
}

extension UserAgentType {
  init(_ userAgentType: CWVUserAgentType) {
    switch userAgentType {
    case .none: self = .none
    case .automatic: self = .automatic
    case .desktop: self = .desktop
    case .mobile: self = .mobile
    default: self = .none
    }
  }
}

private struct ChromiumBackForwardList: BackForwardListProxy {
  struct Item: BackForwardListItemProxy {
    var item: CWVBackForwardListItem

    var url: URL {
      item.url
    }
    var title: String? {
      item.title
    }
  }

  init(webView: CWVWebView) {
    // Current item is fine to get from backForwardList as it just uses GetLastCommittedItem
    self.currentItem = webView.backForwardList?.currentItem.flatMap(Item.init)
    // The rest we want to use our own exposed `backList`/`forwardList` methods to get the
    // version that doesn't use CWVBackForwardListItemArray
    if webView.canGoBack {
      let backItems = webView.backList
      self.backItem = backItems.first.flatMap(Item.init)
      // Reverse the back list to mimic WKWebView's back forward list implementation
      self.backList = backItems.reversed().map(Item.init)
    }
    if webView.canGoForward {
      let forwardItems = webView.forwardList
      self.forwardItem = forwardItems.first.flatMap(Item.init)
      self.forwardList = forwardItems.map(Item.init)
    }
  }

  var backList: [any BackForwardListItemProxy] = []
  var forwardList: [any BackForwardListItemProxy] = []
  var currentItem: (any BackForwardListItemProxy)?
  var backItem: (any BackForwardListItemProxy)?
  var forwardItem: (any BackForwardListItemProxy)?
}

/// A simple container view used to maintain the visibility and layout of the containing CWVWebView
///
/// CWVWebView calls `WasShown`/`WasHidden` based on being in the view hierarchy
class CWVContainerView: UIView {
  var webView: CWVWebView? {
    willSet {
      webView?.removeFromSuperview()
    }
    didSet {
      if let webView {
        addSubview(webView)
        setNeedsLayout()
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    var webViewFrame = bounds
    // CWVWebView must not have a zero size even if the parent has none
    if webViewFrame.width == 0 || webViewFrame.height == 0 {
      webViewFrame.size = .init(width: 1, height: 1)
    }
    webView?.frame = webViewFrame
  }
}
