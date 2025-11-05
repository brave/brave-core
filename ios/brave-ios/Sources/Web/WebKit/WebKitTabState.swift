// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CertificateUtilities
import Combine
import FaviconModels
import Foundation
import OSLog
import OrderedCollections
import Shared
import WebKit

class WebKitTabState: TabState, TabStateImpl {
  init(id: UUID, configuration: WKWebViewConfiguration) {
    self.id = id
    self.initialConfiguration = configuration
    self.isPrivate = !configuration.websiteDataStore.isPersistent
    self.navigationHandler = TabWKNavigationHandler(tab: self)
    self.uiHandler = TabWKUIHandler(tab: self)
  }

  deinit {
    deleteWebView()
    observers.forEach {
      $0.tabWillBeDestroyed(self)
    }
  }

  var webView: WKWebView?
  private var navigationHandler: TabWKNavigationHandler?
  private var uiHandler: TabWKUIHandler?
  private var webViewObservations: [AnyCancellable] = []
  private var _data: OSAllocatedUnfairLock<TabDataValues> = .init(uncheckedState: .init())
  private var virtualURL: URL?

  /// In-memory dictionary of websites that were explicitly set to use either desktop or mobile user agent.
  /// Key is url's base domain
  /// Each tab has separate list of website overrides.
  private var userAgentOverrides: [String: UserAgentType] = [:]
  func userAgentTypeForURL(_ url: URL) -> UserAgentType {
    url.baseDomain.flatMap { userAgentOverrides[$0] } ?? .automatic
  }

  func updateSecureContentState() async {
    visibleSecureContentState = await secureContentState
  }

  func updateSecureContentStateAndNotifyObserversIfNeeded() {
    Task { @MainActor in
      let currentState = visibleSecureContentState
      await updateSecureContentState()
      if currentState != visibleSecureContentState {
        observers.forEach {
          $0.tabDidChangeVisibleSecurityState(self)
        }
      }
    }
  }

  @MainActor private var secureContentState: SecureContentState {
    get async {
      guard let webView = webView, let committedURL = self.lastCommittedURL else {
        return .unknown
      }
      if let internalURL = InternalURL(committedURL), internalURL.isAboutHomeURL {
        // New Tab Page is a special case, should be treated as `unknown` instead of `localhost`
        return .unknown
      }
      if webView.url != committedURL {
        // URL has not been committed yet, so we will not evaluate the secure status of the page yet
        return visibleSecureContentState
      }
      if let internalURL = InternalURL(committedURL) {
        if internalURL.isErrorPage, let errorCode = internalURL.certificateErrorForErrorPage,
          errorCode != 0
        {
          return .invalidCertificate
        }
        return .localhost
      }
      if committedURL.scheme?.lowercased() == "http" {
        return .missingSSL
      }
      if let serverTrust = webView.serverTrust,
        case let origin = committedURL.origin, !origin.isOpaque
      {
        let isMixedContent = !webView.hasOnlySecureContent
        let result = await BraveCertificateUtils.verifyTrust(
          serverTrust,
          host: origin.host,
          port: Int(origin.port)
        )
        switch result {
        case 0:
          // Cert is valid
          return isMixedContent ? .mixedContent : .secure
        case Int(Int32.min):
          // Cert is valid but should be validated by the system
          // Let the system handle it and we'll show an error if the system cannot validate it
          do {
            try await BraveCertificateUtils.evaluateTrust(serverTrust, for: origin.host)
            return isMixedContent ? .mixedContent : .secure
          } catch {
            return .invalidCertificate
          }
        default:
          return .invalidCertificate
        }
      }
      return .unknown
    }
  }

  private func webViewURLDidChange(_ newURL: URL?) {
    var notifyObservers: Bool = false

    // Special case for "about:blank" popups, if the webView.url is nil, keep the tab url as "about:blank"
    if visibleURL?.absoluteString == "about:blank" && newURL == nil {
      return
    }

    // To prevent spoofing, only change the URL immediately if the new URL is on
    // the same origin as the current URL. Otherwise, do nothing and wait for
    // didCommitNavigation to confirm the page load.
    if visibleURL?.origin == newURL?.origin {
      visibleURL = newURL
      notifyObservers = true
    } else {
      if isVisible, visibleURL?.displayURL?.scheme == "about", !isLoading {
        if let newURL {
          visibleURL = newURL
          notifyObservers = true
        }
      }
    }

    if notifyObservers {
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }

    updateSecureContentStateAndNotifyObserversIfNeeded()
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
    observers.forEach {
      $0.tabDidChangeBackForwardState(self)
    }
  }

  private func webViewTitleDidChange() {
    if let webView, let url = webView.url,
      webView.configuration.preferences.isFraudulentWebsiteWarningEnabled,
      webView.responds(to: Selector(("_safeBrowsingWarning"))),
      webView.value(forKey: "_safeBrowsingWarning") != nil
    {
      self.visibleURL = url  // We can update the URL whenever showing an interstitial warning
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }

    observers.forEach {
      $0.tabDidChangeTitle(self)
    }
  }

  private func webViewSecurityStateDidChange() {
    updateSecureContentStateAndNotifyObserversIfNeeded()
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
        \.url,
        options: [.new],
        changeHandler: { [weak self] _, change in
          guard let newValue = change.newValue else { return }
          self?.webViewURLDidChange(newValue)
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
        \.hasOnlySecureContent,
        changeHandler: { [weak self] _, _ in
          self?.webViewSecurityStateDidChange()
        }
      ),
      webView.observe(
        \.serverTrust,
        changeHandler: { [weak self] _, _ in
          self?.webViewSecurityStateDidChange()
        }
      ),
    ]
    let stringKeyObservers = [
      StringKeyPathObserver<WKWebView, UIColor>(
        object: webView,
        keyPath: "_sampl\("edPageTopC")olor",
        changeHandler: { [weak self] _ in
          self?.webViewSampledPageTopColorDidChange()
        }
      )
    ]
    webViewObservations.append(
      contentsOf: keyValueObservations.map { observation in .init { observation.invalidate() } }
    )
    webViewObservations.append(
      contentsOf: stringKeyObservers.map { observation in .init { observation.invalidate() } }
    )
  }

  private func detachWebObservers() {
    webViewObservations.removeAll()
  }

  // MARK: - TabState

  let id: UUID
  let isPrivate: Bool

  var data: TabDataValues {
    get { _data.withLock { $0 } }
    set { _data.withLock { $0 = newValue } }
  }

  var view: UIView = .init()
  var opener: (any TabState)?
  var isVisible: Bool = false {
    didSet {
      if isVisible {
        lastActiveTime = .now
      }
      for observer in observers {
        if isVisible {
          observer.tabWasShown(self)
        } else {
          observer.tabWasHidden(self)
        }
      }
    }
  }
  var lastActiveTime: Date? = .now

  var webViewProxy: (any WebViewProxy)? {
    webView.flatMap { WKWebViewProxy(webView: $0) }
  }

  var isWebViewCreated: Bool {
    webView != nil
  }

  func createWebView() {
    if webView != nil {
      return
    }

    // We need to ensure that each tab has isolated WKUserContentController & WKPreferences, because
    // as of now we specifically adjust these values per web view created rather than when the
    // configuration is created.
    //
    // This must happen prior to WKWebView's creation.
    let configuration = initialConfiguration
    configuration.userContentController = .init()
    configuration.preferences = initialConfiguration.preferences.copy() as! WKPreferences

    let webView = WebKitWebView(frame: .zero, configuration: configuration)
    webView.navigationDelegate = navigationHandler
    webView.uiDelegate = uiHandler
    webView.buildMenu = { [weak self] builder in
      guard let self else { return }
      self.delegate?.tab(self, buildEditMenuWithBuilder: builder)
    }
    webView.allowsBackForwardNavigationGestures = true
    webView.allowsLinkPreview = true
    webView.isFindInteractionEnabled = true
    webView.isInspectable = true

    view.addSubview(webView)
    webView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      webView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      webView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      webView.topAnchor.constraint(equalTo: view.topAnchor),
      webView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
    self.webView = webView

    attachWebObservers()

    didCreateWebView()
  }

  func deleteWebView() {
    observers.forEach {
      $0.tabWillDeleteWebView(self)
    }
    detachWebObservers()
    webView?.removeFromSuperview()
    webView = nil
  }

  var visibleSecureContentState: SecureContentState = .unknown
  var serverTrust: SecTrust? {
    webView?.serverTrust
  }
  var favicon: FaviconModels.Favicon?
  var visibleURL: URL?
  var url: URL? {
    return webView?.url ?? visibleURL
  }
  var lastCommittedURL: URL? {
    didSet {
      visibleURL = lastCommittedURL
      previousCommittedURL = oldValue
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }
  }
  var previousCommittedURL: URL?
  var contentsMimeType: String?
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
    webView?.interactionState as? Data
  }
  func restore(using sessionData: Data) {
    webView?.interactionState = sessionData
  }
  var canGoBack: Bool {
    webView?.canGoBack ?? false
  }
  var canGoForward: Bool {
    webView?.canGoForward ?? false
  }
  var backForwardList: (any BackForwardListProxy)? {
    webView.flatMap { WebKitBackForwardList($0.backForwardList) }
  }
  var redirectChain: [URL] = []
  var currentInitialURL: URL? {
    webView?.backForwardList.currentItem?.initialURL
  }
  var isRestoring: Bool = false
  var currentUserAgentType: UserAgentType {
    if let urlString = visibleURL?.baseDomain, let override = userAgentOverrides[urlString] {
      return override
    }
    if let delegate, let visibleURL {
      let type = delegate.tab(self, defaultUserAgentTypeForURL: visibleURL)
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
    self.visibleURL = url
  }

  func loadHTMLString(_ html: String, baseURL: URL?) {
    webView?.loadHTMLString(html, baseURL: baseURL)
  }

  func reload() {
    // Clear the user agent before further navigation.
    // User Agent setting happens in BVC's WKNavigationDelegate.
    // This prevents a bug with back-forward list, going back or forward and reloading the tab
    // loaded wrong user agent.
    webView?.customUserAgent = nil
    webView?.reload()
  }

  func reloadWithUserAgentType(_ userAgentType: UserAgentType) {
    if let urlString = visibleURL?.baseDomain {
      userAgentOverrides[urlString] = userAgentType
    }
    reload()
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
    guard let wkItem = (item as? WebKitBackForwardList.Item)?.item else { return }
    _ = webView?.go(to: wkItem)
  }

  var canTakeSnapshot: Bool {
    true
  }

  // There is a bug in WebKit where if you call -[WKWebView takeSnapshotWithConfiguration:...]
  // multiple times without waiting for the result the completion handlers are disassociated with
  // the caller, which means if you use Swift Concurrency you can't rely on the closure being
  // called (leaked continuation), and you cant rely on it being called only once (resumed
  // continuation more than once), so we have to ensure we use the standard closure based
  // implementation and allow the calling of `handler` more than once using a standard closure
  func takeSnapshot(rect: CGRect, handler: @escaping (UIImage?) -> Void) {
    guard let webView else {
      handler(nil)
      return
    }
    let configuration = WKSnapshotConfiguration()
    configuration.rect = rect

    webView.takeSnapshot(with: configuration) { image, error in
      if let error {
        Logger.module.error("Failed to take snapshot for web view: \(error)")
        handler(nil)
        return
      }
      handler(image)
    }
  }

  @MainActor func createFullPagePDF() async throws -> Data? {
    guard let webView else { return nil }
    return try await withCheckedThrowingContinuation { continuation in
      webView.createPDF { result in
        continuation.resume(with: result)
      }
    }
  }

  func presentFindInteraction(with text: String) {
    guard let findInteraction = webView?.findInteraction else { return }
    findInteraction.searchText = text
    findInteraction.presentFindNavigator(showingReplace: false)
  }

  func dismissFindInteraction() {
    webView?.findInteraction?.dismissFindNavigator()
  }

  func evaluateJavaScriptUnsafe(_ javascript: String) {
    webView?.evaluateJavaScript(javascript, completionHandler: nil)
  }

  func evaluateJavaScript(
    functionName: String,
    args: [Any],
    frame: WKFrameInfo?,
    contentWorld: WKContentWorld,
    escapeArgs: Bool,
    asFunction: Bool
  ) async throws -> Any? {
    try await webView?.evaluateJavaScript(
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
    try await webView?.callAsyncJavaScript(
      functionBody,
      arguments: arguments,
      in: frame,
      contentWorld: contentWorld
    )
  }

  var configuration: WKWebViewConfiguration {
    if let webView {
      return webView.configuration
    }
    return initialConfiguration
  }
  var initialConfiguration: WKWebViewConfiguration {
    didSet {
      assert(
        self.isPrivate == !initialConfiguration.websiteDataStore.isPersistent,
        "Underlying data store peristance must remain the same"
      )
    }
  }

  var viewPrintFormatter: UIViewPrintFormatter? {
    return webView?.viewPrintFormatter()
  }

  var dataForDisplayedPDF: Data? {
    webView?.dataForDisplayedPDF
  }

  var sampledPageTopColor: UIColor? {
    webView?.sampledPageTopColor
  }

  var viewScale: CGFloat {
    get {
      webView?.viewScale ?? 1
    }
    set {
      webView?.viewScale = newValue
    }
  }

  func clearBackForwardList() {
    webView?.backForwardList.clear()
  }

  func updateScripts() {
    // Nothing to do
  }

  // MARK: - TabStateImpl

  weak var delegate: TabDelegate?
  weak var downloadDelegate: TabDownloadDelegate?
  var observers: OrderedSet<AnyTabObserver> = []
  var policyDeciders: OrderedSet<AnyTabPolicyDecider> = []
}

private class WebKitWebView: WKWebView {
  var buildMenu: ((any UIMenuBuilder) -> Void)?

  override func buildMenu(with builder: any UIMenuBuilder) {
    super.buildMenu(with: builder)
    buildMenu?(builder)
  }
}

// Unfortunately neccessary because UIScrollView is optional in WebViewProxy
private struct WKWebViewProxy: WebViewProxy {
  var webView: WKWebView

  var scrollView: UIScrollView? {
    webView.scrollView
  }

  var bounds: CGRect {
    webView.bounds
  }

  var frame: CGRect {
    webView.frame
  }

  func becomeFirstResponder() -> Bool {
    webView.becomeFirstResponder()
  }
}

private struct WebKitBackForwardList: BackForwardListProxy {
  struct Item: BackForwardListItemProxy {
    var item: WKBackForwardListItem

    var url: URL {
      item.url
    }
    var title: String? {
      item.title
    }
  }

  init(_ backForwardList: WKBackForwardList) {
    self.backForwardList = backForwardList
  }
  var backForwardList: WKBackForwardList

  var backList: [any BackForwardListItemProxy] {
    backForwardList.backList.map(Item.init)
  }
  var forwardList: [any BackForwardListItemProxy] {
    backForwardList.forwardList.map(Item.init)
  }
  var currentItem: (any BackForwardListItemProxy)? {
    backForwardList.currentItem.flatMap(Item.init)
  }
  var backItem: (any BackForwardListItemProxy)? {
    backForwardList.backItem.flatMap(Item.init)
  }
  var forwardItem: (any BackForwardListItemProxy)? {
    backForwardList.forwardItem.flatMap(Item.init)
  }
}
