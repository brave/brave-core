/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Storage
import Shared
import BraveRewards
import BraveShared
import SwiftyJSON
import XCGLogger
import Data

private let log = Logger.browserLogger

protocol TabContentScript {
    static func name() -> String
    func scriptMessageHandlerName() -> String?
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage)
}

@objc
protocol TabDelegate {
    func tab(_ tab: Tab, didAddSnackbar bar: SnackBar)
    func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar)
    func tab(_ tab: Tab, didSelectFindInPageForSelection selection: String)
    @objc optional func tab(_ tab: Tab, didCreateWebView webView: WKWebView)
    @objc optional func tab(_ tab: Tab, willDeleteWebView webView: WKWebView)
}

@objc
protocol URLChangeDelegate {
    func tab(_ tab: Tab, urlDidChangeTo url: URL)
}

enum TabSecureContentState {
    case localHost
    case secure
    case insecure
    case unknown
}

class Tab: NSObject {
    var id: String?
    
    let rewardsId: UInt32
    
    var alertShownCount: Int = 0
    var blockAllAlerts: Bool = false
    private(set) var type: TabType = .regular
    
    var isPrivate: Bool {
        return type.isPrivate
    }

    var secureContentState: TabSecureContentState = .unknown

    // PageMetadata is derived from the page content itself, and as such lags behind the
    // rest of the tab.
    var pageMetadata: PageMetadata?

    var canonicalURL: URL? {
        if let string = pageMetadata?.siteURL,
            let siteURL = URL(string: string) {
            return siteURL
        }
        return self.url
    }

    var userActivity: NSUserActivity?

    var webView: BraveWebView?
    var tabDelegate: TabDelegate?
    weak var urlDidChangeDelegate: URLChangeDelegate?     // TODO: generalize this.
    var bars = [SnackBar]()
    var favicons = [Favicon]()
    var lastExecutedTime: Timestamp?
    var sessionData: SessionData?
    fileprivate var lastRequest: URLRequest?
    var restoring: Bool = false
    var pendingScreenshot = false
    var url: URL?
    var mimeType: String?
    var isEditing: Bool = false
    var shouldClassifyLoadsForAds = true
    
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
 
    // When viewing a non-HTML content type in the webview (like a PDF document), this URL will
    // point to a tempfile containing the content so it can be shared to external applications.
    var temporaryDocument: TemporaryDocument?

    fileprivate var _noImageMode = false

    /// Returns true if this tab's URL is known, and it's longer than we want to store.
    var urlIsTooLong: Bool {
        guard let url = self.url else {
            return false
        }
        return url.absoluteString.lengthOfBytes(using: .utf8) > AppConstants.DB_URL_LENGTH_MAX
    }

    // Use computed property so @available can be used to guard `noImageMode`.
    var noImageMode: Bool {
        get { return _noImageMode }
        set {
            if newValue == _noImageMode {
                return
            }
            _noImageMode = newValue
            contentBlocker.noImageMode(enabled: _noImageMode)
        }
    }

    // There is no 'available macro' on props, we currently just need to store ownership.
    lazy var contentBlocker = ContentBlockerHelper(tab: self)

    /// The last title shown by this tab. Used by the tab tray to show titles for zombie tabs.
    var lastTitle: String?

    var isDesktopSite: Bool {
        webView?.customUserAgent?.lowercased().contains("mobile") == false
    }
    
    /// In-memory dictionary of websites that were explicitly set to use either desktop or mobile user agent.
    /// Key is url's base domain, value is desktop mode on or off.
    /// Each tab has separate list of website overrides.
    private var userAgentOverrides: [String: Bool] = [:]
    
    var readerModeAvailableOrActive: Bool {
        if let readerMode = self.getContentScript(name: "ReaderMode") as? ReaderMode {
            return readerMode.state != .unavailable
        }
        return false
    }

    fileprivate(set) var screenshot: UIImage?
    var screenshotUUID: UUID? {
        didSet { TabMO.saveScreenshotUUID(screenshotUUID, tabId: id) }
    }

    // If this tab has been opened from another, its parent will point to the tab from which it was opened
    var parent: Tab?

    fileprivate var contentScriptManager = TabContentScriptManager()
    private(set) var userScriptManager: UserScriptManager?

    fileprivate var configuration: WKWebViewConfiguration?

    /// Any time a tab tries to make requests to display a Javascript Alert and we are not the active
    /// tab instance, queue it for later until we become foregrounded.
    fileprivate var alertQueue = [JSAlertInfo]()

    init(configuration: WKWebViewConfiguration, type: TabType = .regular) {
        self.configuration = configuration
        rewardsId = UInt32.random(in: 1...UInt32.max)
        super.init()
        self.type = type
    }

    class func toTab(_ tab: Tab) -> RemoteTab? {
        if let displayURL = tab.url?.displayURL, RemoteTab.shouldIncludeURL(displayURL) {
            let history = Array(tab.historyList.filter(RemoteTab.shouldIncludeURL).reversed())
            return RemoteTab(clientGUID: nil,
                URL: displayURL,
                title: tab.displayTitle,
                history: history,
                lastUsed: Date.now(),
                icon: nil)
        } else if let sessionData = tab.sessionData, !sessionData.urls.isEmpty {
            let history = Array(sessionData.urls.filter(RemoteTab.shouldIncludeURL).reversed())
            if let displayURL = history.first {
                return RemoteTab(clientGUID: nil,
                    URL: displayURL,
                    title: tab.displayTitle,
                    history: history,
                    lastUsed: sessionData.lastUsedTime,
                    icon: nil)
            }
        }

        return nil
    }

    weak var navigationDelegate: WKNavigationDelegate? {
        didSet {
            if let webView = webView {
                webView.navigationDelegate = navigationDelegate
            }
        }
    }

    func createWebview() {
        if webView == nil {
            assert(configuration != nil, "Create webview can only be called once")
            configuration!.userContentController = WKUserContentController()
            configuration!.preferences = WKPreferences()
            configuration!.preferences.javaScriptCanOpenWindowsAutomatically = false
            configuration!.allowsInlineMediaPlayback = true
            // Enables Zoom in website by ignoring their javascript based viewport Scale limits.
            configuration!.ignoresViewportScaleLimits = true
            let webView = TabWebView(frame: .zero, configuration: configuration!, isPrivate: isPrivate)
            webView.delegate = self
            configuration = nil

            webView.accessibilityLabel = Strings.webContentAccessibilityLabel
            webView.allowsBackForwardNavigationGestures = true
            if #available(iOS 13, *) {
                webView.allowsLinkPreview = true
            } else {
                webView.allowsLinkPreview = false
            }

            // Turning off masking allows the web content to flow outside of the scrollView's frame
            // which allows the content appear beneath the toolbars in the BrowserViewController
            webView.scrollView.layer.masksToBounds = false
            webView.navigationDelegate = navigationDelegate

            restore(webView, restorationData: self.sessionData?.savedTabData)

            self.webView = webView
            self.webView?.addObserver(self, forKeyPath: KVOConstants.URL.rawValue, options: .new, context: nil)
            self.userScriptManager = UserScriptManager(
                tab: self,
                isFingerprintingProtectionEnabled: Preferences.Shields.fingerprintingProtection.value,
                isCookieBlockingEnabled: Preferences.Privacy.blockAllCookies.value,
                isU2FEnabled: webView.hasOnlySecureContent,
                isPaymentRequestEnabled: webView.hasOnlySecureContent,
                isYoutubeAdblockEnabled: false)
            tabDelegate?.tab?(self, didCreateWebView: webView)
        }
    }
    
    func resetWebView(config: WKWebViewConfiguration) {
        configuration = config
        deleteWebView()
        contentScriptManager.helpers.removeAll()
    }
    
    func restore(_ webView: WKWebView, restorationData: SavedTab?) {
        // Pulls restored session data from a previous SavedTab to load into the Tab. If it's nil, a session restore
        // has already been triggered via custom URL, so we use the last request to trigger it again; otherwise,
        // we extract the information needed to restore the tabs and create a NSURLRequest with the custom session restore URL
        // to trigger the session restore via custom handlers
        if let sessionData = restorationData {
            lastTitle = sessionData.title
            var updatedURLs = [String]()
            var previous = ""
            for urlString in sessionData.history {
                guard let url = URL(string: urlString) else { continue }
                let updatedURL = WebServer.sharedInstance.updateLocalURL(url)!.absoluteString
                guard let current = try? updatedURL.regexReplacePattern("https?:..", with: "") else { continue }
                if current.count > 1 && current == previous {
                    updatedURLs.removeLast()
                }
                previous = current
                updatedURLs.append(updatedURL)
            }
            let currentPage = sessionData.historyIndex
            self.sessionData = nil
            var jsonDict = [String: AnyObject]()
            jsonDict[SessionData.Keys.history] = updatedURLs as AnyObject
            jsonDict[SessionData.Keys.currentPage] = Int(currentPage) as AnyObject
            
            guard let escapedJSON = JSON(jsonDict).rawString()?.addingPercentEncoding(withAllowedCharacters: CharacterSet.urlQueryAllowed) else {
                return
            }
            
            let restoreURL = URL(string: "\(WebServer.sharedInstance.base)/about/sessionrestore?history=\(escapedJSON)")
            lastRequest = PrivilegedRequest(url: restoreURL!) as URLRequest
            webView.load(lastRequest!)
        } else if let request = lastRequest {
            webView.load(request)
        } else {
            log.warning("creating webview with no lastRequest and no session data: \(String(describing: self.url))")
        }
        
    }
    
    func deleteWebView() {
        contentScriptManager.uninstall(from: self)
        
        if let webView = webView {
            webView.removeObserver(self, forKeyPath: KVOConstants.URL.rawValue)
            tabDelegate?.tab?(self, willDeleteWebView: webView)
        }
        webView = nil
    }

    deinit {
        deleteWebView()
        deleteNewTabPageController()
        contentScriptManager.helpers.removeAll()
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate else { return }
        
        let rewards = appDelegate.browserViewController.rewards
        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            rewards.reportTabClosed(tabId: rewardsId)
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
        if let title = webView?.title, !title.isEmpty {
            return title.contains("localhost") ? "" : title
        } else if let url = webView?.url ?? self.url, url.isAboutHomeURL {
            return Strings.newTabTitle
        }
        
        guard let lastTitle = lastTitle, !lastTitle.isEmpty else {
            if let title = url?.absoluteString {
                return title
            } else if let tab = TabMO.get(fromId: id) {
                return tab.title ?? tab.url ?? ""
            }
            return ""
        }
        
        return lastTitle
    }

    var currentInitialURL: URL? {
        return self.webView?.backForwardList.currentItem?.initialURL
    }

    var displayFavicon: Favicon? {
        if url?.isAboutHomeURL == true { return nil }
        return favicons.max { $0.width! < $1.width! }
    }

    var canGoBack: Bool {
        return webView?.canGoBack ?? false
    }

    var canGoForward: Bool {
        return webView?.canGoForward ?? false
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
            if let url = request.url, url.isFileURL, request.isPrivileged {
                return webView.loadFileURL(url, allowingReadAccessTo: url)
            }

            return webView.load(request)
        }
        return nil
    }

    func stop() {
        webView?.stopLoading()
    }

    func reload() {
        // Clear the user agent before further navigation.
        // Proper User Agent setting happens in BVC's WKNavigationDelegate.
        // This prevents a bug with back-forward list, going back or forward and reloading the tab
        // loaded wrong user agent.
        webView?.customUserAgent = nil
        
        // Refreshing error, safe browsing warning pages.
        if let originalUrlFromErrorUrl = webView?.url?.originalURLFromErrorURL {
            webView?.load(URLRequest(url: originalUrlFromErrorUrl))
            return
        }

        if let _ = webView?.reloadFromOrigin() {
            log.debug("reloaded zombified tab from origin")
            return
        }

        if let webView = self.webView {
            log.debug("restoring webView from scratch")
            restore(webView, restorationData: sessionData?.savedTabData)
        }
    }
    
    func updateUserAgent(_ webView: WKWebView, newURL: URL) {
        guard let baseDomain = newURL.baseDomain else { return }
        
        let desktopMode = userAgentOverrides[baseDomain] ?? UserAgent.shouldUseDesktopMode
        webView.customUserAgent = desktopMode ? UserAgent.desktop : UserAgent.mobile
    }

    func addContentScript(_ helper: TabContentScript, name: String) {
        contentScriptManager.addContentScript(helper, name: name, forTab: self)
    }

    func getContentScript(name: String) -> TabContentScript? {
        return contentScriptManager.getContentScript(name)
    }

    func hideContent(_ animated: Bool = false) {
        webView?.isUserInteractionEnabled = false
        if animated {
            UIView.animate(withDuration: 0.25, animations: { () -> Void in
                self.webView?.alpha = 0.0
            })
        } else {
            webView?.alpha = 0.0
        }
    }

    func showContent(_ animated: Bool = false) {
        webView?.isUserInteractionEnabled = true
        if animated {
            UIView.animate(withDuration: 0.25, animations: { () -> Void in
                self.webView?.alpha = 1.0
            })
        } else {
            webView?.alpha = 1.0
        }
    }

    func addSnackbar(_ bar: SnackBar) {
        bars.append(bar)
        tabDelegate?.tab(self, didAddSnackbar: bar)
    }

    func removeSnackbar(_ bar: SnackBar) {
        if let index = bars.firstIndex(of: bar) {
            bars.remove(at: index)
            tabDelegate?.tab(self, didRemoveSnackbar: bar)
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

    func setScreenshot(_ screenshot: UIImage?, revUUID: Bool = true) {
        self.screenshot = screenshot
        if revUUID {
            self.screenshotUUID = UUID()
        }
    }

    /// Switches user agent Desktop -> Mobile or Mobile -> Desktop.
    func switchUserAgent() {
        if let urlString = webView?.url?.baseDomain {
            // The website was changed once already, need to flip the override.
            if let siteOverride = userAgentOverrides[urlString] {
                userAgentOverrides[urlString] = !siteOverride
            } else {
                // First time switch, adding the basedomain to dictionary with flipped value.
                userAgentOverrides[urlString] = !UserAgent.shouldUseDesktopMode
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

    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        guard let webView = object as? BraveWebView, webView == self.webView,
            let path = keyPath, path == KVOConstants.URL.rawValue else {
            return assertionFailure("Unhandled KVO key: \(keyPath ?? "nil")")
        }
        guard let url = self.webView?.url else {
            return
        }

        self.urlDidChangeDelegate?.tab(self, urlDidChangeTo: url)
    }

    func isDescendentOf(_ ancestor: Tab) -> Bool {
        return sequence(first: parent) { $0?.parent }.contains { $0 == ancestor }
    }

    func injectUserScriptWith(fileName: String, type: String = "js", injectionTime: WKUserScriptInjectionTime = .atDocumentEnd, mainFrameOnly: Bool = true) {
        guard let webView = self.webView else {
            return
        }
        if let path = Bundle.main.path(forResource: fileName, ofType: type),
            let source = try? String(contentsOfFile: path) {
            let userScript = WKUserScript(source: source, injectionTime: injectionTime, forMainFrameOnly: mainFrameOnly)
            webView.configuration.userContentController.addUserScript(userScript)
        }
    }

    func observeURLChanges(delegate: URLChangeDelegate) {
        self.urlDidChangeDelegate = delegate
    }

    func removeURLChangeObserver(delegate: URLChangeDelegate) {
        if let existing = self.urlDidChangeDelegate, existing === delegate {
            self.urlDidChangeDelegate = nil
        }
    }
}

extension Tab: TabWebViewDelegate {
    fileprivate func tabWebView(_ tabWebView: TabWebView, didSelectFindInPageForSelection selection: String) {
        tabDelegate?.tab(self, didSelectFindInPageForSelection: selection)
    }
}

private class TabContentScriptManager: NSObject, WKScriptMessageHandler {
    fileprivate var helpers = [String: TabContentScript]()
    
    func uninstall(from tab: Tab) {
        helpers.forEach {
            if let name = $0.value.scriptMessageHandlerName() {
                tab.webView?.configuration.userContentController.removeScriptMessageHandler(forName: name)
            }
        }
    }

    @objc func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        for helper in helpers.values {
            if let scriptMessageHandlerName = helper.scriptMessageHandlerName() {
                if scriptMessageHandlerName == message.name {
                    helper.userContentController(userContentController, didReceiveScriptMessage: message)
                    return
                }
            }
        }
    }

    func addContentScript(_ helper: TabContentScript, name: String, forTab tab: Tab) {
        if let _ = helpers[name] {
            assertionFailure("Duplicate helper added: \(name)")
        }

        helpers[name] = helper

        // If this helper handles script messages, then get the handler name and register it. The Tab
        // receives all messages and then dispatches them to the right TabHelper.
        if let scriptMessageHandlerName = helper.scriptMessageHandlerName() {
            tab.webView?.configuration.userContentController.add(self, name: scriptMessageHandlerName)
        }
    }

    func getContentScript(_ name: String) -> TabContentScript? {
        return helpers[name]
    }
}

private protocol TabWebViewDelegate: class {
    func tabWebView(_ tabWebView: TabWebView, didSelectFindInPageForSelection selection: String)
}

class TabWebView: BraveWebView, MenuHelperInterface {
    fileprivate weak var delegate: TabWebViewDelegate?

    override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
        return super.canPerformAction(action, withSender: sender) || action == MenuHelper.selectorFindInPage
    }

    @objc func menuHelperFindInPage() {
        evaluateJavaScript("getSelection().toString()") { result, _ in
            let selection = result as? String ?? ""
            self.delegate?.tabWebView(self, didSelectFindInPageForSelection: selection)
        }
    }

    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        // The find-in-page selection menu only appears if the webview is the first responder.
        becomeFirstResponder()

        return super.hitTest(point, with: event)
    }
    
    // rdar://33283179 Apple bug where `serverTrust` is not defined as KVO when it should be
    override func value(forUndefinedKey key: String) -> Any? {
        if key == #keyPath(WKWebView.serverTrust) {
            return serverTrust
        }

        return super.value(forUndefinedKey: key)
    }
}

///
// Temporary fix for Bug 1390871 - NSInvalidArgumentException: -[WKContentView menuHelperFindInPage]: unrecognized selector
//
// This class only exists to contain the swizzledMenuHelperFindInPage. This class is actually never
// instantiated. It only serves as a placeholder for the method. When the method is called, self is
// actually pointing to a WKContentView. Which is not public, but that is fine, we only need to know
// that it is a UIView subclass to access its superview.
//

class TabWebViewMenuHelper: UIView {
    @objc func swizzledMenuHelperFindInPage() {
        if let tabWebView = superview?.superview as? TabWebView {
            tabWebView.evaluateJavaScript("getSelection().toString()") { result, _ in
                let selection = result as? String ?? ""
                tabWebView.delegate?.tabWebView(tabWebView, didSelectFindInPageForSelection: selection)
            }
        }
    }
}
