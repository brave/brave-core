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

@objc
protocol URLChangeDelegate {
  func tab(_ tab: Tab, urlDidChangeTo url: URL)
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

class Tab: NSObject {
  let id: UUID
  let rewardsId: UInt32

  var onScreenshotUpdated: (() -> Void)?
  var rewardsEnabledCallback: ((Bool) -> Void)?

  var alertShownCount: Int = 0
  var blockAllAlerts: Bool = false

  private(set) var type: TabType = .regular

  var redirectChain = [URL]()
  var responses = [URL: URLResponse]()

  var isPrivate: Bool {
    return type.isPrivate
  }

  private var webViewObservations: [AnyCancellable] = []

  private var observers: Set<AnyTabObserver> = .init()

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

  private let _syncTab: BraveSyncTab?
  private let _faviconDriver: FaviconDriver?
  private var _walletEthProvider: BraveWalletEthereumProvider?
  private var _walletSolProvider: BraveWalletSolanaProvider?
  private var _walletKeyringService: BraveWalletKeyringService? {
    didSet {
      _walletKeyringService?.addObserver(self)
    }
  }

  private weak var syncTab: BraveSyncTab? {
    _syncTab
  }

  weak var faviconDriver: FaviconDriver? {
    _faviconDriver
  }

  weak var walletEthProvider: BraveWalletEthereumProvider? {
    get { _walletEthProvider }
    set { _walletEthProvider = newValue }
  }

  weak var walletSolProvider: BraveWalletSolanaProvider? {
    get { _walletSolProvider }
    set { _walletSolProvider = newValue }
  }

  weak var walletKeyringService: BraveWalletKeyringService? {
    get { _walletKeyringService }
    set { _walletKeyringService = newValue }
  }

  var tabDappStore: TabDappStore = .init()
  var isWalletIconVisible: Bool = false {
    didSet {
      tabDelegate?.updateURLBarWalletButton()
    }
  }

  // PageMetadata is derived from the page content itself, and as such lags behind the
  // rest of the tab.
  var pageMetadata: PageMetadata?

  var canonicalURL: URL? {
    if let string = pageMetadata?.siteURL,
      let siteURL = URL(string: string)
    {
      return siteURL
    }
    return self.url
  }

  /// The URL that should be shared when requested by the user via the share sheet
  ///
  /// If the canonical URL of the page points to a different base domain entirely, this will result in
  /// sharing the canonical URL. This is to ensure pages such as Google's AMP share the correct URL while
  /// also ensuring single page applications which don't update their canonical URLs on navigation share
  /// the current pages URL
  var shareURL: URL? {
    guard let url = url else { return nil }
    if let canonicalURL = canonicalURL, canonicalURL.baseDomain != url.baseDomain {
      return canonicalURL
    }
    return url
  }

  var userActivity: NSUserActivity?

  var webView: TabWebView?
  var tabDelegate: TabDelegate?
  var bars = [SnackBar]()
  var favicon: Favicon
  var lastExecutedTime: Timestamp?
  var sessionData: (title: String, interactionState: Data)?
  fileprivate var lastRequest: URLRequest?
  var restoring: Bool = false
  var pendingScreenshot = false
  var isDisplayingBasicAuthPrompt = false

  // This variable is used to keep track of current page. It is used to detect
  // and report same document navigations to Brave Rewards library.
  var rewardsXHRLoadURL: URL?

  /// This object holds on to information regarding the current web page
  ///
  /// The page data is cleared when the user leaves the page (i.e. when the main frame url changes)
  @MainActor var currentPageData: PageData?

  /// The url set after a successful navigation. This will also set the `url` property.
  ///
  /// - Note: Unlike the `url` property, which may be set during pre-navigation,
  /// the `committedURL` is only assigned when navigation was committed..
  var committedURL: URL? {
    willSet {
      url = newValue
      previousComittedURL = committedURL

      // Clear the current request url and the redirect source url
      // We don't need these values after the request has been comitted
      currentRequestURL = nil
      redirectSourceURL = nil
      isInternalRedirect = false
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

  /// This is the url for the current request
  var currentRequestURL: URL? {
    willSet {
      // Lets push the value as a redirect value
      redirectSourceURL = currentRequestURL
    }
  }

  /// This tells us if we internally redirected while navigating (i.e. debounce or query stripping)
  var isInternalRedirect: Bool = false

  // If the current reqest wasn't comitted and a new one was set
  // we were redirected and therefore this is set
  var redirectSourceURL: URL?

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

      if isDisplayingBasicAuthPrompt {
        url = URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
      }

      // Setting URL in SyncTab is adding pending item to navigation manager on brave-core side
      if let url = url, !isPrivate, !url.isLocal, !InternalURL.isValid(url: url),
        !url.isInternalURL(for: .readermode)
      {
        syncTab?.setURL(url)
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
  var isEditing = false
  var playlistItem: PlaylistInfo?
  var playlistItemState: PlaylistItemAddedState = .none
  var translationState: TranslateURLBarButton.TranslateState = .unavailable

  /// The rewards reporting state which is filled during a page navigation.
  // It is reset to initial values when the page navigation is finished.
  var rewardsReportingState = RewardsTabChangeReportingState()

  /// This is the request that was upgraded to HTTPS
  /// This allows us to rollback the upgrade when we encounter a 4xx+
  var upgradedHTTPSRequest: URLRequest?

  /// This is a timer that's started on HTTPS upgrade
  /// If the upgrade hasn't completed within 3s, it is cancelled
  /// and we fallback to HTTP or cancel the request (strict vs. standard)
  var upgradeHTTPSTimeoutTimer: Timer?

  /// The tabs new tab page controller.
  ///
  /// Should be setup in BVC then assigned here for future use.
  var newTabPageViewController: NewTabPageViewController? {
    willSet {
      if newValue == nil {
        deleteNewTabPageController()
      }
    }
  }

  private func deleteNewTabPageController() {
    guard let controller = newTabPageViewController, controller.parent != nil else { return }
    controller.willMove(toParent: nil)
    controller.removeFromParent()
    controller.view.removeFromSuperview()
  }

  /// When viewing a non-HTML content type in the webview (like a PDF document), this URL will
  /// point to a tempfile containing the content so it can be shared to external applications.
  var temporaryDocument: TemporaryDocument?

  // There is no 'available macro' on props, we currently just need to store ownership.
  lazy var contentBlocker = ContentBlockerHelper(tab: self)
  let requestBlockingContentHelper = RequestBlockingContentScriptHandler()

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

  var readerModeAvailableOrActive: Bool {
    if let readerMode = self.getContentScript(name: ReaderModeScriptHandler.scriptName)
      as? ReaderModeScriptHandler
    {
      return readerMode.state != .unavailable
    }
    return false
  }

  fileprivate(set) var screenshot: UIImage?

  var webStateDebounceTimer: Timer?
  var onPageReadyStateChanged: ((ReadyState.State) -> Void)?

  // If this tab has been opened from another, its parent will point to the tab from which it was opened
  weak var parent: Tab?

  fileprivate let contentScriptManager = TabContentScriptManager()
  private var userScripts = Set<UserScriptManager.ScriptType>()
  private var customUserScripts = Set<UserScriptType>()

  fileprivate var configuration: WKWebViewConfiguration?

  /// Any time a tab tries to make requests to display a Javascript Alert and we are not the active
  /// tab instance, queue it for later until we become foregrounded.
  fileprivate var alertQueue = [JSAlertInfo]()
  weak var shownPromptAlert: UIAlertController?

  var nightMode: Bool {
    didSet {
      var isNightModeEnabled = false

      if let fetchedTabURL = fetchedURL, nightMode,
        !DarkReaderScriptHandler.isNightModeBlockedURL(fetchedTabURL)
      {
        isNightModeEnabled = true
      }

      if let webView = webView {
        if isNightModeEnabled {
          DarkReaderScriptHandler.enable(for: webView)
        } else {
          DarkReaderScriptHandler.disable(for: webView)
        }
      }

      self.setScript(script: .nightMode, enabled: isNightModeEnabled)
    }
  }

  var translateHelper: BraveTranslateTabHelper?
  private(set) lazy var leoTabHelper = BraveLeoScriptTabHelper(tab: self)

  /// Boolean tracking custom url-scheme alert presented
  var isExternalAppAlertPresented = false
  var externalAppPopup: AlertPopupView?
  var externalAppPopupContinuation: CheckedContinuation<Bool, Never>?
  var externalAppAlertCounter = 0
  var isExternalAppAlertSuppressed = false
  var externalAppURLDomain: String?

  func resetExternalAlertProperties() {
    externalAppAlertCounter = 0
    isExternalAppAlertPresented = false
    isExternalAppAlertSuppressed = false
    externalAppURLDomain = nil
  }

  init(
    configuration: WKWebViewConfiguration,
    id: UUID = UUID(),
    type: TabType = .regular,
    tabGeneratorAPI: BraveTabGeneratorAPI? = nil
  ) {
    self.configuration = configuration
    self.id = id
    self.type = type

    self.favicon = Favicon.default
    rewardsId = UInt32.random(in: 1...UInt32.max)
    nightMode = Preferences.General.nightModeEnabled.value
    _syncTab = tabGeneratorAPI?.createBraveSyncTab(isOffTheRecord: type == .private)

    if let syncTab = _syncTab {
      _faviconDriver = FaviconDriver(webState: syncTab.webState).then {
        $0.setMaximumFaviconImageSize(CGSize(width: 1024, height: 1024))
      }
    } else {
      _faviconDriver = nil
    }

    super.init()

    self.contentScriptManager.tab = self
  }

  /// A helper property that handles native to Brave Search communication.
  var braveSearchManager: BraveSearchManager?

  /// A helper property that handles Brave Search Result Ads.
  var braveSearchResultAdManager: BraveSearchResultAdManager?

  private lazy var refreshControl = UIRefreshControl().then {
    $0.addTarget(self, action: #selector(reload), for: .valueChanged)
  }

  func createWebview() {
    if webView == nil {
      assert(configuration != nil, "Create webview can only be called once")
      configuration!.userContentController = WKUserContentController()
      configuration!.preferences = WKPreferences()
      configuration!.preferences.javaScriptCanOpenWindowsAutomatically = false
      configuration!.preferences.isFraudulentWebsiteWarningEnabled =
        Preferences.Shields.googleSafeBrowsing.value
      configuration!.allowsInlineMediaPlayback = true
      // Enables Zoom in website by ignoring their javascript based viewport Scale limits.
      configuration!.ignoresViewportScaleLimits = true
      configuration!.upgradeKnownHostsToHTTPS = ShieldPreferences.httpsUpgradeLevel.isEnabled
      configuration!.enablePageTopColorSampling()

      if configuration!.urlSchemeHandler(forURLScheme: InternalURL.scheme) == nil {
        configuration!.setURLSchemeHandler(
          InternalSchemeHandler(tab: self),
          forURLScheme: InternalURL.scheme
        )
      }
      let webView = TabWebView(
        frame: .zero,
        configuration: configuration!,
        isPrivate: isPrivate
      )
      webView.didSelectSearchWithBrave = { [weak self] selectedText in
        guard let self else { return }
        observers.forEach {
          $0.tab(self, didSelectSearchWithBraveFor: selectedText)
        }
      }

      webView.accessibilityLabel = Strings.webContentAccessibilityLabel
      webView.allowsBackForwardNavigationGestures = true
      webView.allowsLinkPreview = true

      // Turning off masking allows the web content to flow outside of the scrollView's frame
      // which allows the content appear beneath the toolbars in the BrowserViewController
      webView.scrollView.layer.masksToBounds = false

      restore(webView, restorationData: self.sessionData)

      self.webView = webView
      attachWebObservers()

      observers.forEach {
        $0.tab(self, didCreateWebView: webView)
      }

      let scriptPreferences: [UserScriptManager.ScriptType: Bool] = [
        .cookieBlocking: Preferences.Privacy.blockAllCookies.value,
        .mediaBackgroundPlay: Preferences.General.mediaAutoBackgrounding.value,
        .nightMode: Preferences.General.nightModeEnabled.value,
        .braveTranslate: Preferences.Translate.translateEnabled.value != false,
      ]

      userScripts = Set(scriptPreferences.filter({ $0.value }).map({ $0.key }))
      self.updateInjectedScripts()
      nightMode = Preferences.General.nightModeEnabled.value
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
      if isTabVisible(), url?.displayURL != nil, url?.displayURL?.scheme == "about", !loading {
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
    configuration = config
    deleteWebView()
    contentScriptManager.helpers.removeAll()
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

  func restore(_ webView: WKWebView, restorationData: (title: String, interactionState: Data)?) {
    // Pulls restored session data from a previous SavedTab to load into the Tab. If it's nil, a session restore
    // has already been triggered via custom URL, so we use the last request to trigger it again; otherwise,
    // we extract the information needed to restore the tabs and create a NSURLRequest with the custom session restore URL
    // to trigger the session restore via custom handlers
    if let sessionInfo = restorationData {
      restoring = true
      lastTitle = sessionInfo.title
      webView.interactionState = sessionInfo.interactionState
      restoring = false
      rewardsReportingState.wasRestored = true
      self.sessionData = nil
    } else if let request = lastRequest {
      webView.load(request)
    } else {
      Logger.module.warning(
        "creating webview with no lastRequest and no session data: \(self.url?.absoluteString ?? "nil")"
      )
    }
  }

  func restore(_ webView: WKWebView, requestRestorationData: (title: String, request: URLRequest)?)
  {
    if let sessionInfo = requestRestorationData {
      restoring = true
      lastTitle = sessionInfo.title
      webView.load(sessionInfo.request)
      restoring = false
      self.sessionData = nil
    } else if let request = lastRequest {
      webView.load(request)
    } else {
      Logger.module.warning(
        "creating webview with no lastRequest and no session data: \(self.url?.absoluteString ?? "nil")"
      )
    }
  }

  func deleteWebView() {
    contentScriptManager.uninstall(from: self)
    translateHelper = nil

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
    deleteNewTabPageController()
    contentScriptManager.helpers.removeAll()

    // A number of mojo-powered core objects have to be deconstructed on the same
    // thread they were constructed
    var mojoObjects: [Any?] = [
      _faviconDriver,
      _syncTab,
      _walletEthProvider,
      _walletSolProvider,
      _walletKeyringService,
    ]

    DispatchQueue.main.async {
      // Reference inside to retain it, supress warnings by reading/writing
      _ = mojoObjects
      mojoObjects = []
    }
  }

  var loading: Bool {
    return webView?.isLoading ?? false
  }

  var estimatedProgress: Double {
    return webView?.estimatedProgress ?? 0
  }

  var backList: [WKBackForwardListItem]? {
    return webView?.backForwardList.backList
  }

  var forwardList: [WKBackForwardListItem]? {
    return webView?.backForwardList.forwardList
  }

  var historyList: [URL] {
    func listToUrl(_ item: WKBackForwardListItem) -> URL { return item.url }
    var tabs = self.backList?.map(listToUrl) ?? [URL]()
    tabs.append(self.url!)
    return tabs
  }

  var title: String? {
    return webView?.title
  }

  var displayTitle: String {
    if let displayTabTitle = fetchDisplayTitle(using: url, title: title) {
      syncTab?.setTitle(displayTabTitle)
      return displayTabTitle
    }

    // When picking a display title. Tabs with sessionData are pending a restore so show their old title.
    // To prevent flickering of the display title. If a tab is restoring make sure to use its lastTitle.
    if let url = self.url, InternalURL(url)?.isAboutHomeURL ?? false, sessionData == nil, !restoring
    {
      syncTab?.setTitle(Strings.Hotkey.newTabTitle)
      return Strings.Hotkey.newTabTitle
    }

    if let url = self.url, !InternalURL.isValid(url: url),
      let shownUrl = url.displayURL?.absoluteString, webView != nil
    {
      syncTab?.setTitle(shownUrl)
      return shownUrl
    }

    guard let lastTitle = lastTitle, !lastTitle.isEmpty else {
      // FF uses url?.displayURL?.absoluteString ??  ""
      if let title = url?.absoluteString {
        syncTab?.setTitle(title)
        return title
      } else if let tab = SessionTab.from(tabId: id) {
        if tab.title.isEmpty {
          return Strings.Hotkey.newTabTitle
        }
        syncTab?.setTitle(tab.title)
        return tab.title
      }

      syncTab?.setTitle("")
      return ""
    }

    syncTab?.setTitle(lastTitle)
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

  /// A list of domains that we want to proceed to anyways regardless of any ad-blocking
  var proceedAnywaysDomainList: Set<String> = []

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

      syncTab?.setTitle(displayTitle)
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

  func goToBackForwardListItem(_ item: WKBackForwardListItem) {
    _ = webView?.go(to: item)
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

    resetExternalAlertProperties()

    // If the current page is an error page, and the reload button is tapped, load the original URL
    if let url = webView?.url, let internalUrl = InternalURL(url),
      let page = internalUrl.originalURLFromErrorPage
    {
      webView?.replaceLocation(with: page)
      return
    }

    if let _ = webView?.reloadFromOrigin() {
      nightMode = Preferences.General.nightModeEnabled.value
      Logger.module.debug("reloaded zombified tab from origin")
      return
    }

    if let webView = self.webView {
      Logger.module.debug("restoring webView from scratch")
      restore(webView, restorationData: sessionData)
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

  func addContentScript(_ helper: TabContentScript, name: String, contentWorld: WKContentWorld) {
    contentScriptManager.addContentScript(
      helper,
      name: name,
      forTab: self,
      contentWorld: contentWorld
    )
  }

  func removeContentScript(name: String, forTab tab: Tab, contentWorld: WKContentWorld) {
    contentScriptManager.removeContentScript(name: name, forTab: tab, contentWorld: contentWorld)
  }

  func replaceContentScript(_ helper: TabContentScript, name: String, forTab tab: Tab) {
    contentScriptManager.replaceContentScript(helper, name: name, forTab: tab)
  }

  func getContentScript(name: String) -> TabContentScript? {
    return contentScriptManager.getContentScript(name)
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

  func addSnackbar(_ bar: SnackBar) {
    bars.append(bar)
    observers.forEach {
      $0.tab(self, didAddSnackbar: bar)
    }
  }

  func removeSnackbar(_ bar: SnackBar) {
    if let index = bars.firstIndex(of: bar) {
      bars.remove(at: index)
      observers.forEach {
        $0.tab(self, didRemoveSnackbar: bar)
      }
    }
  }

  func removeAllSnackbars() {
    // Enumerate backwards here because we'll remove items from the list as we go.
    bars.reversed().forEach { removeSnackbar($0) }
  }

  func expireSnackbars() {
    // Enumerate backwards here because we may remove items from the list as we go.
    bars.reversed().filter({ !$0.shouldPersist(self) }).forEach({ removeSnackbar($0) })
  }

  func setScreenshot(_ screenshot: UIImage?) {
    self.screenshot = screenshot
    onScreenshotUpdated?()
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

  func queueJavascriptAlertPrompt(_ alert: JSAlertInfo) {
    alertQueue.append(alert)
  }

  func dequeueJavascriptAlertPrompt() -> JSAlertInfo? {
    guard !alertQueue.isEmpty else {
      return nil
    }
    return alertQueue.removeFirst()
  }

  func cancelQueuedAlerts() {
    alertQueue.forEach { alert in
      alert.cancel()
    }
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

  func addTabInfoToSyncedSessions(url: URL, displayTitle: String) {
    syncTab?.setURL(url)
    syncTab?.setTitle(displayTitle)
  }
}

private class TabContentScriptManager: NSObject, WKScriptMessageHandlerWithReply {
  fileprivate var helpers = [String: TabContentScript]()
  weak var tab: Tab?

  func uninstall(from tab: Tab) {
    helpers.forEach {
      let name = type(of: $0.value).messageHandlerName
      tab.webView?.configuration.userContentController.removeScriptMessageHandler(forName: name)
    }
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let tab,
      let helper = helpers.values.first(where: {
        type(of: $0).messageHandlerName == message.name
      })
    else {
      replyHandler(nil, nil)
      return
    }
    helper.tab(tab, receivedScriptMessage: message, replyHandler: replyHandler)
  }

  func addContentScript(
    _ helper: TabContentScript,
    name: String,
    forTab tab: Tab,
    contentWorld: WKContentWorld
  ) {
    if let _ = helpers[name] {
      assertionFailure("Duplicate helper added: \(name)")
    }

    helpers[name] = helper

    // If this helper handles script messages, then get the handler name and register it. The Tab
    // receives all messages and then dispatches them to the right TabHelper.
    let scriptMessageHandlerName = type(of: helper).messageHandlerName
    tab.webView?.configuration.userContentController.addScriptMessageHandler(
      self,
      contentWorld: contentWorld,
      name: scriptMessageHandlerName
    )
  }

  func removeContentScript(name: String, forTab tab: Tab, contentWorld: WKContentWorld) {
    if let helper = helpers[name] {
      let scriptMessageHandlerName = type(of: helper).messageHandlerName
      tab.webView?.configuration.userContentController.removeScriptMessageHandler(
        forName: scriptMessageHandlerName,
        contentWorld: contentWorld
      )
      helpers[name] = nil
    }
  }

  func replaceContentScript(_ helper: TabContentScript, name: String, forTab tab: Tab) {
    if helpers[name] != nil {
      helpers[name] = helper
    }
  }

  func getContentScript(_ name: String) -> TabContentScript? {
    return helpers[name]
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

  fileprivate var didSelectSearchWithBrave: ((String) -> Void)?

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

  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    if action == MenuHelper.selectorForcePaste {
      // If paste is allowed, show force paste as well
      return super.canPerformAction(#selector(paste(_:)), withSender: sender)
    }
    return super.canPerformAction(action, withSender: sender)
  }

  @objc func menuHelperForcePaste() {
    if let string = UIPasteboard.general.string {
      evaluateSafeJavaScript(
        functionName: "window.__firefox__.forcePaste",
        args: [string, UserScriptManager.securityToken],
        contentWorld: .defaultClient
      ) { _, _ in }
    }
  }

  @objc func menuHelperSearchWithBrave() {
    getCurrentSelectedText { [weak self] selectedText in
      guard let self = self else { return }
      guard let selectedText = selectedText else {
        assertionFailure("Impossible to trigger this without selected text")
        return
      }

      self.didSelectSearchWithBrave?(selectedText)
    }
  }

  // rdar://33283179 Apple bug where `serverTrust` is not defined as KVO when it should be
  override func value(forUndefinedKey key: String) -> Any? {
    if key == #keyPath(WKWebView.serverTrust) {
      return serverTrust
    }

    return super.value(forUndefinedKey: key)
  }

  private func getCurrentSelectedText(callback: @escaping (String?) -> Void) {
    evaluateSafeJavaScript(functionName: "getSelection().toString", contentWorld: .defaultClient) {
      result,
      _ in
      let selectedText = result as? String
      callback(selectedText)
    }
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

        if let url = self.webView?.url,
          BraveSearchManager.isValidURL(url),
          let result = self.braveSearchManager?.fallbackQueryResult
        {
          queryResult = result
        }

        self.webView?.evaluateSafeJavaScript(
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
    self.webView?.evaluateSafeJavaScript(
      functionName: "localStorage.setItem",
      args: [key, value],
      contentWorld: BraveSkusScriptHandler.scriptSandbox
    )
  }
}

// MARK: Script Injection
extension Tab {
  func setScript(script: UserScriptManager.ScriptType, enabled: Bool) {
    setScripts(scripts: [script: enabled])
  }

  func setScripts(scripts: Set<UserScriptManager.ScriptType>, enabled: Bool) {
    var scriptMap = [UserScriptManager.ScriptType: Bool]()
    scripts.forEach({ scriptMap[$0] = enabled })
    setScripts(scripts: scriptMap)
  }

  func setScripts(scripts: [UserScriptManager.ScriptType: Bool]) {
    var scriptsToAdd = Set<UserScriptManager.ScriptType>()
    var scriptsToRemove = Set<UserScriptManager.ScriptType>()

    for (script, enabled) in scripts {
      let scriptExists = userScripts.contains(script)

      if !scriptExists && enabled {
        scriptsToAdd.insert(script)
      } else if scriptExists && !enabled {
        scriptsToRemove.insert(script)
      }
    }

    if scriptsToAdd.isEmpty && scriptsToRemove.isEmpty {
      // Scripts already enabled or disabled
      return
    }

    userScripts.formUnion(scriptsToAdd)
    userScripts.subtract(scriptsToRemove)
    updateInjectedScripts()
  }

  func setCustomUserScript(scripts: Set<UserScriptType>) {
    if customUserScripts != scripts {
      customUserScripts = scripts
      updateInjectedScripts()
    }
  }

  private func updateInjectedScripts() {
    UserScriptManager.shared.loadCustomScripts(
      into: self,
      userScripts: userScripts,
      customScripts: customUserScripts
    )
  }
}
