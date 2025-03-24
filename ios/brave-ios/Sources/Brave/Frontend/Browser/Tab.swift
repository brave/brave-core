// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import BraveWallet
import CertificateUtilities
import Combine
import Data
import Favicon
import Foundation
import Preferences
import Shared
import Storage
import SwiftyJSON
import UserAgent
import WebKit
import os.log

protocol TabContentScriptLoader {
  static func loadUserScript(named: String) -> String?
  static func secureScript(handlerName: String, securityToken: String, script: String) -> String
  static func secureScript(
    handlerNamesMap: [String: String],
    securityToken: String,
    script: String
  ) -> String
}

protocol TabContentScript: TabContentScriptLoader {
  static var scriptName: String { get }
  static var scriptId: String { get }
  static var messageHandlerName: String { get }
  static var scriptSandbox: WKContentWorld { get }
  static var userScript: WKUserScript? { get }

  func verifyMessage(message: WKScriptMessage) -> Bool
  func verifyMessage(message: WKScriptMessage, securityToken: String) -> Bool

  @MainActor func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  )
}

protocol TabDelegate {
  func showRequestRewardsPanel(_ tab: Tab)
  func stopMediaPlayback(_ tab: Tab)
  func showWalletNotification(_ tab: Tab, origin: URLOrigin)
  func updateURLBarWalletButton()
  func isTabVisible(_ tab: Tab) -> Bool
}

struct RewardsTabChangeReportingState {
  /// Set to true when the resulting page was restored from session state.
  var wasRestored = false
  /// Set to true when the resulting page navigation is not a reload or a
  /// back/forward type.
  var isNewNavigation = true
  /// HTTP status code of the resulting page.
  var httpStatusCode = -1
}

enum TabSecureContentState: String {
  case unknown = "Unknown"
  case localhost = "Localhost"
  case secure = "Secure"
  case invalidCert = "InvalidCertificate"
  case missingSSL = "SSL Certificate Missing"
  case mixedContent = "Mixed Content"

  var shouldDisplayWarning: Bool {
    switch self {
    case .unknown, .invalidCert, .missingSSL, .mixedContent:
      return true
    case .localhost, .secure:
      return false
    }
  }
}

@dynamicMemberLookup
class Tab: NSObject {
  let id: UUID

  var data: TabDataValues {
    get { _data.withLock { $0 } }
    set { _data.withLock { $0 = newValue } }
  }
  private var _data: OSAllocatedUnfairLock<TabDataValues> = .init(uncheckedState: .init())

  subscript<Value>(dynamicMember member: KeyPath<TabDataValues, Value>) -> Value {
    return data[keyPath: member]
  }
  subscript<Value>(dynamicMember member: WritableKeyPath<TabDataValues, Value>) -> Value {
    get { data[keyPath: member] }
    set { data[keyPath: member] = newValue }
  }

  private(set) var type: TabType = .regular

  var redirectChain = [URL]()
  var responses = [URL: URLResponse]()

  var isPrivate: Bool {
    return type.isPrivate
  }

  private var webViewObservations: [AnyCancellable] = []

  private var observers: Set<AnyTabObserver> = .init()
  private var policyDeciders: Set<AnyTabWebPolicyDecider> = .init()

  // WebKit handlers
  private var navigationHandler: TabWKNavigationHandler?
  private var uiHandler: TabWKUIHandler?

  var certStore: CertStore?
  weak var webDelegate: TabWebDelegate?
  weak var downloadDelegate: TabDownloadDelegate?

  func addPolicyDecider(_ policyDecider: some TabWebPolicyDecider) {
    policyDeciders.insert(.init(policyDecider))
  }

  func removePolicyDecider(_ policyDecider: some TabWebPolicyDecider) {
    if let policyDecider = policyDeciders.first(where: { $0.id == ObjectIdentifier(policyDecider) })
    {
      policyDeciders.remove(policyDecider)
    }
  }

  func addObserver(_ observer: some TabObserver) {
    observers.insert(.init(observer))
  }

  func removeObserver(_ observer: some TabObserver) {
    if let observer = observers.first(where: { $0.id == ObjectIdentifier(observer) }) {
      observers.remove(observer)
    }
  }

  private(set) var lastKnownSecureContentState: TabSecureContentState = .unknown
  func updateSecureContentState() async {
    lastKnownSecureContentState = await secureContentState
  }
  @MainActor private var secureContentState: TabSecureContentState {
    get async {
      guard let webView = webView, let committedURL = self.committedURL else {
        return .unknown
      }
      if let internalURL = InternalURL(committedURL), internalURL.isAboutHomeURL {
        // New Tab Page is a special case, should be treated as `unknown` instead of `localhost`
        return .unknown
      }
      if webView.url != committedURL {
        // URL has not been committed yet, so we will not evaluate the secure status of the page yet
        return lastKnownSecureContentState
      }
      if let internalURL = InternalURL(committedURL) {
        if internalURL.isErrorPage, ErrorPageHelper.certificateError(for: committedURL) != 0 {
          return .invalidCert
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
            return .invalidCert
          }
        default:
          return .invalidCert
        }
      }
      return .unknown
    }
  }

  var sslPinningError: Error?

  private var webView: TabWebView?
  // Should only be used by internal Tab code, will be removed in the future when Tab can live
  // in its own SPM target
  var internalTabWebViewDoNotUse: TabWebView? { webView }
  var viewPrintFormatter: UIPrintFormatter? { webView?.viewPrintFormatter() }
  var webContentView: UIView? { webView }
  var webScrollView: UIScrollView? { webView?.scrollView }
  var serverTrust: SecTrust? { webView?.serverTrust }
  var sessionData: Data? {
    webView?.interactionState as? Data
  }
  var pageZoomLevel: CGFloat {
    get {
      webView?.value(forKey: PageZoomHandler.propertyName) as? CGFloat ?? 1
    }
    set {
      webView?.setValue(newValue, forKey: PageZoomHandler.propertyName)
    }
  }
  var sampledPageTopColor: UIColor? {
    webView?.sampledPageTopColor
  }
  var tabDelegate: TabDelegate?
  var favicon: Favicon
  var lastExecutedTime: Timestamp?
  fileprivate var lastRequest: URLRequest?
  var isRestoring: Bool = false

  /// The url set after a successful navigation. This will also set the `url` property.
  ///
  /// - Note: Unlike the `url` property, which may be set during pre-navigation,
  /// the `committedURL` is only assigned when navigation was committed..
  var committedURL: URL? {
    willSet {
      url = newValue
      previousComittedURL = committedURL
    }
    didSet {
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
      // Server Trust and URL is also updated in didCommit
      // However, WebKit does NOT trigger the `serverTrust` observer when the URL changes, but the trust has not.
      // WebKit also does NOT trigger the `serverTrust` observer when the page is actually insecure (non-https).
      // So manually trigger it with the current trust.
      Task { @MainActor [self] in
        let state = lastKnownSecureContentState
        await updateSecureContentState()
        if state != lastKnownSecureContentState {
          observers.forEach {
            $0.tabDidChangeVisibleSecurityState(self)
          }
        }
      }
    }
  }

  /// The previous url that was set before `comittedURL` was set again
  private(set) var previousComittedURL: URL?

  /// The URL that is loaded by the web view, but has not committed yet
  var visibleURL: URL? {
    if let webView {
      return webView.url
    }
    return url
  }

  private(set) var url: URL? {
    didSet {
      if let _url = url, let internalUrl = InternalURL(_url), internalUrl.isAuthorized {
        url = URL(string: internalUrl.stripAuthorization)
      }

      if data.browserData?.isDisplayingBasicAuthPrompt == true {
        url = URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
      }

      // Setting URL in SyncTab is adding pending item to navigation manager on brave-core side
      if let url = url, !isPrivate, !url.isLocal, !InternalURL.isValid(url: url),
        !url.isInternalURL(for: .readermode)
      {
        data.browserData?.syncTab?.setURL(url)
      }
    }
  }

  /// Sets the URL of the tab that is not directly tied to the underlying web view.
  ///
  /// This should only be used in certain scenarios such as restoring a tab, creating a child tab as
  /// typically the `url` property is updated based on web navigations.
  ///
  /// - warning: This does not notify TabObserver's that the url has changed
  func setVirtualURL(_ url: URL?) {
    self.url = url
  }

  var mimeType: String?

  /// The last title shown by this tab. Used by the tab tray to show titles for zombie tabs.
  var lastTitle: String?

  var isDesktopSite: Bool {
    webView?.customUserAgent?.lowercased().contains("mobile") == false
  }

  var containsWebPage: Bool {
    if let url = url {
      let isHomeURL = InternalURL(url)?.isAboutHomeURL
      return url.isWebPage() && isHomeURL != true
    }

    return false
  }

  /// In-memory dictionary of websites that were explicitly set to use either desktop or mobile user agent.
  /// Key is url's base domain, value is desktop mode on or off.
  /// Each tab has separate list of website overrides.
  private var userAgentOverrides: [String: Bool] = [:]

  // If this tab has been opened from another, its parent will point to the tab from which it was opened
  weak var parent: Tab?

  var configuration: WKWebViewConfiguration {
    if let webView {
      return webView.configuration
    }
    return initialConfiguration
  }
  private var initialConfiguration: WKWebViewConfiguration

  init(
    configuration: WKWebViewConfiguration,
    id: UUID = UUID(),
    type: TabType = .regular
  ) {
    self.initialConfiguration = configuration
    self.id = id
    self.type = type

    self.favicon = Favicon.default
    super.init()

    self.navigationHandler = TabWKNavigationHandler(tab: self)
    self.uiHandler = TabWKUIHandler(tab: self)
  }

  private lazy var refreshControl = UIRefreshControl().then {
    $0.addTarget(self, action: #selector(reload), for: .valueChanged)
  }

  var isWebViewCreated: Bool {
    webView != nil
  }

  func createWebview() {
    if webView == nil {
      configuration.userContentController = WKUserContentController()
      configuration.preferences = WKPreferences()
      configuration.preferences.javaScriptCanOpenWindowsAutomatically = false
      configuration.preferences.isFraudulentWebsiteWarningEnabled =
        Preferences.Shields.googleSafeBrowsing.value
      configuration.allowsInlineMediaPlayback = true
      // Enables Zoom in website by ignoring their javascript based viewport Scale limits.
      configuration.ignoresViewportScaleLimits = true
      configuration.upgradeKnownHostsToHTTPS = ShieldPreferences.httpsUpgradeLevel.isEnabled
      configuration.enablePageTopColorSampling()

      if configuration.urlSchemeHandler(forURLScheme: InternalURL.scheme) == nil {
        configuration.setURLSchemeHandler(
          InternalSchemeHandler(tab: self),
          forURLScheme: InternalURL.scheme
        )
      }

      let webView = TabWebView(
        frame: .zero,
        configuration: configuration,
        isPrivate: isPrivate
      )
      webView.buildEditMenuWithBuilder = { [weak self] builder in
        guard let self else { return }
        self.webDelegate?.tab(self, buildEditMenuWithBuilder: builder)
      }
      webView.accessibilityLabel = Strings.webContentAccessibilityLabel
      webView.allowsBackForwardNavigationGestures = true
      webView.allowsLinkPreview = true
      webView.navigationDelegate = navigationHandler
      webView.uiDelegate = uiHandler

      // Turning off masking allows the web content to flow outside of the scrollView's frame
      // which allows the content appear beneath the toolbars in the BrowserViewController
      webView.scrollView.layer.masksToBounds = false

      if let request = lastRequest {
        webView.load(request)
      }

      self.webView = webView
      attachWebObservers()

      observers.forEach {
        $0.tab(self, didCreateWebView: webView)
      }
    }
  }

  private func webViewURLDidChange(_ newURL: URL?) {
    var notifyObservers: Bool = false

    // Special case for "about:blank" popups, if the webView.url is nil, keep the tab url as "about:blank"
    if url?.absoluteString == "about:blank" && newURL == nil {
      return
    }

    // To prevent spoofing, only change the URL immediately if the new URL is on
    // the same origin as the current URL. Otherwise, do nothing and wait for
    // didCommitNavigation to confirm the page load.
    if url?.origin == newURL?.origin {
      url = newURL
      notifyObservers = true
    } else {
      if tabDelegate?.isTabVisible(self) == true, url?.displayURL != nil,
        url?.displayURL?.scheme == "about", !loading
      {
        if let newURL {
          url = newURL
          notifyObservers = true
        }
      }
    }

    updatePullToRefreshVisibility()

    if notifyObservers {
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }

    Task { @MainActor in
      let currentState = lastKnownSecureContentState
      await updateSecureContentState()
      if currentState != lastKnownSecureContentState {
        observers.forEach {
          $0.tabDidChangeVisibleSecurityState(self)
        }
      }
    }
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
      self.url = url  // We can update the URL whenever showing an interstitial warning
      observers.forEach {
        $0.tabDidUpdateURL(self)
      }
    }

    observers.forEach {
      $0.tabDidChangeTitle(self)
    }
  }

  private func webViewSecurityStateDidChange() {
    Task { @MainActor in
      await updateSecureContentState()
      observers.forEach {
        $0.tabDidChangeVisibleSecurityState(self)
      }
    }
  }

  private func webViewSampledPageTopColorDidChange() {
    observers.forEach {
      $0.tabDidChangeSampledPageTopColor(self)
    }
  }

  private class StringKeyPathObserver<Object: NSObject, Value>: NSObject {
    weak var object: Object?
    let keyPath: String
    let changeHandler: (Value?) -> Void
    var isInvalidated: Bool = false

    init(object: Object, keyPath: String, changeHandler: @escaping (Value?) -> Void) {
      self.object = object
      self.keyPath = keyPath
      self.changeHandler = changeHandler
      super.init()
      object.addObserver(self, forKeyPath: keyPath, options: .new, context: nil)
    }

    func invalidate() {
      if isInvalidated { return }
      defer { isInvalidated = true }
      object?.removeObserver(self, forKeyPath: keyPath)
    }

    deinit {
      invalidate()
    }

    override func observeValue(
      forKeyPath keyPath: String?,
      of object: Any?,
      change: [NSKeyValueChangeKey: Any]?,
      context: UnsafeMutableRawPointer?
    ) {
      if self.keyPath != keyPath { return }
      let newValue = change?[.newKey] as? Value
      changeHandler(newValue)
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

  func resetWebView(config: WKWebViewConfiguration) {
    initialConfiguration = config
    deleteWebView()
  }

  func clearHistory(config: WKWebViewConfiguration) {
    guard let webView = webView else {
      return
    }

    // Clear selector is used on WKWebView backForwardList because backForwardList list is only exposed with a getter
    // and this method Removes all items except the current one in the tab list so when another url is added it will add the list properly
    // This approach is chosen to achieve removing tab history in the event of removing  browser history
    // Best way perform this is to clear the backforward list and in our case there is no drawback to clear the list
    // And alternative would be to reload webpages which will be costly and also can cause unexpected results
    let argument: [Any] = ["_c", "lea", "r"]

    let method = argument.compactMap { $0 as? String }.joined()
    let selector: Selector = NSSelectorFromString(method)

    if webView.backForwardList.responds(to: selector) {
      webView.backForwardList.performSelector(
        onMainThread: selector,
        with: nil,
        waitUntilDone: true
      )
    }

    // Remove the tab history from saved tabs
    // To remove history from WebKit, we simply update the session data AFTER calling "clear" above
    SessionTab.update(
      tabId: id,
      interactionState: webView.sessionData ?? Data(),
      title: title ?? "",
      url: webView.url ?? TabManager.ntpInteralURL
    )
  }

  func restore(restorationData: (title: String, interactionState: Data)?) {
    guard let webView else { return }
    restore(webView, restorationData: restorationData)
  }

  func restore(requestRestorationData: (title: String, request: URLRequest)?) {
    guard let webView else { return }
    restore(webView, requestRestorationData: requestRestorationData)
  }

  private func restore(
    _ webView: WKWebView,
    restorationData: (title: String, interactionState: Data)?
  ) {
    // Pulls restored session data from a previous SavedTab to load into the Tab. If it's nil, a session restore
    // has already been triggered via custom URL, so we use the last request to trigger it again; otherwise,
    // we extract the information needed to restore the tabs and create a NSURLRequest with the custom session restore URL
    // to trigger the session restore via custom handlers
    if let sessionInfo = restorationData {
      isRestoring = true
      lastTitle = sessionInfo.title
      webView.interactionState = sessionInfo.interactionState
    } else if let request = lastRequest {
      webView.load(request)
    } else {
      Logger.module.warning(
        "creating webview with no lastRequest and no session data: \(self.url?.absoluteString ?? "nil")"
      )
    }
  }

  private func restore(
    _ webView: WKWebView,
    requestRestorationData: (title: String, request: URLRequest)?
  ) {
    if let sessionInfo = requestRestorationData {
      isRestoring = true
      lastTitle = sessionInfo.title
      webView.load(sessionInfo.request)
    } else if let request = lastRequest {
      webView.load(request)
    } else {
      Logger.module.warning(
        "creating webview with no lastRequest and no session data: \(self.url?.absoluteString ?? "nil")"
      )
    }
  }

  func deleteWebView() {
    if let webView = webView {
      observers.forEach {
        $0.tab(self, willDeleteWebView: webView)
      }
    }
    detachWebObservers()
    webView = nil
  }

  deinit {
    deleteWebView()
    observers.forEach {
      $0.tabWillBeDestroyed(self)
    }
  }

  var loading: Bool {
    return webView?.isLoading ?? false
  }

  var estimatedProgress: Double {
    return webView?.estimatedProgress ?? 0
  }

  var backList: [BackForwardList.Item]? {
    return backForwardList.backList
  }

  var forwardList: [BackForwardList.Item]? {
    return backForwardList.forwardList
  }

  var title: String? {
    return webView?.title
  }

  var displayTitle: String {
    if let displayTabTitle = fetchDisplayTitle(using: url, title: title) {
      return displayTabTitle
    }

    // When picking a display title. Tabs with sessionData are pending a restore so show their old title.
    // To prevent flickering of the display title. If a tab is restoring make sure to use its lastTitle.
    if let url = self.url, InternalURL(url)?.isAboutHomeURL ?? false, !isRestoring {
      return Strings.Hotkey.newTabTitle
    }

    if let url = self.url, !InternalURL.isValid(url: url),
      let shownUrl = url.displayURL?.absoluteString, webView != nil
    {
      return shownUrl
    }

    guard let lastTitle = lastTitle, !lastTitle.isEmpty else {
      // FF uses url?.displayURL?.absoluteString ??  ""
      if let title = url?.absoluteString {
        return title
      } else if let tab = SessionTab.from(tabId: id) {
        if tab.title.isEmpty {
          return Strings.Hotkey.newTabTitle
        }
        return tab.title
      }

      return ""
    }

    return lastTitle
  }

  var currentInitialURL: URL? {
    return self.webView?.backForwardList.currentItem?.initialURL
  }

  var displayFavicon: Favicon? {
    if let url = url, InternalURL(url)?.isAboutHomeURL == true {
      return Favicon(
        image: UIImage(sharedNamed: "brave.logo"),
        isMonogramImage: false,
        backgroundColor: .clear
      )
    }
    return favicon
  }

  var canGoBack: Bool {
    return webView?.canGoBack ?? false
  }

  var canGoForward: Bool {
    return webView?.canGoForward ?? false
  }

  /// This property is for fetching the actual URL for the Tab
  /// In private browsing the URL is in memory but this is not the case for normal mode
  /// For Normal  Mode Tab information is fetched using Tab ID from
  var fetchedURL: URL? {
    if isPrivate {
      if let url = url, url.isWebPage() {
        return url
      }
    } else {
      if let tabUrl = url, tabUrl.isWebPage() {
        return tabUrl
      } else if let fetchedTab = SessionTab.from(tabId: id), fetchedTab.url?.isWebPage() == true {
        return url
      }
    }

    return nil
  }

  func fetchDisplayTitle(using url: URL?, title: String?) -> String? {
    if let tabTitle = title, !tabTitle.isEmpty {
      var displayTitle = tabTitle

      // Checking host is "localhost" || host == "127.0.0.1"
      // or hostless URL (iOS forwards hostless URLs (e.g., http://:6571) to localhost.)
      // DisplayURL will retrieve original URL even it is redirected to Error Page
      if let isLocal = url?.displayURL?.isLocal, isLocal {
        displayTitle = ""
      }

      return displayTitle
    }

    return nil
  }

  func goBack() {
    _ = webView?.goBack()
  }

  func goForward() {
    _ = webView?.goForward()
  }

  func goToBackForwardListItem(_ item: BackForwardList.Item) {
    guard let wkItem = item.item else { return }
    _ = webView?.go(to: wkItem)
  }

  func loadHTMLString(_ html: String, baseURL: URL?) {
    webView?.loadHTMLString(html, baseURL: baseURL)
  }

  @discardableResult func loadRequest(_ request: URLRequest) -> WKNavigation? {
    if let webView = webView {
      lastRequest = request
      sslPinningError = nil

      if let url = request.url {
        // Donate Custom Intent Open Website
        if url.isSecureWebPage(), !isPrivate {
          ActivityShortcutManager.shared.donateCustomIntent(
            for: .openWebsite,
            with: url.absoluteString
          )
        }
      }

      return webView.load(request)
    }
    return nil
  }

  func stop() {
    webView?.stopLoading()
  }

  @objc func reload() {
    // Clear the user agent before further navigation.
    // Proper User Agent setting happens in BVC's WKNavigationDelegate.
    // This prevents a bug with back-forward list, going back or forward and reloading the tab
    // loaded wrong user agent.
    webView?.customUserAgent = nil

    defer {
      if let refreshControl = webView?.scrollView.refreshControl,
        refreshControl.isRefreshing
      {
        refreshControl.endRefreshing()
      }
    }

    // If the current page is an error page, and the reload button is tapped, load the original URL
    if let url = webView?.url, let internalUrl = InternalURL(url),
      let page = internalUrl.originalURLFromErrorPage
    {
      webView?.replaceLocation(with: page)
      return
    }

    if let _ = webView?.reloadFromOrigin() {
      Logger.module.debug("reloaded zombified tab from origin")
      return
    }

    if let request = lastRequest {
      Logger.module.debug("restoring webView from scratch")
      loadRequest(request)
    }
  }

  func updateUserAgent(_ webView: WKWebView, newURL: URL) {
    guard let baseDomain = newURL.baseDomain else { return }

    let screenWidth = webView.currentScene?.screen.bounds.width ?? webView.bounds.size.width
    if webView.traitCollection.horizontalSizeClass == .compact
      && (webView.bounds.size.width < screenWidth / 2.0)
    {
      let desktopMode = userAgentOverrides[baseDomain] == true
      webView.customUserAgent = desktopMode ? UserAgent.desktop : UserAgent.mobile
      return
    }

    let desktopMode = userAgentOverrides[baseDomain] ?? UserAgent.shouldUseDesktopMode()
    webView.customUserAgent = desktopMode ? UserAgent.desktop : UserAgent.mobile
  }

  func hideContent(_ animated: Bool = false) {
    webView?.isUserInteractionEnabled = false
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.webView?.alpha = 0.0
        }
      )
    } else {
      webView?.alpha = 0.0
    }
  }

  func showContent(_ animated: Bool = false) {
    webView?.isUserInteractionEnabled = true
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.webView?.alpha = 1.0
        }
      )
    } else {
      webView?.alpha = 1.0
    }
  }

  /// Switches user agent Desktop -> Mobile or Mobile -> Desktop.
  func switchUserAgent() {
    if let urlString = webView?.url?.baseDomain {
      // The website was changed once already, need to flip the override.onScreenshotUpdated
      if let siteOverride = userAgentOverrides[urlString] {
        userAgentOverrides[urlString] = !siteOverride
      } else {
        // First time switch, adding the basedomain to dictionary with flipped value.
        userAgentOverrides[urlString] = !UserAgent.shouldUseDesktopMode()
      }
    }

    reload()
  }

  func updatePullToRefreshVisibility() {
    guard let url = webView?.url, let webView = webView else { return }
    webView.scrollView.refreshControl =
      url.isLocalUtility || !Preferences.General.enablePullToRefresh.value ? nil : refreshControl
  }

  func isDescendentOf(_ ancestor: Tab) -> Bool {
    return sequence(first: parent) { $0?.parent }.contains { $0 == ancestor }
  }

  func stopMediaPlayback() {
    tabDelegate?.stopMediaPlayback(self)
  }

  private func resolvedPolicyDecision(
    for policyDeciders: Set<AnyTabWebPolicyDecider>,
    task: @escaping (AnyTabWebPolicyDecider) async -> WebPolicyDecision
  ) async -> WebPolicyDecision {
    let result = await withTaskGroup(of: WebPolicyDecision.self, returning: WebPolicyDecision.self)
    { group in
      for policyDecider in policyDeciders {
        group.addTask { @MainActor in
          if Task.isCancelled {
            return .cancel
          }
          let decision = await task(policyDecider)
          if decision == .cancel {
            return .cancel
          }
          return .allow
        }
      }
      for await result in group {
        if result == .cancel {
          group.cancelAll()
          return .cancel
        }
      }
      return .allow
    }
    return result
  }

  func shouldAllowRequest(
    _ request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    return await resolvedPolicyDecision(for: policyDeciders) { policyDecider in
      await policyDecider.tab(
        self,
        shouldAllowRequest: request,
        requestInfo: requestInfo
      )
    }
  }

  func shouldAllowResponse(
    _ response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    return await resolvedPolicyDecision(for: policyDeciders) { policyDecider in
      await policyDecider.tab(
        self,
        shouldAllowResponse: response,
        responseInfo: responseInfo
      )
    }
  }

  func didStartNavigation() {
    observers.forEach {
      $0.tabDidStartNavigation(self)
    }
  }

  func didCommitNavigation() {
    observers.forEach {
      $0.tabDidCommitNavigation(self)
    }
  }

  func didFinishNavigation() {
    observers.forEach {
      $0.tabDidFinishNavigation(self)
    }
  }

  func didFailNavigation(with error: Error) {
    observers.forEach {
      $0.tab(self, didFailNavigationWithError: error)
    }
  }
}

class TabWebView: WKWebView, MenuHelperInterface {
  /// Stores last position when the webview was touched on.
  private(set) var lastHitPoint = CGPoint(x: 0, y: 0)

  private static var nonPersistentDataStore: WKWebsiteDataStore?
  static func sharedNonPersistentStore() -> WKWebsiteDataStore {
    if let dataStore = nonPersistentDataStore {
      return dataStore
    }

    let dataStore = WKWebsiteDataStore.nonPersistent()
    nonPersistentDataStore = dataStore
    return dataStore
  }

  fileprivate var buildEditMenuWithBuilder: ((any UIMenuBuilder) -> Void)?

  fileprivate init(
    frame: CGRect,
    configuration: WKWebViewConfiguration = WKWebViewConfiguration(),
    isPrivate: Bool = true
  ) {
    if isPrivate {
      configuration.websiteDataStore = TabWebView.sharedNonPersistentStore()
    } else {
      configuration.websiteDataStore = WKWebsiteDataStore.default()
    }

    super.init(frame: frame, configuration: configuration)

    isFindInteractionEnabled = true

    customUserAgent = UserAgent.userAgentForIdiom()
    if #available(iOS 16.4, *) {
      isInspectable = true
    }

    updateBackgroundColor()
    Preferences.General.nightModeEnabled.observe(from: self)
  }

  private func updateBackgroundColor() {
    if Preferences.General.nightModeEnabled.value {
      let color = UIColor(braveSystemName: .containerBackground)
      backgroundColor = color
      scrollView.backgroundColor = color
      // WKWebView flashes white screen on load regardless of background colour assignments without
      // setting `isOpaque` to false
      isOpaque = false
    } else {
      backgroundColor = nil
      scrollView.backgroundColor = nil
      isOpaque = true
    }
  }

  static func removeNonPersistentStore() {
    TabWebView.nonPersistentDataStore = nil
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    lastHitPoint = point
    return super.hitTest(point, with: event)
  }

  override func buildMenu(with builder: any UIMenuBuilder) {
    super.buildMenu(with: builder)
    buildEditMenuWithBuilder?(builder)
  }
}

extension TabWebView: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    updateBackgroundColor()
  }
}

// MARK: - Brave Search

extension Tab {
  /// Call the api on the Brave Search website and passes the fallback results to it.
  /// Important: This method is also called when there is no fallback results
  /// or when the fallback call should not happen at all.
  /// The website expects the iOS device to always call this method(blocks on it).
  func injectResults() {
    DispatchQueue.main.async {
      // If the backup search results happen before the Brave Search loads
      // The method we pass data to is undefined.
      // For such case we do not call that method or remove the search backup manager.

      self.webView?.evaluateJavaScript("window.onFetchedBackupResults === undefined") {
        result,
        error in

        if let error = error {
          Logger.module.error(
            "onFetchedBackupResults existence check error: \(error.localizedDescription, privacy: .public)"
          )
        }

        guard let methodUndefined = result as? Bool else {
          Logger.module.error(
            "onFetchedBackupResults existence check, failed to unwrap bool result value"
          )
          return
        }

        if methodUndefined {
          Logger.module.info("Search Backup results are ready but the page has not been loaded yet")
          return
        }

        var queryResult = "null"

        if let url = self.url,
          BraveSearchManager.isValidURL(url),
          let result = self.braveSearchManager?.fallbackQueryResult
        {
          queryResult = result
        }

        self.evaluateSafeJavaScript(
          functionName: "window.onFetchedBackupResults",
          args: [queryResult],
          contentWorld: BraveSearchScriptHandler.scriptSandbox,
          escapeArgs: false
        )

        // Cleanup
        self.braveSearchManager = nil
      }
    }
  }
}

// MARK: - Brave SKU
extension Tab {
  func injectLocalStorageItem(key: String, value: String) {
    self.evaluateSafeJavaScript(
      functionName: "localStorage.setItem",
      args: [key, value],
      contentWorld: BraveSkusScriptHandler.scriptSandbox
    )
  }
}

// MARK: Script Injection
extension Tab {
}

// Find In Page interaction
extension Tab {
  func presentFindInteraction(with text: String? = nil) {
    if let findInteraction = webView?.findInteraction {
      findInteraction.searchText = text
      findInteraction.presentFindNavigator(showingReplace: false)
    }
  }
  func dismissFindInteraction() {
    webView?.findInteraction?.dismissFindNavigator()
  }
}

// Back Forward
struct BackForwardList {
  struct Item: Equatable {
    var url: URL
    var title: String?
    fileprivate var item: WKBackForwardListItem?
    static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.item === rhs.item
    }
  }
  var backList: [Item] = []
  var forwardList: [Item] = []
  var currentItem: Item?
  var backItem: Item?
  var forwardItem: Item?
}

extension Tab {
  var backForwardList: BackForwardList {
    guard let webView else { return .init() }
    let list = webView.backForwardList
    return .init(
      backList: list.backList.map { .init(url: $0.url, title: $0.title, item: $0) },
      forwardList: list.forwardList.map { .init(url: $0.url, title: $0.title, item: $0) },
      currentItem: list.currentItem.flatMap { .init(url: $0.url, title: $0.title, item: $0) },
      backItem: list.backItem.flatMap { .init(url: $0.url, title: $0.title, item: $0) },
      forwardItem: list.forwardItem.flatMap { .init(url: $0.url, title: $0.title, item: $0) }
    )
  }
}

// PDF creation
extension Tab {
  enum PDFCreationError: Error {
    case webViewNotRealized
  }
  func createPDF(completionHandler: @escaping (Result<Data, any Error>) -> Void) {
    guard let webView else {
      completionHandler(.failure(PDFCreationError.webViewNotRealized))
      return
    }
    webView.createPDF(completionHandler: completionHandler)
  }
  func dataForDisplayedPDF() -> Data? {
    guard let webView, webView.responds(to: Selector(("_dataForDisplayedPDF"))) else {
      return nil
    }
    return webView.perform(Selector(("_dataForDisplayedPDF"))).takeUnretainedValue() as? Data
  }
}

// Snapshots
extension Tab {
  func takeSnapshot(rect: CGRect = .null, _ completionHandler: @escaping (UIImage?) -> Void) {
    let configuration = WKSnapshotConfiguration()
    configuration.rect = rect
    webView?.takeSnapshot(
      with: configuration,
      completionHandler: { image, error in
        completionHandler(image)
      }
    )
  }
}

// JavaScript injection
extension Tab {
  enum JavaScriptError: Error {
    case invalid
    case webViewNotRealized
  }
  private func generateJSFunctionString(
    functionName: String,
    args: [Any?],
    escapeArgs: Bool = true
  ) -> (javascript: String, error: Error?) {
    var sanitizedArgs = [String]()
    for arg in args {
      if let arg = arg {
        do {
          if let arg = arg as? String {
            sanitizedArgs.append(escapeArgs ? "'\(arg.htmlEntityEncodedString)'" : "\(arg)")
          } else {
            let data = try JSONSerialization.data(withJSONObject: arg, options: [.fragmentsAllowed])

            if let str = String(data: data, encoding: .utf8) {
              sanitizedArgs.append(str)
            } else {
              throw JavaScriptError.invalid
            }
          }
        } catch {
          return ("", error)
        }
      } else {
        sanitizedArgs.append("null")
      }
    }

    if args.count != sanitizedArgs.count {
      assertionFailure("Javascript parsing failed.")
      return ("", JavaScriptError.invalid)
    }

    return ("\(functionName)(\(sanitizedArgs.joined(separator: ", ")))", nil)
  }

  public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true,
    completion: ((Any?, Error?) -> Void)? = nil
  ) {
    guard let webView else {
      completion?(nil, JavaScriptError.webViewNotRealized)
      return
    }
    var javascript = functionName

    if asFunction {
      let js = generateJSFunctionString(
        functionName: functionName,
        args: args,
        escapeArgs: escapeArgs
      )
      if js.error != nil {
        if let completionHandler = completion {
          completionHandler(nil, js.error)
        }
        return
      }
      javascript = js.javascript
    }

    DispatchQueue.main.async {
      webView.evaluateJavaScript(javascript, in: frame, in: contentWorld) { result in
        switch result {
        case .success(let value):
          completion?(value, nil)
        case .failure(let error):
          completion?(nil, error)
        }
      }
    }
  }

  @discardableResult @MainActor public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async -> (Any?, Error?) {
    await withCheckedContinuation { continuation in
      evaluateSafeJavaScript(
        functionName: functionName,
        args: args,
        frame: frame,
        contentWorld: contentWorld,
        escapeArgs: escapeArgs,
        asFunction: asFunction
      ) { value, error in
        continuation.resume(returning: (value, error))
      }
    }
  }

  @discardableResult
  @MainActor public func evaluateSafeJavaScriptThrowing(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async throws -> Any? {
    let result = await evaluateSafeJavaScript(
      functionName: functionName,
      args: args,
      frame: frame,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )

    if let error = result.1 {
      throw error
    } else {
      return result.0
    }
  }

  public func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any] = [:],
    in frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld
  ) async throws -> Any? {
    try await webView?.callAsyncJavaScript(
      functionBody,
      arguments: arguments,
      in: frame,
      contentWorld: contentWorld
    )
  }
}

struct TabDataValues {
  private var storage: [AnyHashable: Any] = [:]

  public subscript<Key: TabDataKey>(key: Key.Type) -> Key.Value? {
    get {
      guard let helper = storage[ObjectIdentifier(key)] as? Key.Value else {
        return Key.defaultValue
      }
      return helper
    }
    set {
      storage[ObjectIdentifier(key)] = newValue
    }
  }
}

protocol TabDataKey {
  associatedtype Value
  static var defaultValue: Value? { get }
}

protocol TabHelper {
  init(tab: Tab)
  static var keyPath: WritableKeyPath<TabDataValues, Self?> { get }
  static func create(for tab: Tab)
  static func remove(from tab: Tab)
  static func from(tab: Tab) -> Self?
}

extension TabHelper {
  static func create(for tab: Tab) {
    if tab.data[keyPath: keyPath] == nil {
      tab.data[keyPath: keyPath] = Self(tab: tab)
    }
  }
  static func remove(from tab: Tab) {
    tab.data[keyPath: keyPath] = nil
  }
  static func from(tab: Tab) -> Self? {
    tab.data[keyPath: keyPath]
  }
}
