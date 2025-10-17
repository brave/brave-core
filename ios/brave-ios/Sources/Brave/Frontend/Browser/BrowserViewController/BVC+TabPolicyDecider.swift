// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import Data
import Foundation
import Growth
import MarketplaceKit
import OSLog
import Preferences
import Shared
import UIKit
import Web

extension BrowserViewController: TabPolicyDecider {
  public func tab(
    _ tab: some TabState,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
    let responseURL = response.url

    // Store the response in the tab
    if let responseURL = responseURL {
      tab.responses?[responseURL] = response
    }

    // Check if we upgraded to https and if so we need to update the url of frame evaluations
    if let responseURL = responseURL,
      let pageData = tab.currentPageData,
      tab.currentPageData?.upgradeFrameURL(
        forResponseURL: responseURL,
        isForMainFrame: responseInfo.isForMainFrame
      ) == true
    {
      let scriptTypes =
        await tab.currentPageData?.makeUserScriptTypes(
          isDeAmpEnabled: profileController.deAmpPrefs.isDeAmpEnabled,
          isAdBlockEnabled: tab.braveShieldsHelper?.shieldLevel(
            for: pageData.mainFrameURL,
            considerAllShieldsOption: true
          ).isEnabled ?? true,
          isBlockFingerprintingEnabled: tab.braveShieldsHelper?.isShieldExpected(
            for: pageData.mainFrameURL,
            shield: .fpProtection,
            considerAllShieldsOption: true
          ) ?? true
        ) ?? []
      tab.browserData?.setCustomUserScript(scripts: scriptTypes)
    }

    if let responseURL = responseURL,
      let response = response as? HTTPURLResponse
    {
      let internalUrl = InternalURL(responseURL)

      tab.rewardsReportingState?.httpStatusCode = response.statusCode
    }

    let request = response.url.flatMap { pendingRequests[$0.absoluteString] }

    // If the content type is not HTML, create a temporary document so it can be downloaded and
    // shared to external applications later. Otherwise, clear the old temporary document.
    if responseInfo.isForMainFrame {
      if response.mimeType?.isKindOfHTML == false, let request {
        tab.temporaryDocument = TemporaryDocument(
          preflightResponse: response,
          request: request,
          tab: tab
        )
      } else {
        tab.temporaryDocument = nil
      }
    }

    return .allow
  }

  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url else {
      return .cancel
    }
    let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing

    if tab.isExternalAppAlertPresented == true {
      tab.externalAppPopup?.dismissWithType(dismissType: .noAnimation)
      tab.externalAppPopupContinuation?.resume(with: .success(false))
      tab.externalAppPopupContinuation = nil
      tab.externalAppPopup = nil
    }

    tab.rewardsReportingState?.wasRestored = tab.isRestoring

    // Handle internal:// urls
    if InternalURL.isValid(url: requestURL) {
      // Requests for Internal pages have a 60s timeout by default
      let isPrivilegedRequest =
        Int64(request.timeoutInterval) < Int64(Int32.max) || request.isPrivileged

      if !isPrivilegedRequest {
        Logger.module.error("Denying Unprivileged Request: \(request)")
      }
      return .allow
    }

    if requestURL.scheme == "about" {
      return .allow
    }

    if requestURL.isBookmarklet {
      return .cancel
    }

    // Universal links do not work if the request originates from the app, manual handling is required.
    if let mainDocURL = request.mainDocumentURL,
      let universalLink = UniversalLinkManager.universalLinkType(for: mainDocURL, checkPath: true),
      universalLink == .buyVPN
    {
      presentCorrespondingVPNViewController()
      return .cancel
    }

    // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
    // gives us the exact same behaviour as Safari.
    // tel:, facetime:, facetime-audio:, already has its own native alert displayed by the OS!
    if ["sms", "mailto"].contains(requestURL.scheme) {
      let shouldOpen = await handleExternalURL(
        requestURL,
        tab: tab,
        requestInfo: requestInfo
      )
      return (shouldOpen ? .allow : .cancel)
    }

    // The system's prompt could handle these via UIApplication.shared.openURL.
    // However, this can lead to a spoof if the prompt shows,
    // then a new tab is opened while the prompt is showing.
    // There is no way to dismiss the system prompt when navigation changes!
    // So handle it manually
    if ["tel", "facetime", "facetime-audio"].contains(requestURL.scheme) {
      let shouldOpen = await handleExternalURL(
        requestURL,
        tab: tab,
        requestInfo: requestInfo
      )
      return (shouldOpen ? .allow : .cancel)
    }

    // Second special case are a set of URLs that look like regular http links, but should be handed over to iOS
    // instead of being loaded in the webview.
    // In addition we are exchaging actual scheme with "maps" scheme
    // So the Apple maps URLs will open properly
    if let mapsURL = isAppleMapsURL(requestURL), mapsURL.enabled {
      let shouldOpen = await handleExternalURL(
        mapsURL.url,
        tab: tab,
        requestInfo: requestInfo
      )
      return shouldOpen ? .allow : .cancel
    }

    if #available(iOS 17.4, *), !ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Accessing `MarketplaceKitURIScheme` on Vision OS results in a crash
      if requestURL.scheme == MarketplaceKitURIScheme {
        if let queryItems = URLComponents(url: requestURL, resolvingAgainstBaseURL: false)?
          .queryItems,
          let adpURL = queryItems.first(where: {
            $0.name.caseInsensitiveCompare("alternativeDistributionPackage") == .orderedSame
          })?.value?.asURL,
          requestInfo.isMainFrame,
          adpURL.baseDomain == request.url?.baseDomain
        {
          return .allow
        }
        return .cancel
      }
    }

    if isStoreURL(requestURL) {
      let shouldOpen = await handleExternalURL(
        requestURL,
        tab: tab,
        requestInfo: requestInfo
      )
      return shouldOpen ? .allow : .cancel
    }

    // handles Decentralized DNS
    if let decentralizedDNSHelper = self.decentralizedDNSHelperFor(url: requestURL),
      requestInfo.isMainFrame
    {
      topToolbar.locationView.loading = true
      let result = await decentralizedDNSHelper.lookup(
        domain: requestURL.schemelessAbsoluteDisplayString
      )
      topToolbar.locationView.loading = tabManager.selectedTab?.isLoading == true
      guard !Task.isCancelled else {  // user pressed stop, or typed new url
        return .cancel
      }
      switch result {
      case .loadInterstitial(let service):
        showWeb3ServiceInterstitialPage(service: service, originalURL: requestURL)
        return .cancel
      case .load(let resolvedURL):
        if resolvedURL.isIPFSScheme,
          let resolvedIPFSURL = profileController.ipfsAPI.resolveGatewayUrl(for: resolvedURL)
        {
          // FIXME: This should cancel & load the resolvedIPFSURL
        } else {
          // FIXME: This should cancel & load the resolvedURL
        }
      case .none:
        break
      }
    }

    tab.rewardsReportingState?.isNewNavigation =
      requestInfo.navigationType != .backForward && requestInfo.navigationType != .reload
    tab.currentRequestURL = requestURL

    // Website redirection logic
    if requestURL.isWebPage(includeDataURIs: false),
      requestInfo.isMainFrame,
      let redirectURL = WebsiteRedirects.redirect(for: requestURL)
    {

      tab.loadRequest(URLRequest(url: redirectURL))
      return .cancel
    }

    // Shields

    // before loading any ad-block scripts
    // await the preparation of the ad-block services
    await LaunchHelper.shared.prepareAdBlockServices(
      adBlockService: self.braveCore.adblockService
    )

    if let mainDocumentURL = request.mainDocumentURL {
      if mainDocumentURL != tab.currentPageData?.mainFrameURL {
        // Clear the current page data if the page changes.
        // Do this before anything else so that we have a clean slate.
        tab.currentPageData = PageData(mainFrameURL: mainDocumentURL)
      }

      // Handle the "forget me" feature on navigation
      if requestInfo.isMainFrame {
        // Cancel any forget data requests
        tabManager.cancelForgetData(for: mainDocumentURL, in: tab)

        // Forget any websites that have "forget me" enabled
        // if we navigated away from the previous domain
        if let currentURL = tab.visibleURL,
          !InternalURL.isValid(url: currentURL),
          let currentETLDP1 = currentURL.baseDomain,
          mainDocumentURL.baseDomain != currentETLDP1
        {
          tabManager.forgetDataIfNeeded(for: currentURL, in: tab)
        }
      }

      let isAdBlockEnabled =
        tab.braveShieldsHelper?.shieldLevel(
          for: mainDocumentURL,
          considerAllShieldsOption: true
        ).isEnabled ?? true
      let isBlockFingerprintingEnabled =
        tab.braveShieldsHelper?.isShieldExpected(
          for: mainDocumentURL,
          shield: .fpProtection,
          considerAllShieldsOption: true
        ) ?? true

      if let modifiedRequest = getInternalRedirect(
        from: request,
        isMainFrame: requestInfo.isMainFrame,
        in: tab,
        isAdBlockEnabled: isAdBlockEnabled
      ) {
        tab.isInternalRedirect = true
        tab.loadRequest(modifiedRequest)

        if let url = modifiedRequest.url {
          Logger.module.debug(
            "Redirected to `\(url.absoluteString, privacy: .private)`"
          )
        }

        return .cancel
      } else {
        tab.isInternalRedirect = false
      }

      // Set some additional user scripts
      if requestInfo.isMainFrame {
        tab.browserData?.setScripts(scripts: [
          // Add de-amp script
          // The user script manager will take care to not reload scripts if this value doesn't change
          .deAmp: profileController.deAmpPrefs.isDeAmpEnabled,

          // Add request blocking script
          // This script will block certian `xhr` and `window.fetch()` requests
          .requestBlocking: requestURL.isWebPage(includeDataURIs: false)
            && isAdBlockEnabled,

          // The tracker protection script
          // This script will track what is blocked and increase stats
          .trackerProtectionStats: requestURL.isWebPage(includeDataURIs: false)
            && isAdBlockEnabled,

          // Add Brave search result ads processing script
          // This script will process search result ads on the Brave search page.
          .searchResultAd: BraveSearchManager.isValidURL(requestURL) && !isPrivateBrowsing,
        ])
      }

      if !requestInfo.isNewWindow {
        // Check if custom user scripts must be added to or removed from the web view.
        tab.currentPageData?.addSubframeURL(
          forRequestURL: requestURL,
          isForMainFrame: requestInfo.isMainFrame
        )
        let scriptTypes =
          await tab.currentPageData?.makeUserScriptTypes(
            isDeAmpEnabled: profileController.deAmpPrefs.isDeAmpEnabled,
            isAdBlockEnabled: isAdBlockEnabled,
            isBlockFingerprintingEnabled: isBlockFingerprintingEnabled
          ) ?? []
        tab.browserData?.setCustomUserScript(scripts: scriptTypes)
      }

      // Brave Search logic.

      if requestInfo.isMainFrame,
        BraveSearchManager.isValidURL(requestURL)
      {
        // We fetch cookies to determine if backup search was enabled on the website.
        let cookies = await tab.configuration.websiteDataStore.httpCookieStore.allCookies()
        tab.braveSearchManager = BraveSearchManager(
          url: requestURL,
          cookies: cookies
        )

        let isAdBlockModeAggressive =
          tab.braveShieldsHelper?.shieldLevel(
            for: requestURL,
            considerAllShieldsOption: true
          ).isAggressive ?? true

        if BraveSearchResultAdManager.shouldTriggerSearchResultAdClickedEvent(
          requestURL,
          isPrivateBrowsing: isPrivateBrowsing,
          isAggressiveAdsBlocking: isAdBlockModeAggressive
        ) {
          // Ensure the webView is not a link preview popup.
          if self.presentedViewController == nil {
            let showSearchResultAdClickedPrivacyNotice =
              rewards.ads.shouldShowSearchResultAdClickedInfoBar()
            BraveSearchResultAdManager.maybeTriggerSearchResultAdClickedEvent(
              requestURL,
              rewards: rewards,
              completion: { [weak self] success in
                guard let self, success, showSearchResultAdClickedPrivacyNotice else {
                  return
                }
                let searchResultClickedInfobar = SearchResultAdClickedInfoBar(
                  tabManager: self.tabManager
                )
                self.show(toast: searchResultClickedInfobar, duration: nil)
              }
            )
          }
        } else {
          // The Brave-Search-Ads header should be added with a negative value when all
          // of the following conditions are met:
          //   - The current tab is not a Private tab
          //   - Brave Rewards is enabled.
          //   - The "Search Ads" is opted-out.
          //   - The requested URL host is one of the Brave Search domains.
          if !isPrivateBrowsing && rewards.isEnabled
            && !rewards.ads.isOptedInToSearchResultAds()
            && request.allHTTPHeaderFields?["Brave-Search-Ads"] == nil
          {
            var modifiedRequest = URLRequest(url: requestURL)
            modifiedRequest.setValue("?0", forHTTPHeaderField: "Brave-Search-Ads")
            tab.loadRequest(modifiedRequest)
            return .cancel
          }

          tab.braveSearchResultAdManager = BraveSearchResultAdManager(
            url: requestURL,
            rewards: rewards,
            isPrivateBrowsing: isPrivateBrowsing,
            isAggressiveAdsBlocking: isAdBlockModeAggressive
          )
        }

        if let braveSearchManager = tab.braveSearchManager {
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
                tab.injectResults()
              }
            }
          }
        }
      } else {
        tab.braveSearchManager = nil
        tab.braveSearchResultAdManager = nil
      }
    }

    // This is the normal case, opening a http or https url, which we handle by loading them in this WKWebView. We
    // always allow this. Additionally, data URIs are also handled just like normal web pages.

    if ["http", "https", "data", "blob", "file"].contains(requestURL.scheme) {
      pendingRequests[requestURL.absoluteString] = request

      if requestInfo.isMainFrame,
        let etldP1 = requestURL.baseDomain,
        tab.proceedAnywaysDomainList?.contains(etldP1) == false
      {
        let shieldLevel =
          tab.braveShieldsHelper?.shieldLevel(
            for: requestURL,
            considerAllShieldsOption: true
          ) ?? .standard

        let shouldBlock = await AdBlockGroupsManager.shared.shouldBlock(
          requestURL: requestURL,
          sourceURL: requestURL,
          resourceType: .document,
          isAdBlockEnabled: shieldLevel.isEnabled,
          isAdBlockModeAggressive: shieldLevel.isAggressive
        )

        if shouldBlock, let url = requestURL.encodeEmbeddedInternalURL(for: .blocked) {
          let request = PrivilegedRequest(url: url) as URLRequest
          tab.loadRequest(request)
          return .cancel
        }
      }

      // Cookie Blocking code below
      tab.browserData?.setScript(
        script: .cookieBlocking,
        enabled: Preferences.Privacy.blockAllCookies.value
      )

      // Reset the block alert bool on new host.
      if let newHost: String = requestURL.host, let oldHost: String = tab.visibleURL?.host,
        newHost != oldHost
      {
        self.tabManager.selectedTab?.alertShownCount = 0
        self.tabManager.selectedTab?.blockAllAlerts = false
      }

      return .allow
    }

    if requestURL.scheme?.contains("brave") == true || requestURL.scheme?.contains("chrome") == true
    {
      // brave://account should not be treated as a regular WebUI page.
      // It is part of the Settings UI and is intended to be opened only
      // from within Settings – much like you cannot open a new tab
      // and directly navigate to a settings screen (e.g. for Page Zoom).
      return requestURL.host == "account" ? .cancel : .allow
    }

    // Standard schemes are handled in previous if-case.
    // This check handles custom app schemes to open external apps.
    // Our own 'brave' scheme does not require the switch-app prompt.
    // Do not allow opening external URLs from child tabs
    let shouldOpen = await handleExternalURL(
      requestURL,
      tab: tab,
      requestInfo: requestInfo
    )
    let isSyntheticClick = !requestInfo.isUserInitiated

    // Do not show error message for JS navigated links or redirect
    // as it's not the result of a user action.
    if let tabData = tab.browserData, !shouldOpen,
      requestInfo.navigationType == .linkActivated && !isSyntheticClick
    {
      if self.presentedViewController == nil && self.presentingViewController == nil
        && !tabData.isExternalAppAlertPresented && !tabData.isExternalAppAlertSuppressed
      {
        return await withCheckedContinuation { continuation in
          // This alert does not need to be a BrowserAlertController because we return a policy
          // without waiting for user action
          let alert = UIAlertController(
            title: Strings.unableToOpenURLErrorTitle,
            message: Strings.unableToOpenURLError,
            preferredStyle: .alert
          )
          alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
          self.present(alert, animated: true) {
            continuation.resume(returning: shouldOpen ? .allow : .cancel)
          }
        }
      }
    }

    return shouldOpen ? .allow : .cancel
  }
}

extension BrowserViewController {
  // Recognize an Apple Maps URL. This will trigger the native app. But only if a search query is present.
  // Otherwise it could just be a visit to a regular page on maps.apple.com.
  // Exchaging https/https scheme with maps in order to open URLS properly on Apple Maps
  fileprivate func isAppleMapsURL(_ url: URL) -> (enabled: Bool, url: URL)? {
    if url.scheme == "http" || url.scheme == "https" {
      if url.host == "maps.apple.com" && url.query != nil {
        guard var urlComponents = URLComponents(url: url, resolvingAgainstBaseURL: false) else {
          return nil
        }
        urlComponents.scheme = "maps"

        if let url = urlComponents.url {
          return (true, url)
        }
        return nil
      }
    }
    return (false, url)
  }

  // Recognize a iTunes Store URL. These all trigger the native apps. Note that appstore.com and phobos.apple.com
  // used to be in this list. I have removed them because they now redirect to itunes.apple.com. If we special case
  // them then iOS will actually first open Safari, which then redirects to the app store. This works but it will
  // leave a 'Back to Safari' button in the status bar, which we do not want.
  fileprivate func isStoreURL(_ url: URL) -> Bool {
    let isStoreScheme = ["itms-apps", "itms-appss", "itmss"].contains(url.scheme)
    if isStoreScheme {
      return true
    }

    let isHttpScheme = ["http", "https"].contains(url.scheme)
    let isAppStoreHost = ["itunes.apple.com", "apps.apple.com", "appsto.re"].contains(url.host)
    return isHttpScheme && isAppStoreHost
  }

  /// Get a possible redirect request from debouncing or query param stripping
  func getInternalRedirect(
    from request: URLRequest,
    isMainFrame: Bool,
    in tab: some TabState,
    isAdBlockEnabled: Bool
  ) -> URLRequest? {
    guard let requestURL = request.url else { return nil }

    // For main frame only and if shields are enabled
    guard requestURL.isWebPage(includeDataURIs: false),
      isAdBlockEnabled,
      isMainFrame
    else { return nil }

    // Handle Debounce
    // Only if the site (etld+1) changes
    // We also only handle `http` and `https` requests
    // Lets get the redirect chain.
    // Then we simply get all elements up until the user allows us to redirect
    // (i.e. appropriate settings are enabled for that redirect rule)
    if let debounceService = DebounceServiceFactory.get(privateMode: tab.isPrivate),
      debounceService.isEnabled,
      let lastCommittedURL = tab.lastCommittedURL,
      lastCommittedURL.baseDomain != requestURL.baseDomain
    {
      if let redirectURL = debounceService.debounce(url: requestURL) {
        // For now we only allow the `Referer`. The browser will add other headers during navigation.
        var modifiedRequest = URLRequest(url: redirectURL)

        // Also strip query params if debouncing
        modifiedRequest =
          modifiedRequest.stripQueryParams(
            initiatorURL: tab.lastCommittedURL,
            redirectSourceURL: requestURL,
            isInternalRedirect: false
          ) ?? modifiedRequest

        for (headerKey, headerValue) in request.allHTTPHeaderFields ?? [:] {
          guard headerKey == "Referer" else { continue }
          modifiedRequest.setValue(headerValue, forHTTPHeaderField: headerKey)
        }

        Logger.module.debug(
          "Debouncing `\(requestURL.absoluteString)`"
        )

        return modifiedRequest
      }
    }

    // Handle query param stripping
    if let request = request.stripQueryParams(
      initiatorURL: tab.lastCommittedURL,
      redirectSourceURL: tab.redirectSourceURL,
      isInternalRedirect: tab.isInternalRedirect == true
    ) {
      Logger.module.debug(
        "Stripping query params for `\(requestURL.absoluteString)`"
      )
      return request
    }

    // HTTPS by Default
    if shouldUpgradeToHttps(url: requestURL, isPrivate: tab.isPrivate),
      var urlComponents = URLComponents(url: requestURL, resolvingAgainstBaseURL: true)
    {
      if let existingUpgradeRequestURL = tab.upgradedHTTPSRequest?.url,
        existingUpgradeRequestURL == requestURL
      {
        // if server redirected https -> http, https load never fails.
        // `webView(_:decidePolicyFor:preferences:)` will be called before
        // `webView(_:didReceiveServerRedirectForProvisionalNavigation:)`
        // so we must prevent upgrade loop.
        return handleInvalidHTTPSUpgrade(tab: tab, responseURL: requestURL)
      }
      // Attempt to upgrade to HTTPS
      urlComponents.scheme = "https"
      if let upgradedURL = urlComponents.url {
        Logger.module.debug(
          "Upgrading `\(requestURL.absoluteString)` to HTTPS"
        )
        tab.upgradedHTTPSRequest = request
        tab.upgradeHTTPSTimeoutTimer?.invalidate()
        var modifiedRequest = request
        modifiedRequest.url = upgradedURL

        tab.upgradeHTTPSTimeoutTimer = Timer.scheduledTimer(
          withTimeInterval: 3.seconds,
          repeats: false,
          block: { [weak tab, weak self] timer in
            guard let self, let tab else { return }
            if let url = modifiedRequest.url,
              let request = handleInvalidHTTPSUpgrade(tab: tab, responseURL: url)
            {
              tab.stopLoading()
              tab.loadRequest(request)
            }
          }
        )
        return modifiedRequest
      }
    }

    return nil
  }

  /// Determines if the given url should be upgraded from http to https.
  fileprivate func shouldUpgradeToHttps(url: URL, isPrivate: Bool) -> Bool {
    guard FeatureList.kBraveHttpsByDefault.enabled,
      let httpUpgradeService = HttpsUpgradeServiceFactory.get(privateMode: isPrivate),
      url.scheme == "http", let host = url.host
    else {
      return false
    }
    let isInUserAllowList = httpUpgradeService.isHttpAllowed(forHost: host)
    let shouldUpgrade: Bool
    switch Preferences.Shields.httpsUpgradeLevel {
    case .strict:
      // Always upgrade for Strict HTTPS upgrade unless previously allowed by user.
      shouldUpgrade = !isInUserAllowList
    case .standard:
      // Upgrade for Standard HTTPS upgrade if host is not on the exceptions list and not previously allowed by user.
      shouldUpgrade =
        braveCore.httpsUpgradeExceptionsService.canUpgradeToHTTPS(for: url)
        && !isInUserAllowList
    case .disabled:
      shouldUpgrade = false
    }
    return shouldUpgrade
  }

  /// Upon an invalid response, check that we need to roll back any HTTPS upgrade
  /// or show the interstitial page
  func handleInvalidHTTPSUpgrade(tab: some TabState, responseURL: URL) -> URLRequest? {
    // Handle invalid upgrade to https
    guard let originalRequest = tab.upgradedHTTPSRequest,
      let originalURL = originalRequest.url,
      responseURL.baseDomain == originalURL.baseDomain
    else {
      return nil
    }

    if Preferences.Shields.httpsUpgradeLevel.isStrict,
      let url = originalURL.encodeEmbeddedInternalURL(for: .httpBlocked)
    {
      Logger.module.debug(
        "Show http blocked interstitial for `\(originalURL.absoluteString)`"
      )

      let request = PrivilegedRequest(url: url) as URLRequest
      return request
    } else {
      Logger.module.debug(
        "Revert HTTPS upgrade for `\(originalURL.absoluteString)`"
      )

      tab.upgradedHTTPSRequest = nil
      tab.upgradeHTTPSTimeoutTimer?.invalidate()
      tab.upgradeHTTPSTimeoutTimer = nil
      if let httpsUpgradeService = HttpsUpgradeServiceFactory.get(privateMode: tab.isPrivate),
        let host = originalURL.host
      {
        httpsUpgradeService.allowHttp(forHost: host)
      }
      return originalRequest
    }
  }

  func handleExternalURL(
    _ url: URL,
    tab: some TabState,
    requestInfo: WebRequestInfo
  ) async -> Bool {
    // Do not open external links for child tabs automatically
    // The user must tap on the link to open it.
    if tab.opener != nil && requestInfo.navigationType != .linkActivated {
      return false
    }

    // If the request is cross origin frame, block it.
    // If the request is cross origin window, block it.
    if requestInfo.isCrossOriginFrame || requestInfo.isCrossOriginWindow {
      return false
    }

    // If the request is from a sub-frame and not user-initiated, block it.
    if !requestInfo.isMainFrame && !requestInfo.isUserInitiated {
      return false
    }

    // Check if the current url of the caller has changed
    if let domain = tab.visibleURL?.baseDomain,
      domain != tab.externalAppURLDomain
    {
      tab.externalAppAlertCounter = 0
      tab.isExternalAppAlertSuppressed = false
    }

    tab.externalAppURLDomain = tab.visibleURL?.baseDomain

    // Do not try to present over existing warning
    if let tabData = tab.browserData,
      tabData.isExternalAppAlertPresented || tabData.isExternalAppAlertSuppressed
    {
      return false
    }

    // External dialog should not be shown for non-active tabs #6687 - #7835
    let isVisibleTab = tab.browserData?.isTabVisible() == true

    if !isVisibleTab {
      return false
    }

    var alertTitle = Strings.openExternalAppURLGenericTitle

    if requestInfo.isMainFrame {
      if case let origin = URLOrigin(url: url), !origin.isOpaque {
        let displayHost =
          "\(origin.scheme)://\(origin.host):\(origin.port)"
        alertTitle = String(format: Strings.openExternalAppURLTitle, displayHost)
      } else if let displayHost = tab.visibleURL?.withoutWWW.host {
        alertTitle = String(format: Strings.openExternalAppURLTitle, displayHost)
      }
    }

    // Handling condition when Tab is empty when handling an external URL we should remove the tab once the user decides
    let removeTabIfEmpty = { [weak self] in
      if tab.visibleURL == nil {
        self?.tabManager.removeTab(tab)
      }
    }

    // Show the external sceheme invoke alert
    @MainActor
    func showExternalSchemeAlert(
      for tab: some TabState,
      isSuppressActive: Bool,
      openedURLCompletionHandler: @escaping (Bool) -> Void
    ) {
      // Check if active controller is bvc otherwise do not show show external sceheme alerts
      guard shouldShowExternalSchemeAlert() else {
        openedURLCompletionHandler(false)
        return
      }

      view.endEditing(true)
      tab.isExternalAppAlertPresented = true

      let popup = AlertPopupView(
        imageView: nil,
        title: alertTitle,
        message: String(format: Strings.openExternalAppURLMessage, url.relativeString),
        titleWeight: .semibold,
        titleSize: 21
      )

      tab.externalAppPopup = popup

      if isSuppressActive {
        popup.addButton(title: Strings.suppressAlertsActionTitle, type: .destructive) {
          [weak tab] () -> PopupViewDismissType in
          openedURLCompletionHandler(false)
          tab?.isExternalAppAlertSuppressed = true
          return .flyDown
        }
      } else {
        popup.addButton(title: Strings.openExternalAppURLDontAllow) {
          [weak tab] () -> PopupViewDismissType in
          openedURLCompletionHandler(false)
          removeTabIfEmpty()
          tab?.isExternalAppAlertPresented = false
          return .flyDown
        }
      }
      popup.addButton(title: Strings.openExternalAppURLAllow, type: .primary) {
        [weak tab] () -> PopupViewDismissType in
        UIApplication.shared.open(url, options: [:]) { didOpen in
          openedURLCompletionHandler(!didOpen)
        }
        removeTabIfEmpty()
        tab?.isExternalAppAlertPresented = false
        return .flyDown
      }
      popup.showWithType(showType: .flyUp)
    }

    func shouldShowExternalSchemeAlert() -> Bool {
      guard let rootVC = currentScene?.browserViewController else {
        return false
      }

      func topViewController(startingFrom viewController: UIViewController) -> UIViewController {
        var top = viewController
        if let navigationController = top as? UINavigationController,
          let vc = navigationController.visibleViewController
        {
          return topViewController(startingFrom: vc)
        }
        if let tabController = top as? UITabBarController,
          let vc = tabController.selectedViewController
        {
          return topViewController(startingFrom: vc)
        }
        while let next = top.presentedViewController {
          top = next
        }
        return top
      }

      let isTopController = self == topViewController(startingFrom: rootVC)
      let isTopWindow = view.window?.isKeyWindow == true
      return isTopController && isTopWindow
    }

    tab.browserData?.externalAppAlertCounter += 1

    return await withTaskCancellationHandler {
      return await withCheckedContinuation { [weak tab] continuation in
        guard let tab else {
          continuation.resume(returning: false)
          return
        }
        tab.externalAppPopupContinuation = continuation
        showExternalSchemeAlert(for: tab, isSuppressActive: tab.externalAppAlertCounter ?? 0 > 2) {
          [weak tab] in
          tab?.externalAppPopupContinuation = nil
          continuation.resume(with: .success($0))
        }
      }
    } onCancel: { [weak tab] in
      tab?.externalAppPopupContinuation?.resume(with: .success(false))
      tab?.externalAppPopupContinuation = nil
    }
  }

  func recordFinishedPageLoadP3A() {
    var storage = P3ATimedStorage<Int>.pagesLoadedStorage
    storage.add(value: 1, to: Date())
    UmaHistogramRecordValueToBucket(
      "Brave.Core.PagesLoaded",
      buckets: [
        0,
        .r(1...10),
        .r(11...50),
        .r(51...100),
        .r(101...500),
        .r(501...1000),
        .r(1001...),
      ],
      value: storage.combinedValue
    )
  }
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var pagesLoadedStorage: Self { .init(name: "paged-loaded", lifetimeInDays: 7) }
}

extension URLRequest {
  /// Strip any query params in the request and return a new request if anything is stripped.
  ///
  /// The `isInternalRedirect` is a true value whenever we redirected the user for debouncing or query-stripping.
  /// It's an optimization because we assume that we stripped and debounced the user fully so there should be no further stripping on the next iteration.
  ///
  /// - Parameters:
  ///   - initiatorURL: The url page the user is coming from before any redirects
  ///   - redirectSourceURL: The last redirect url that happened (the true page the user is coming from)
  ///   - isInternalRedirect: Identifies if we have internally redirected or not. More info in the description
  /// - Returns: A modified request if any stripping is to occur.
  fileprivate func stripQueryParams(
    initiatorURL: URL?,
    redirectSourceURL: URL?,
    isInternalRedirect: Bool
  ) -> URLRequest? {
    guard let requestURL = url,
      let requestMethod = httpMethod
    else { return nil }

    guard
      let strippedURL = (requestURL as NSURL).applyingQueryFilter(
        initiatorURL: initiatorURL,
        redirectSourceURL: redirectSourceURL,
        requestMethod: requestMethod,
        isInternalRedirect: isInternalRedirect
      )
    else { return nil }

    var modifiedRequest = self
    modifiedRequest.url = strippedURL
    return modifiedRequest
  }

  /// Allow local requests only if the request is privileged.
  /// If the request is internal or unprivileged, we should deny it.
  var isInternalUnprivileged: Bool {
    guard let url = url else {
      return true
    }

    if let url = InternalURL(url) {
      return !url.isAuthorized
    } else {
      return false
    }
  }
}
