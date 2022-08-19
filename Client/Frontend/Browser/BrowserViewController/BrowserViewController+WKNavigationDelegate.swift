/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import Data
import BraveShared
import BraveCore
import BraveUI
import BraveWallet

private let log = Logger.browserLogger
private let rewardsLog = Logger.braveCoreLogger

extension WKNavigationAction {
  /// Allow local requests only if the request is privileged.
  /// If the request is internal or unprivileged, we should deny it.
  var isInternalUnprivileged: Bool {
    guard let url = request.url else {
      return true
    }

    if let url = InternalURL(url) {
      return !url.isAuthorized
    } else {
      return false
    }
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
  public func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
    if tabManager.selectedTab?.webView !== webView {
      return
    }
    toolbarVisibilityViewModel.toolbarState = .expanded

    if let selectedTab = tabManager.selectedTab,
       selectedTab.url?.origin != webView.url?.origin {
      // new site has a different origin, hide wallet icon.
      tabManager.selectedTab?.isWalletIconVisible = false
      // close wallet panel if it's open
      if let popoverController = self.presentedViewController as? PopoverController,
         popoverController.contentController is WalletPanelHostingController {
        self.dismiss(animated: true)
      }
    }

    updateFindInPageVisibility(visible: false)
    displayPageZoom(visible: false)

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

  public func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences, decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
    guard let url = navigationAction.request.url else {
      decisionHandler(.cancel, preferences)
      return
    }

    if InternalURL.isValid(url: url) {
      if navigationAction.navigationType != .backForward, navigationAction.isInternalUnprivileged {
        log.warning("Denying unprivileged request: \(navigationAction.request)")
        decisionHandler(.cancel, preferences)
        return
      }

      decisionHandler(.allow, preferences)
      return
    }

    if url.scheme == "about" {
      decisionHandler(.allow, preferences)
      return
    }

    if url.isBookmarklet {
      decisionHandler(.cancel, preferences)
      return
    }

    webView.configuration.preferences.isFraudulentWebsiteWarningEnabled = SafeBrowsing.isSafeBrowsingEnabledForURL(url)

    // Universal links do not work if the request originates from the app, manual handling is required.
    if let mainDocURL = navigationAction.request.mainDocumentURL,
      let universalLink = UniversalLinkManager.universalLinkType(for: mainDocURL, checkPath: true) {
      switch universalLink {
      case .buyVPN:
        presentCorrespondingVPNViewController()
        decisionHandler(.cancel, preferences)
        return
      }
    }

    // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
    // gives us the exact same behaviour as Safari.
    if ["sms", "tel", "facetime", "facetime-audio"].contains(url.scheme) {
      handleExternalURL(url)
      decisionHandler(.cancel, preferences)
      return
    }

    // Second special case are a set of URLs that look like regular http links, but should be handed over to iOS
    // instead of being loaded in the webview. Note that there is no point in calling canOpenURL() here, because
    // iOS will always say yes. TODO Is this the same as isWhitelisted?

    if isAppleMapsURL(url) {
      handleExternalURL(url)
      decisionHandler(.cancel, preferences)
      return
    }

    if isStoreURL(url) {
      handleExternalURL(url)
      decisionHandler(.cancel, preferences)
      return
    }

    // Handles custom mailto URL schemes.
    if url.scheme == "mailto" {
      handleExternalURL(url)
      decisionHandler(.cancel, preferences)
      return
    }

    let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
    let tab = tabManager[webView]

    let domainForRequestURL = Domain.getOrCreate(
      forUrl: url,
      persistent: !isPrivateBrowsing
    )
    
    // Website redirection logic
    if url.isWebPage(includeDataURIs: false),
       navigationAction.targetFrame?.isMainFrame == true,
       let redirectURL = WebsiteRedirects.redirect(for: url) {
      
      decisionHandler(.cancel, preferences)
      tab?.loadRequest(URLRequest(url: redirectURL))
      return
    }
        
    // Debouncing logic
    // Handle debouncing for main frame only and only if the site (etld+1) changes
    // We also only handle `http` and `https` requests
    if url.isWebPage(includeDataURIs: false),
       let currentURL = tab?.webView?.url,
       currentURL.baseDomain != url.baseDomain,
       domainForRequestURL.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true),
       navigationAction.targetFrame?.isMainFrame == true {
      
      // Lets get the redirect chain.
      // Then we simply get all elements up until the user allows us to redirect
      // (i.e. appropriate settings are enabled for that redirect rule)
      let redirectChain = DebouncingResourceDownloader.shared
        .redirectChain(for: url)
        .contiguousUntil { _, rule in
          return rule.preferences.allSatisfy { pref in
            switch pref {
            case .deAmpEnabled:
              return Preferences.Shields.autoRedirectAMPPages.value
            }
          }
        }
      
      // Once we check the redirect chain only need the last (final) url from our redirect chain
      if let redirectURL = redirectChain.last?.url {
        // Cancel the original request. We don't want it to load as it's tracking us
        decisionHandler(.cancel, preferences)

        // For now we only allow the `Referer`. The browser will add other headers during navigation.
        var modifiedRequest = URLRequest(url: redirectURL)

        for (headerKey, headerValue) in navigationAction.request.allHTTPHeaderFields ?? [:] {
          guard headerKey == "Referer" else { continue }
          modifiedRequest.setValue(headerValue, forHTTPHeaderField: headerKey)
        }

        tab?.loadRequest(modifiedRequest)
      }
    }
    
    // Add de-amp script
    // The user script manager will take care to not reload scripts if this value doesn't change
    tab?.userScriptManager?.isDeAMPEnabled = Preferences.Shields.autoRedirectAMPPages.value && navigationAction.targetFrame?.isMainFrame == true
      
    // Add request blocking script
    // This script will block certian `xhr` and `window.fetch()` requests
    tab?.userScriptManager?.isRequestBlockingEnabled = url.isWebPage(includeDataURIs: false) &&
      domainForRequestURL.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true)

    // Check if custom user scripts must be added to or removed from the web view.
    tab?.userScriptManager?.userScriptTypes = UserScriptHelper.getUserScriptTypes(
      for: navigationAction, options: isPrivateBrowsing ? .privateBrowsing : .default
    )

    // Brave Search logic.

    if navigationAction.targetFrame?.isMainFrame == true,
      BraveSearchManager.isValidURL(url) {
      // We fetch cookies to determine if backup search was enabled on the website.
      let profile = self.profile
      webView.configuration.websiteDataStore.httpCookieStore.getAllCookies { cookies in
        tab?.braveSearchManager = BraveSearchManager(profile: profile, url: url, cookies: cookies)
        if let braveSearchManager = tab?.braveSearchManager {
          braveSearchManager.fallbackQueryResultsPending = true
          braveSearchManager.shouldUseFallback { backupQuery in
            guard let query = backupQuery else {
              braveSearchManager.fallbackQueryResultsPending = false
              return
            }

            if query.found {
              braveSearchManager.fallbackQueryResultsPending = false
            } else {
              braveSearchManager.backupSearch(with: query) { completion in
                braveSearchManager.fallbackQueryResultsPending = false
                tab?.injectResults()
              }
            }
          }
        }
      }
    } else {
      tab?.braveSearchManager = nil
    }

    // This is the normal case, opening a http or https url, which we handle by loading them in this WKWebView. We
    // always allow this. Additionally, data URIs are also handled just like normal web pages.

    if ["http", "https", "data", "blob", "file"].contains(url.scheme) {
      if navigationAction.targetFrame?.isMainFrame == true {
        tabManager[webView]?.updateUserAgent(webView, newURL: url)
      }

      pendingRequests[url.absoluteString] = navigationAction.request

      // TODO: Downgrade to 14.5 once api becomes available.
      if #unavailable(iOS 15.0) {
        if Preferences.Shields.httpsEverywhere.value,
          url.scheme == "http",
          let urlHost = url.normalizedHost() {
          HttpsEverywhereStats.shared.shouldUpgrade(url) { shouldupgrade in
            DispatchQueue.main.async {
              if shouldupgrade {
                self.pendingHTTPUpgrades[urlHost] = navigationAction.request
              }
            }
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
      if let mainDocumentURL = navigationAction.request.mainDocumentURL,
        mainDocumentURL.schemelessAbsoluteString == url.schemelessAbsoluteString,
        !(InternalURL(url)?.isSessionRestore ?? false),
        navigationAction.sourceFrame.isMainFrame || navigationAction.targetFrame?.isMainFrame == true {
        // Identify specific block lists that need to be applied to the requesting domain
        let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: !isPrivateBrowsing)
        let (on, off) = BlocklistName.blocklists(forDomain: domainForShields)
        let controller = webView.configuration.userContentController

        // Grab all lists that have valid rules and add/remove them as necessary
        on.compactMap { $0.rule }.forEach(controller.add)
        off.compactMap { $0.rule }.forEach(controller.remove)

        let isScriptsEnabled = !domainForShields.isShieldExpected(.NoScript, considerAllShieldsOption: true)
        preferences.allowsContentJavaScript = isScriptsEnabled
      }

      // Cookie Blocking code below
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

      decisionHandler(.allow, preferences)
      return
    }

    // Standard schemes are handled in previous if-case.
    // This check handles custom app schemes to open external apps.
    // Our own 'brave' scheme does not require the switch-app prompt.
    if url.scheme?.contains("brave") == false {
      handleExternalURL(url) { didOpenURL in
        // Do not show error message for JS navigated links or redirect
        // as it's not the result of a user action.
        if !didOpenURL, navigationAction.navigationType == .linkActivated {
          let alert = UIAlertController(title: Strings.unableToOpenURLErrorTitle, message: Strings.unableToOpenURLError, preferredStyle: .alert)
          alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
          self.present(alert, animated: true, completion: nil)
        }
      }
    }
    decisionHandler(.cancel, preferences)
  }

  public func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse, decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void) {
    let response = navigationResponse.response
    let responseURL = response.url

    if let tab = tabManager[webView],
      let responseURL = responseURL,
      InternalURL(responseURL)?.isSessionRestore == true {
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
          self.tabManager[webView]?.contentBlocker.stats = stats.adding(httpsCount: 1)
        }
      }
    }

    // Check if this response should be handed off to Passbook.
    if let passbookHelper = OpenPassBookHelper(request: request, response: response, canShowInWebView: canShowInWebView, forceDownload: forceDownload, browserViewController: self) {
      // Open our helper and cancel this response from the webview.
      passbookHelper.open()
      decisionHandler(.cancel)
      return
    }

    // Check if this response should be downloaded.
    let cookieStore = webView.configuration.websiteDataStore.httpCookieStore
    if let downloadHelper = DownloadHelper(request: request, response: response, cookieStore: cookieStore, canShowInWebView: canShowInWebView, forceDownload: forceDownload) {
      // Clear the pending download web view so that subsequent navigations from the same
      // web view don't invoke another download.
      pendingDownloadWebView = nil

      let downloadAlertAction: (HTTPDownload) -> Void = { [weak self] download in
        self?.downloadQueue.enqueue(download)
      }

      // Open our helper and cancel this response from the webview.
      if let downloadAlert = downloadHelper.downloadAlert(from: view, okAction: downloadAlertAction) {
        present(downloadAlert, animated: true, completion: nil)
      }
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

      tab.mimeType = response.mimeType
    }

    // If none of our helpers are responsible for handling this response,
    // just let the webview handle it as normal.
    decisionHandler(.allow)
  }

  public func webView(_ webView: WKWebView, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {

    // If this is a certificate challenge, see if the certificate has previously been
    // accepted by the user.
    let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
      let trust = challenge.protectionSpace.serverTrust,
      let cert = SecTrustGetCertificateAtIndex(trust, 0), profile.certStore.containsCertificate(cert, forOrigin: origin) {
      completionHandler(.useCredential, URLCredential(trust: trust))
      return
    }

    guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM,
      let tab = tabManager[webView]
    else {
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
    Task { @MainActor in
      do {
        let credentials = try await Authenticator.handleAuthRequest(self, challenge: challenge, loginsHelper: loginsHelper)
        completionHandler(.useCredential, credentials.credentials)
      } catch {
        completionHandler(.rejectProtectionSpace, nil)
      }
    }
  }

  public func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    guard let tab = tabManager[webView] else { return }
    // Set the committed url which will also set tab.url
    tab.committedURL = webView.url
    
    // Need to evaluate Night mode script injection after url is set inside the Tab
    tab.nightMode = Preferences.General.nightModeEnabled.value

    rewards.reportTabNavigation(tabId: tab.rewardsId)

    if tabManager.selectedTab === tab {
      updateUIForReaderHomeStateForTab(tab)
    }
  }

  public func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    if let tab = tabManager[webView] {
      
      // Deciding whether to inject app's IAP receipt for Brave SKUs or not
      if let url = tab.url,
          let braveSkusHelper = BraveSkusWebHelper(for: url),
          let receiptData = braveSkusHelper.receiptData,
          !tab.isPrivate {
        tab.injectSessionStorageItem(key: receiptData.key, value: receiptData.value)
      }

      // Second attempt to inject results to the BraveSearch.
      // This will be called if we got fallback results faster than
      // the page navigation.
      if let braveSearchManager = tab.braveSearchManager {
        // Fallback results are ready before navigation finished,
        // they must be injected here.
        if !braveSearchManager.fallbackQueryResultsPending {
          tab.injectResults()
        }
      } else {
        // If not applicable, null results must be injected regardless.
        // The website waits on us until this is called with either results or null.
        tab.injectResults()
      }

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
      tab.updateEthereumProperties()
      tab.reportPageLoad(to: rewards, redirectionURLs: tab.redirectURLs)
      tab.redirectURLs = []
      if webView.url?.isLocal == false {
        // Reset should classify
        tab.shouldClassifyLoadsForAds = true
        // Set rewards inter site url as new page load url.
        rewardsXHRLoadURL = webView.url
      }

      tabsBar.reloadDataAndRestoreSelectedTab()
      
      if tab.walletEthProvider != nil {
        tab.emitEthereumEvent(.connect)
      }
    }

    // Cosmetic Filters
    do {
      // If the URL is not an `internal://`
      // about:home url
      // local-host url
      // or any sort of internal-url, then we run cosmetic filters on it.
      // If ad-block shields are enabled for this URL
      if let url = webView.url,
        !InternalURL.isValid(url: url),
        !(InternalURL(url)?.isSessionRestore ?? false),
        Domain.getOrCreate(
          forUrl: url,
          persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing
        )
        .isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true),
        let cosmeticFiltersScript = try AdBlockStats.shared.cosmeticFiltersScript(for: url) {
        // Execute the cosmetic filters script in the cosmetic filters sandbox world
        webView.evaluateSafeJavaScript(functionName: cosmeticFiltersScript, args: [], contentWorld: .cosmeticFiltersSandbox, asFunction: false) { _, error in
          log.error("AdblockRustInjector error: \(String(describing: error))")
        }
      }
    } catch {
      log.error(error)
    }

    // Added this method to determine long press menu actions better
    // Since these actions are depending on tabmanager opened WebsiteCount
    updateToolbarUsingTabManager(tabManager)
  }

  public func webView(_ webView: WKWebView, didReceiveServerRedirectForProvisionalNavigation navigation: WKNavigation!) {
    guard let tab = tabManager[webView], let url = webView.url, rewards.isEnabled else { return }
    tab.redirectURLs.append(url)
  }
}

extension WKNavigationType: CustomDebugStringConvertible {
  public var debugDescription: String {
    switch self {
    case .linkActivated: return "linkActivated"
    case .formResubmitted: return "formResubmitted"
    case .backForward: return "backForward"
    case .formSubmitted: return "formSubmitted"
    case .other: return "other"
    case .reload: return "reload"
    @unknown default:
      return "Unknown(\(rawValue))"
    }
  }
}
