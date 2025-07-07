// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import BraveWallet
import Data
import Foundation
import Preferences
import Shared
import Storage
import Web

extension TabDataValues {
  private struct TabBrowserDataKey: TabDataKey {
    static var defaultValue: TabBrowserData? { nil }
  }

  var browserData: TabBrowserData? {
    get { self[TabBrowserDataKey.self] }
    set { self[TabBrowserDataKey.self] = newValue }
  }
}

protocol TabMiscDelegate {
  func showRequestRewardsPanel(_ tab: some TabState)
  func stopMediaPlayback(_ tab: some TabState)
  func showWalletNotification(_ tab: some TabState, origin: URLOrigin)
  func updateURLBarWalletButton()
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

/// A broad container of assorted data that was previously stored in Tab
///
/// DO NOT ADD NEW PROPERTIES TO THIS TYPE
///
/// This is a temporary container to shift non-web navigation related properties out of Tab,
/// any additional properties or changes to data in this should be pulled out and placed in its own
/// type such as a tab helper.
class TabBrowserData: NSObject, TabObserver {
  weak var tab: (any TabState)?

  init(
    tab: some TabState,
    tabGeneratorAPI: BraveTabGeneratorAPI? = nil
  ) {
    self.tab = tab
    _syncTab = tabGeneratorAPI?.createBraveSyncTab(isOffTheRecord: tab.isPrivate)

    if let syncTab = _syncTab {
      _faviconDriver = FaviconDriver(webState: syncTab.webState).then {
        $0.setMaximumFaviconImageSize(CGSize(width: 1024, height: 1024))
      }
    } else {
      _faviconDriver = nil
    }

    nightMode = Preferences.General.nightModeEnabled.value

    super.init()

    tab.addObserver(self)
    contentScriptManager.tab = tab
  }

  deinit {
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

  let rewardsId: UInt32 = .random(in: 1...UInt32.max)

  var pendingScreenshot = false
  fileprivate(set) var screenshot: UIImage?
  func setScreenshot(_ screenshot: UIImage?) {
    self.screenshot = screenshot
    onScreenshotUpdated?()
  }
  var onScreenshotUpdated: (() -> Void)?
  var rewardsEnabledCallback: ((Bool) -> Void)?

  var alertShownCount: Int = 0
  var blockAllAlerts: Bool = false

  var responses = [URL: URLResponse]()
  var miscDelegate: TabMiscDelegate?

  private var _syncTab: BraveSyncTab?
  private var _faviconDriver: FaviconDriver?
  private var _walletEthProvider: BraveWalletEthereumProvider?
  private var _walletSolProvider: BraveWalletSolanaProvider?
  private var _walletKeyringService: BraveWalletKeyringService? {
    didSet {
      _walletKeyringService?.addObserver(self)
    }
  }

  weak var syncTab: BraveSyncTab? {
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
      tab?.miscDelegate?.updateURLBarWalletButton()
    }
  }

  var lastTitle: String?

  var isDisplayingBasicAuthPrompt = false

  // This variable is used to keep track of current page. It is used to detect
  // and report same document navigations to Brave Rewards library.
  var rewardsXHRLoadURL: URL?

  /// This object holds on to information regarding the current web page
  ///
  /// The page data is cleared when the user leaves the page (i.e. when the main frame url changes)
  @MainActor var currentPageData: PageData?

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

  // There is no 'available macro' on props, we currently just need to store ownership.
  lazy var contentBlocker = ContentBlockerHelper(tab: tab)
  let requestBlockingContentHelper = RequestBlockingContentScriptHandler()

  var readerModeAvailableOrActive: Bool {
    if let readerMode = getContentScript(name: ReaderModeScriptHandler.scriptName)
      as? ReaderModeScriptHandler
    {
      return readerMode.state != .unavailable
    }
    return false
  }

  var webStateDebounceTimer: Timer?
  var onPageReadyStateChanged: ((ReadyState.State) -> Void)?

  fileprivate let contentScriptManager = TabContentScriptManager()
  private var userScripts = Set<UserScriptManager.ScriptType>()
  private var customUserScripts = Set<UserScriptType>()

  /// Any time a tab tries to make requests to display a Javascript Alert and we are not the active
  /// tab instance, queue it for later until we become foregrounded.
  fileprivate var alertQueue = [JSAlertInfo]()
  weak var shownPromptAlert: UIAlertController?

  var nightMode: Bool {
    didSet {
      var isNightModeEnabled = false

      if let fetchedTabURL = tab?.fetchedURL, nightMode,
        !DarkReaderScriptHandler.isNightModeBlockedURL(fetchedTabURL)
      {
        isNightModeEnabled = true
      }

      if let tab = tab {
        if isNightModeEnabled {
          DarkReaderScriptHandler.enable(for: tab)
        } else {
          DarkReaderScriptHandler.disable(for: tab)
        }
      }

      self.setScript(script: .nightMode, enabled: isNightModeEnabled)
    }
  }

  var translateHelper: BraveTranslateTabHelper?
  private(set) lazy var leoTabHelper = BraveLeoScriptTabHelper(tab: tab)

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

  /// A helper property that handles native to Brave Search communication.
  var braveSearchManager: BraveSearchManager?

  /// A helper property that handles Brave Search Result Ads.
  var braveSearchResultAdManager: BraveSearchResultAdManager?

  /// A list of domains that we want to proceed to anyways regardless of any ad-blocking
  var proceedAnywaysDomainList: Set<String> = []

  /// When viewing a non-HTML content type in the webview (like a PDF document), this URL will
  /// point to a tempfile containing the content so it can be shared to external applications.
  var temporaryDocument: TemporaryDocument?

  func addContentScript(_ helper: TabContentScript, name: String, contentWorld: WKContentWorld) {
    guard let tab else { return }
    contentScriptManager.addContentScript(
      helper,
      name: name,
      forTab: tab,
      contentWorld: contentWorld
    )
  }

  func removeContentScript(name: String, forTab tab: some TabState, contentWorld: WKContentWorld) {
    contentScriptManager.removeContentScript(name: name, forTab: tab, contentWorld: contentWorld)
  }

  func replaceContentScript(_ helper: TabContentScript, name: String, forTab tab: some TabState) {
    contentScriptManager.replaceContentScript(helper, name: name, forTab: tab)
  }

  func getContentScript(name: String) -> TabContentScript? {
    return contentScriptManager.getContentScript(name)
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

  func addTabInfoToSyncedSessions(url: URL, displayTitle: String) {
    syncTab?.setURL(url)
    syncTab?.setTitle(displayTitle)
  }

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
    guard let tab else { return }
    UserScriptManager.shared.loadCustomScripts(
      into: tab,
      userScripts: userScripts,
      customScripts: customUserScripts
    )
  }

  // MARK: - TabObserver

  func tabDidStartNavigation(_ tab: some TabState) {
    resetExternalAlertProperties()
    nightMode = Preferences.General.nightModeEnabled.value
  }

  func tabDidChangeTitle(_ tab: some TabState) {
    syncTab?.setTitle(tab.displayTitle)
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    if let url = tab.visibleURL, !tab.isPrivate, !url.isLocal, !InternalURL.isValid(url: url),
      !url.isInternalURL(for: .readermode)
    {
      syncTab?.setURL(url)
    }
  }

  func tabDidCreateWebView(_ tab: some TabState) {
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

  func tabWillDeleteWebView(_ tab: some TabState) {
    contentScriptManager.helpers.removeAll()
    contentScriptManager.uninstall(from: tab)
    translateHelper = nil
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

private class TabContentScriptManager: NSObject, WKScriptMessageHandlerWithReply {
  fileprivate var helpers = [String: TabContentScript]()
  weak var tab: (any TabState)?

  func uninstall(from tab: some TabState) {
    helpers.forEach {
      let name = type(of: $0.value).messageHandlerName
      tab.configuration.userContentController.removeScriptMessageHandler(forName: name)
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
    forTab tab: some TabState,
    contentWorld: WKContentWorld
  ) {
    if let _ = helpers[name] {
      assertionFailure("Duplicate helper added: \(name)")
    }

    helpers[name] = helper

    // If this helper handles script messages, then get the handler name and register it. The Tab
    // receives all messages and then dispatches them to the right TabHelper.
    let scriptMessageHandlerName = type(of: helper).messageHandlerName
    tab.configuration.userContentController.addScriptMessageHandler(
      self,
      contentWorld: contentWorld,
      name: scriptMessageHandlerName
    )
  }

  func removeContentScript(name: String, forTab tab: some TabState, contentWorld: WKContentWorld) {
    if let helper = helpers[name] {
      let scriptMessageHandlerName = type(of: helper).messageHandlerName
      tab.configuration.userContentController.removeScriptMessageHandler(
        forName: scriptMessageHandlerName,
        contentWorld: contentWorld
      )
      helpers[name] = nil
    }
  }

  func replaceContentScript(_ helper: TabContentScript, name: String, forTab tab: some TabState) {
    if helpers[name] != nil {
      helpers[name] = helper
    }
  }

  func getContentScript(_ name: String) -> TabContentScript? {
    return helpers[name]
  }
}

/// Allows fetching directly from TabBrowserData directly from Tab using dynamic member lookup
///
/// DO NOT REPLICATE FOR OTHER TYPES
///
/// Tab will retain dynamic member lookup only one layer deep into TabDataValues, this is only
/// here to avoid more significant refactors
extension TabState {
  subscript<Value>(dynamicMember member: KeyPath<TabBrowserData, Value>) -> Value? {
    return data.browserData?[keyPath: member]
  }
  subscript<Value>(dynamicMember member: KeyPath<TabBrowserData, Value?>) -> Value? {
    return data.browserData?[keyPath: member]
  }
  subscript<Value>(dynamicMember member: WritableKeyPath<TabBrowserData, Value>) -> Value? {
    get {
      return data.browserData?[keyPath: member]
    }
    set {
      if let newValue {
        data.browserData?[keyPath: member] = newValue
      }
    }
  }
  subscript<Value>(dynamicMember member: WritableKeyPath<TabBrowserData, Value?>) -> Value? {
    get {
      return data.browserData?[keyPath: member]
    }
    set {
      data.browserData?[keyPath: member] = newValue
    }
  }
}
