/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import Data
import BraveShared
import BraveRewards

private let log = Logger.browserLogger
private let rewardsLog = Logger.rewardsLogger

extension WKNavigationAction {
    /// Allow local requests only if the request is privileged.
    var isAllowed: Bool {
        guard let url = request.url else {
            return true
        }

        return !url.isWebPage(includeDataURIs: false) || !url.isLocal || request.isPrivileged
    }
}

extension URL {
    /// Obtain a schemeless absolute string
    fileprivate var schemelessAbsoluteString: String {
        guard let scheme = self.scheme else { return absoluteString }
        return absoluteString.replacingOccurrences(of: "\(scheme)://", with: "")
    }
}

extension BrowserViewController {
    fileprivate func handleExternalURL(_ url: URL, openedURLCompletionHandler: ((Bool) -> Void)? = nil) {
        self.view.endEditing(true)
        let popup = AlertPopupView(
            imageView: nil,
            title: Strings.openExternalAppURLTitle,
            message: String(format: Strings.openExternalAppURLMessage, url.relativeString),
            titleWeight: .semibold,
            titleSize: 21
        )
        popup.addButton(title: Strings.openExternalAppURLDontAllow, fontSize: 16) { () -> PopupViewDismissType in
            return .flyDown
        }
        popup.addButton(title: Strings.openExternalAppURLAllow, type: .primary, fontSize: 16) { () -> PopupViewDismissType in
            UIApplication.shared.open(url, options: [:], completionHandler: openedURLCompletionHandler)
            return .flyDown
        }
        popup.showWithType(showType: .flyUp)
    }
}

extension BrowserViewController: WKNavigationDelegate {
    func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
        if tabManager.selectedTab?.webView !== webView {
            return
        }

        updateFindInPageVisibility(visible: false)

        // If we are going to navigate to a new page, hide the reader mode button. Unless we
        // are going to a about:reader page. Then we keep it on screen: it will change status
        // (orange color) as soon as the page has loaded.
        if let url = webView.url {
            if !url.isReaderModeURL {
                topToolbar.updateReaderModeState(ReaderModeState.unavailable)
                hideReaderModeBar(animated: false)
            }
        }
    }

    // Recognize an Apple Maps URL. This will trigger the native app. But only if a search query is present. Otherwise
    // it could just be a visit to a regular page on maps.apple.com.
    fileprivate func isAppleMapsURL(_ url: URL) -> Bool {
        if url.scheme == "http" || url.scheme == "https" {
            if url.host == "maps.apple.com" && url.query != nil {
                return true
            }
        }
        return false
    }

    // Recognize a iTunes Store URL. These all trigger the native apps. Note that appstore.com and phobos.apple.com
    // used to be in this list. I have removed them because they now redirect to itunes.apple.com. If we special case
    // them then iOS will actually first open Safari, which then redirects to the app store. This works but it will
    // leave a 'Back to Safari' button in the status bar, which we do not want.
    fileprivate func isStoreURL(_ url: URL) -> Bool {
        if url.scheme == "http" || url.scheme == "https" {
            if url.host == "itunes.apple.com" {
                return true
            }
        }
        if url.scheme == "itms-appss" || url.scheme == "itmss" {
            return true
        }
        return false
    }

    // This is the place where we decide what to do with a new navigation action. There are a number of special schemes
    // and http(s) urls that need to be handled in a different way. All the logic for that is inside this delegate
    // method.
    
    fileprivate func isUpholdOAuthAuthorization(_ url: URL) -> Bool {
        return url.scheme == "rewards" && url.host == "uphold"
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        guard let url = navigationAction.request.url else {
            decisionHandler(.cancel)
            return
        }
        
        if let customHeader = UserReferralProgram.shouldAddCustomHeader(for: navigationAction.request) {
            decisionHandler(.cancel)
            var newRequest = navigationAction.request
            UrpLog.log("Adding custom header: [\(customHeader.field): \(customHeader.value)] for domain: \(newRequest.url?.absoluteString ?? "404")")
            newRequest.addValue(customHeader.value, forHTTPHeaderField: customHeader.field)
            webView.load(newRequest)
            return
        }

        if url.scheme == "about" {
            decisionHandler(.allow)
            return
        }
        
        if url.isBookmarklet {
            decisionHandler(.cancel)
            return
        }

        if !navigationAction.isAllowed && navigationAction.navigationType != .backForward {
            log.warning("Denying unprivileged request: \(navigationAction.request)")
            decisionHandler(.cancel)
            return
        }
        
        if let safeBrowsing = safeBrowsing, safeBrowsing.shouldBlock(url) {
            safeBrowsing.showMalwareWarningPage(forUrl: url, inWebView: webView)
            decisionHandler(.cancel)
            return
        }
        
        #if !NO_USER_WALLETS
        if isUpholdOAuthAuthorization(url) {
            decisionHandler(.cancel)
            guard let tab = tabManager[webView], let queryItems = URLComponents(url: url, resolvingAgainstBaseURL: false)?.queryItems else {
                return
            }
            var items: [String: String] = [:]
            for query in queryItems {
                items[query.name] = query.value
            }
            authorizeUpholdWallet(from: tab, queryItems: items)
            return
        }
        #endif

        // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
        // gives us the exact same behaviour as Safari.
        if url.scheme == "tel" || url.scheme == "facetime" || url.scheme == "facetime-audio" {
            handleExternalURL(url)
            decisionHandler(.cancel)
            return
        }

        // Second special case are a set of URLs that look like regular http links, but should be handed over to iOS
        // instead of being loaded in the webview. Note that there is no point in calling canOpenURL() here, because
        // iOS will always say yes. TODO Is this the same as isWhitelisted?

        if isAppleMapsURL(url) {
            handleExternalURL(url)
            decisionHandler(.cancel)
            return
        }

        if isStoreURL(url) {
            handleExternalURL(url)
            decisionHandler(.cancel)
            return
        }

        // Handles custom mailto URL schemes.
        if url.scheme == "mailto" {
            handleExternalURL(url)
            decisionHandler(.cancel)
            return
        }
        
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        
        if url.baseDomain == "youtube.com" {
            let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
            tabManager[webView]?.userScriptManager?.isYoutubeAdblockEnabled = domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true)
        }
        
        // This is the normal case, opening a http or https url, which we handle by loading them in this WKWebView. We
        // always allow this. Additionally, data URIs are also handled just like normal web pages.

        if ["http", "https", "data", "blob", "file"].contains(url.scheme) {
            if navigationAction.targetFrame?.isMainFrame == true {
                tabManager[webView]?.updateUserAgent(webView, newURL: url)
            }

            pendingRequests[url.absoluteString] = navigationAction.request
            
            if let urlHost = url.normalizedHost() {
                if let mainDocumentURL = navigationAction.request.mainDocumentURL, url.scheme == "http" {
                    let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: !isPrivateBrowsing)
                    if domainForShields.isShieldExpected(.HTTPSE, considerAllShieldsOption: true) && HttpsEverywhereStats.shared.shouldUpgrade(url) {
                        // Check if HTTPSE is on and if it is, whether or not this http url would be upgraded
                        pendingHTTPUpgrades[urlHost] = navigationAction.request
                    }
                }
            }

            // Adblock logic,
            // Only use main document URL, not the request URL
            // If an iFrame is loaded, shields depending on the main frame, not the iFrame request
            
            // Weird behavior here with `targetFram` and `sourceFrame`, on refreshing page `sourceFrame` is not nil (it is non-optional)
            //  however, it is still an uninitialized object, making it an unreliable source to compare `isMainFrame` against.
            //  Rather than using `sourceFrame.isMainFrame` or even comparing `sourceFrame == targetFrame`, a simple URL check is used.
            // No adblocking logic is be used on session restore urls. It uses javascript to retrieve the
            // request then the page is reloaded with a proper url and adblocking rules are applied.
            if
                let mainDocumentURL = navigationAction.request.mainDocumentURL,
                mainDocumentURL.schemelessAbsoluteString == url.schemelessAbsoluteString,
                !url.isSessionRestoreURL,
                navigationAction.sourceFrame.isMainFrame || navigationAction.targetFrame?.isMainFrame == true {
                
                // Identify specific block lists that need to be applied to the requesting domain
                let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: !isPrivateBrowsing)
                let (on, off) = BlocklistName.blocklists(forDomain: domainForShields)
                let controller = webView.configuration.userContentController
                
                // Grab all lists that have valid rules and add/remove them as necessary
                on.compactMap { $0.rule }.forEach(controller.add)
                off.compactMap { $0.rule }.forEach(controller.remove)
              
                if let tab = tabManager[webView] {
                    tab.userScriptManager?.isFingerprintingProtectionEnabled =
                        domainForShields.isShieldExpected(.FpProtection, considerAllShieldsOption: true)
                }

                webView.configuration.preferences.javaScriptEnabled = !domainForShields.isShieldExpected(.NoScript, considerAllShieldsOption: true)
            }
            
            //Cookie Blocking code below
            if let tab = tabManager[webView] {
                tab.userScriptManager?.isCookieBlockingEnabled = Preferences.Privacy.blockAllCookies.value
            }
            
            if let rule = BlocklistName.cookie.rule {
                if Preferences.Privacy.blockAllCookies.value {
                    webView.configuration.userContentController.add(rule)
                } else {
                    webView.configuration.userContentController.remove(rule)
                }
            }
            // Reset the block alert bool on new host. 
            if let newHost: String = url.host, let oldHost: String = webView.url?.host, newHost != oldHost {
                self.tabManager.selectedTab?.alertShownCount = 0
                self.tabManager.selectedTab?.blockAllAlerts = false
            }
            decisionHandler(.allow)
            return
        }

        // Ignore JS navigated links, the intention is to match Safari and native WKWebView behaviour.
        if navigationAction.navigationType == .linkActivated {
            handleExternalURL(url) { didOpenURL in
                if !didOpenURL {
                    let alert = UIAlertController(title: Strings.unableToOpenURLErrorTitle, message: Strings.unableToOpenURLError, preferredStyle: .alert)
                    alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
                    self.present(alert, animated: true, completion: nil)
                }
            }
        }
        decisionHandler(.cancel)
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse, decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void) {
        let response = navigationResponse.response
        let responseURL = response.url
        
        if let tab = tabManager[webView], responseURL?.isSessionRestoreURL == true {
            tab.shouldClassifyLoadsForAds = false
        }
        
        var request: URLRequest?
        if let url = responseURL {
            request = pendingRequests.removeValue(forKey: url.absoluteString)
        }
        
        // We can only show this content in the web view if this web view is not pending
        // download via the context menu.
        let canShowInWebView = navigationResponse.canShowMIMEType && (webView != pendingDownloadWebView)
        let forceDownload = webView == pendingDownloadWebView
        
        if let url = responseURL, let urlHost = responseURL?.normalizedHost() {
            // If an upgraded https load happens with a host which was upgraded, increase the stats
            if url.scheme == "https", let _ = pendingHTTPUpgrades.removeValue(forKey: urlHost) {
                BraveGlobalShieldStats.shared.httpse += 1
                if let stats = self.tabManager[webView]?.contentBlocker.stats {
                    self.tabManager[webView]?.contentBlocker.stats = stats.create(byAddingListItem: .https)
                }
            }
        }

        // Check if this response should be handed off to Passbook.
        if let passbookHelper = OpenPassBookHelper(request: request, response: response, canShowInWebView: canShowInWebView, forceDownload: forceDownload, browserViewController: self) {
            // Clear the network activity indicator since our helper is handling the request.
            UIApplication.shared.isNetworkActivityIndicatorVisible = false

            // Open our helper and cancel this response from the webview.
            passbookHelper.open()
            decisionHandler(.cancel)
            return
        }
        
        // Check if this response should be downloaded.
        if let downloadHelper = DownloadHelper(request: request, response: response, canShowInWebView: canShowInWebView, forceDownload: forceDownload, browserViewController: self) {
            // Clear the network activity indicator since our helper is handling the request.
            UIApplication.shared.isNetworkActivityIndicatorVisible = false
            
            // Clear the pending download web view so that subsequent navigations from the same
            // web view don't invoke another download.
            pendingDownloadWebView = nil
            
            // Open our helper and cancel this response from the webview.
            downloadHelper.open()
            decisionHandler(.cancel)
            return
        }
        
        // If the content type is not HTML, create a temporary document so it can be downloaded and
        // shared to external applications later. Otherwise, clear the old temporary document.
        if let tab = tabManager[webView], navigationResponse.isForMainFrame {
            if response.mimeType?.isKindOfHTML == false, let request = request {
                tab.temporaryDocument = TemporaryDocument(preflightResponse: response, request: request, tab: tab)
            } else {
                tab.temporaryDocument = nil
            }
        }

        // If none of our helpers are responsible for handling this response,
        // just let the webview handle it as normal.
        decisionHandler(.allow)
    }
    
    @available(iOS 13.0, *)
    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences, decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
        
        self.webView(webView, decidePolicyFor: navigationAction) {
            decisionHandler($0, preferences)
        }
    }

    func webView(_ webView: WKWebView, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {

        // If this is a certificate challenge, see if the certificate has previously been
        // accepted by the user.
        let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
           let trust = challenge.protectionSpace.serverTrust,
           let cert = SecTrustGetCertificateAtIndex(trust, 0), profile.certStore.containsCertificate(cert, forOrigin: origin) {
            completionHandler(.useCredential, URLCredential(trust: trust))
            return
        }

        guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic ||
              challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest ||
              challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM,
              let tab = tabManager[webView] else {
            completionHandler(.performDefaultHandling, nil)
            return
        }

        // If this is a request to our local web server, use our private credentials.
        if challenge.protectionSpace.host == "localhost" && challenge.protectionSpace.port == Int(WebServer.sharedInstance.server.port) {
            completionHandler(.useCredential, WebServer.sharedInstance.credentials)
            return
        }

        // The challenge may come from a background tab, so ensure it's the one visible.
        tabManager.selectTab(tab)

        let loginsHelper = tab.getContentScript(name: LoginsHelper.name()) as? LoginsHelper
        Authenticator.handleAuthRequest(self, challenge: challenge, loginsHelper: loginsHelper).uponQueue(.main) { res in
            if let credentials = res.successValue {
                completionHandler(.useCredential, credentials.credentials)
            } else {
                completionHandler(.rejectProtectionSpace, nil)
            }
        }
    }

    func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
        guard let tab = tabManager[webView] else { return }

        tab.url = webView.url
        self.scrollController.resetZoomState()

        rewards.reportTabNavigation(tabId: tab.rewardsId)
        
        if tabManager.selectedTab === tab {
            updateUIForReaderHomeStateForTab(tab)
        }
    }
    
    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        if let tab = tabManager[webView] {
            navigateInTab(tab: tab, to: navigation)
            if let url = tab.url, tab.shouldClassifyLoadsForAds {
                let faviconURL = URL(string: tab.displayFavicon?.url ?? "")
                rewards.reportTabUpdated(
                    Int(tab.rewardsId),
                    url: url,
                    faviconURL: faviconURL,
                    isSelected: tabManager.selectedTab == tab,
                    isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing
                )
            }
            tab.reportPageLoad(to: rewards)
            if webView.url?.isLocal == false {
                // Reset should classify
                tab.shouldClassifyLoadsForAds = true
                // Set rewards inter site url as new page load url.
                rewardsXHRLoadURL = webView.url
            }
            
            tabsBar.reloadDataAndRestoreSelectedTab()
        }
    }
}
