// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import CertificateUtilities
import Data
import Foundation
import Preferences
import Shared
import WebKit
import os.log

extension BrowserViewController: WKNavigationDelegate {
  public func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!)
  {
    guard let tab = tabManager[webView] else { return }
    self.tabDidStartWebViewNavigation(tab)
  }

  private func defaultAllowPolicy(
    for navigationAction: WKNavigationAction
  ) -> WKNavigationActionPolicy {
    let isPrivateBrowsing = tabManager.privateBrowsingManager.isPrivateBrowsing
    if shouldBlockUniversalLinksFor(
      request: navigationAction.request,
      isPrivateBrowsing: isPrivateBrowsing
    ) {
      // Stop Brave from opening universal links by using the private enum value
      // `_WKNavigationActionPolicyAllowWithoutTryingAppLink` which is defined here:
      // https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/Cocoa/WKNavigationDelegatePrivate.h#L62
      let allowDecision =
        WKNavigationActionPolicy(rawValue: WKNavigationActionPolicy.allow.rawValue + 2) ?? .allow
      return allowDecision
    }
    return .allow
  }

  @MainActor
  public func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationAction: WKNavigationAction,
    preferences: WKWebpagePreferences
  ) async -> (WKNavigationActionPolicy, WKWebpagePreferences) {
    guard let tab = tabManager[webView],
      let requestURL = navigationAction.request.url
    else {
      return (.cancel, preferences)
    }
    let isMainFrame =
      navigationAction.targetFrame?.isMainFrame ?? navigationAction.sourceFrame.isMainFrame

    // Setup braveSearchManager on the tab as it requires accessing WKWebView cookies
    if isMainFrame, BraveSearchManager.isValidURL(requestURL) {
      // We fetch cookies to determine if backup search was enabled on the website.

      let cookies = await webView.configuration.websiteDataStore.httpCookieStore.allCookies()
      tab.braveSearchManager = BraveSearchManager(
        url: requestURL,
        cookies: cookies
      )
    } else {
      tab.braveSearchManager = nil
    }

    let navigationType: WebNavigationType =
      switch navigationAction.navigationType {
      case .backForward: .backForward
      case .formSubmitted: .formSubmitted
      case .formResubmitted: .formResubmitted
      case .linkActivated: .linkActivated
      case .other: .other
      case .reload: .reload
      @unknown default: .other
      }

    let isSyntheticClick =
      navigationAction.responds(to: Selector(("_syntheticClickType")))
      && navigationAction.value(forKey: "syntheticClickType") as? Int == 0

    let policy = await self.tab(
      tab,
      shouldAllowRequest: navigationAction.request,
      requestInfo: .init(
        navigationType: navigationType,
        isMainFrame: isMainFrame,
        isNewWindow: navigationAction.targetFrame == nil,
        isUserInitiated: !isSyntheticClick
      )
    )

    if policy == .cancel {
      return (.cancel, preferences)
    }

    if ["http", "https", "data", "blob", "file"].contains(requestURL.scheme) {
      // Handle updating the user agent
      if navigationAction.targetFrame?.isMainFrame == true {
        tab.updateUserAgent(webView, newURL: requestURL)
      }

      // Handle blocking JS
      let documentTargetURL = navigationAction.request.mainDocumentURL ?? requestURL
      let domainForShields = Domain.getOrCreate(
        forUrl: documentTargetURL,
        persistent: !privateBrowsingManager.isPrivateBrowsing
      )
      let isScriptsEnabled = !domainForShields.isShieldExpected(
        .noScript,
        considerAllShieldsOption: true
      )

      // Due to a bug in iOS WKWebpagePreferences.allowsContentJavaScript does NOT work!
      // https://github.com/brave/brave-ios/issues/8585
      //
      // However, the deprecated API WKWebViewConfiguration.preferences.javaScriptEnabled DOES work!
      // Even though `configuration` is @NSCopying, somehow this actually updates the preferences LIVE!!
      // This follows the same behaviour as Safari
      //
      // - Brandon T.
      //
      preferences.allowsContentJavaScript = isScriptsEnabled
      webView.configuration.preferences.javaScriptEnabled = isScriptsEnabled

      // Downloads
      self.shouldDownloadNavigationResponse = navigationAction.shouldPerformDownload
    }

    return (defaultAllowPolicy(for: navigationAction), preferences)
  }

  @MainActor
  public func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationResponse: WKNavigationResponse
  ) async -> WKNavigationResponsePolicy {
    guard let tab = tabManager[webView] else { return .cancel }
    let responseURL = navigationResponse.response.url
    let request = responseURL.flatMap { pendingRequests[$0.absoluteString] }
    let response = navigationResponse.response
    defer {
      if let responseURL {
        pendingRequests.removeValue(forKey: responseURL.absoluteString)
      }
    }
    let decision: WebPolicyDecision = await self.tab(
      tab,
      shouldAllowResponse: navigationResponse.response,
      responseInfo: .init(isForMainFrame: navigationResponse.isForMainFrame)
    )

    if decision == .cancel {
      return .cancel
    }

    /// WebKit Download handling

    // We can only show this content in the web view if this web view is not pending
    // download via the context menu.
    let canShowInWebView = navigationResponse.canShowMIMEType

    // If the response has the attachment content-disposition, download it
    let mimeType = response.mimeType ?? MIMEType.octetStream
    let isAttachment =
      (response as? HTTPURLResponse)?.value(
        forHTTPHeaderField: "Content-Disposition"
      )?.starts(with: "attachment") ?? (mimeType == MIMEType.octetStream)

    if isAttachment {
      return .download
    }

    if response.mimeType == MIMEType.passbook {
      return .download
    }

    // Check if this response should be handed off to Passbook.
    if shouldDownloadNavigationResponse {
      shouldDownloadNavigationResponse = false
      return .download
    }

    if canShowInWebView {
      return .allow
    }

    // Check if this response should be downloaded.
    let cookieStore = webView.configuration.websiteDataStore.httpCookieStore
    if let downloadHelper = DownloadHelper(
      request: request,
      response: response,
      cookieStore: cookieStore,
      canShowInWebView: canShowInWebView
    ) {
      let downloadAlertAction: (HTTPDownload) -> Void = { [weak self] download in
        self?.downloadQueue.enqueue(download)
      }

      // Open our helper and cancel this response from the webview.
      if tab === tabManager.selectedTab,
        let downloadAlert = downloadHelper.downloadAlert(from: view, okAction: downloadAlertAction)
      {
        present(downloadAlert, animated: true, completion: nil)
      }

      return .cancel
    }

    // If none of our helpers are responsible for handling this response,
    // just let the webview handle it as normal.
    return .download
  }

  public func webView(
    _ webView: WKWebView,
    navigationAction: WKNavigationAction,
    didBecome download: WKDownload
  ) {
    Logger.module.error(
      "ERROR: Should Never download NavigationAction since we never return .download from decidePolicyForAction."
    )
  }

  public func webView(
    _ webView: WKWebView,
    navigationResponse: WKNavigationResponse,
    didBecome download: WKDownload
  ) {
    download.delegate = self
  }

  @MainActor
  public func webView(
    _ webView: WKWebView,
    respondTo challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {

    // If this is a certificate challenge, see if the certificate has previously been
    // accepted by the user.
    let host = challenge.protectionSpace.host
    let origin = "\(host):\(challenge.protectionSpace.port)"
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
      let trust = challenge.protectionSpace.serverTrust
    {

      let cert = await Task<SecCertificate?, Never>.detached {
        return (SecTrustCopyCertificateChain(trust) as? [SecCertificate])?.first
      }.value

      if let cert = cert, profile.certStore.containsCertificate(cert, forOrigin: origin) {
        return (.useCredential, URLCredential(trust: trust))
      }
    }

    // Certificate Pinning
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust {
      if let serverTrust = challenge.protectionSpace.serverTrust {
        let host = challenge.protectionSpace.host
        let port = challenge.protectionSpace.port

        let result = await BraveCertificateUtils.verifyTrust(serverTrust, host: host, port: port)
        let certificateChain = await Task<[SecCertificate], Never>.detached {
          return SecTrustCopyCertificateChain(serverTrust) as? [SecCertificate] ?? []
        }.value

        // Cert is valid and should be pinned
        if result == 0 {
          return (.useCredential, URLCredential(trust: serverTrust))
        }

        // Cert is valid and should not be pinned
        // Let the system handle it and we'll show an error if the system cannot validate it
        if result == Int32.min {
          // Cert is POTENTIALLY invalid and cannot be pinned

          // Let WebKit handle the request and validate the cert
          // This is the same as calling `BraveCertificateUtils.evaluateTrust` but with more error info provided by WebKit
          return (.performDefaultHandling, nil)
        }

        // Cert is invalid and cannot be pinned
        Logger.module.error("CERTIFICATE_INVALID")
        let errorCode = CFNetworkErrors.braveCertificatePinningFailed.rawValue

        let underlyingError = NSError(
          domain: kCFErrorDomainCFNetwork as String,
          code: Int(errorCode),
          userInfo: ["_kCFStreamErrorCodeKey": Int(errorCode)]
        )

        let error = NSError(
          domain: kCFErrorDomainCFNetwork as String,
          code: Int(errorCode),
          userInfo: [
            NSURLErrorFailingURLErrorKey: webView.url as Any,
            "NSErrorPeerCertificateChainKey": certificateChain,
            NSUnderlyingErrorKey: underlyingError,
          ]
        )

        // Handle the error later in `didFailProvisionalNavigation`
        tabManager[webView]?.sslPinningError = error

        return (.cancelAuthenticationChallenge, nil)
      }
    }

    // URLAuthenticationChallenge isn't Sendable atm
    let protectionSpace = challenge.protectionSpace
    let credential = challenge.proposedCredential
    let previousFailureCount = challenge.previousFailureCount

    guard
      protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic
        || protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest
        || protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM,
      let tab = tabManager[webView]
    else {
      return (.performDefaultHandling, nil)
    }

    let resolvedCredential = await self.tab(
      tab,
      didRequestHTTPAuthFor: protectionSpace,
      proposedCredential: credential,
      previousFailureCount: previousFailureCount
    )

    if let resolvedCredential {
      return (.useCredential, resolvedCredential)
    } else {
      return (.rejectProtectionSpace, nil)
    }
  }

  public func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    guard let tab = tabManager[webView] else { return }

    // Set the committed url which will also set tab.url
    tab.committedURL = webView.url

    // Server Trust and URL is also updated in didCommit
    // However, WebKit does NOT trigger the `serverTrust` observer when the URL changes, but the trust has not.
    // WebKit also does NOT trigger the `serverTrust` observer when the page is actually insecure (non-https).
    // So manually trigger it with the current trust.

    observeValue(
      forKeyPath: KVOConstants.serverTrust.keyPath,
      of: webView,
      change: [.newKey: webView.serverTrust as Any, .kindKey: 1],
      context: nil
    )

    self.tabDidCommitWebViewNavigation(tab)
  }

  public func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    guard let tab = tabManager[webView] else { return }
    self.tabDidFinishWebViewNavigation(tab)
  }

  public func webView(
    _ webView: WKWebView,
    didReceiveServerRedirectForProvisionalNavigation navigation: WKNavigation!
  ) {
    guard let tab = tabManager[webView], let url = webView.url else { return }
    tab.redirectChain.append(url)
  }

  /// Invoked when an error occurs while starting to load data for the main frame.
  public func webView(
    _ webView: WKWebView,
    didFailProvisionalNavigation navigation: WKNavigation!,
    withError error: Error
  ) {
    guard let tab = tabManager[webView] else { return }

    var error = error as NSError
    if error.domain == "WebKitErrorDomain" {
      if error.code == 102 {
        // Ignore the "Frame load interrupted" error that is triggered when we cancel a request
        // to open an external application and hand it over to UIApplication.openURL(). The result
        // will be that we switch to the external app, for example the app store, while keeping the
        // original web page in the tab instead of replacing it with an error page.
        return
      }
      if error.code == WKError.webContentProcessTerminated.rawValue {
        Logger.module.warning("WebContent process has crashed. Trying to reload to restart it.")
        tab.reload()
        return
      }
    }

    if self.tab(tab, didFailWebViewNavigationWithError: error) {
      // Handled in shared code
      return
    }

    if let sslPinningError = tab.sslPinningError {
      error = sslPinningError as NSError
    }

    if let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL {
      // Check for invalid upgrade to https
      if url.scheme == "https",  // verify failing url was https
        let response = handleInvalidHTTPSUpgrade(
          tab: tab,
          responseURL: url
        )
      {
        // load original or strict mode interstitial
        tab.loadRequest(response)
        return
      }

      ErrorPageHelper(certStore: profile.certStore).loadPage(error, forUrl: url, inWebView: webView)

      // Submitting same erroneous URL using toolbar will cause progress bar get stuck
      // Reseting the progress bar in case there is an error is necessary
      if tab == self.tabManager.selectedTab {
        self.topToolbar.hideProgressBar()
      }

      // If the local web server isn't working for some reason (Brave cellular data is
      // disabled in settings, for example), we'll fail to load the session restore URL.
      // We rely on loading that page to get the restore callback to reset the restoring
      // flag, so if we fail to load that page, reset it here.
      if InternalURL(url)?.aboutComponent == "sessionrestore" {
        tab.restoring = false
      }
    }
  }
}

// MARK: WKUIDelegate

extension BrowserViewController: WKUIDelegate {
  public func webView(
    _ webView: WKWebView,
    createWebViewWith configuration: WKWebViewConfiguration,
    for navigationAction: WKNavigationAction,
    windowFeatures: WKWindowFeatures
  ) -> WKWebView? {
    guard let parentTab = tabManager[webView] else { return nil }

    guard !navigationAction.request.isInternalUnprivileged,
      let navigationURL = navigationAction.request.url,
      navigationURL.shouldRequestBeOpenedAsPopup()
    else {
      print("Denying popup from request: \(navigationAction.request)")
      return nil
    }

    if let currentTab = tabManager.selectedTab {
      screenshotHelper.takeScreenshot(currentTab)
    }

    // If the page uses `window.open()` or `[target="_blank"]`, open the page in a new tab.
    // IMPORTANT!!: WebKit will perform the `URLRequest` automatically!! Attempting to do
    // the request here manually leads to incorrect results!!
    let newTab = tabManager.addPopupForParentTab(parentTab, configuration: configuration)

    newTab.url = URL(string: "about:blank")

    toolbarVisibilityViewModel.toolbarState = .expanded

    // Wait until WebKit starts the request before selecting the new tab, otherwise the tab manager may
    // restore it as if it was a dead tab.
    var observation: NSKeyValueObservation?
    observation = newTab.webView?.observe(
      \.url,
      changeHandler: { [weak self] webView, _ in
        _ = observation  // Silence write but not read warning
        observation = nil
        guard let self = self, let tab = self.tabManager[webView] else { return }

        // When a child tab is being selected, dismiss any popups on the parent tab
        tab.parent?.shownPromptAlert?.dismiss(animated: false)
        self.tabManager.selectTab(tab)
      }
    )

    return newTab.webView
  }

  public func webViewDidClose(_ webView: WKWebView) {
    guard let tab = tabManager[webView] else { return }
    tabWebViewDidClose(tab)
  }

  public func webView(
    _ webView: WKWebView,
    requestMediaCapturePermissionFor origin: WKSecurityOrigin,
    initiatedByFrame frame: WKFrameInfo,
    type: WKMediaCaptureType,
    decisionHandler: @escaping (WKPermissionDecision) -> Void
  ) {
    guard let tab = tabManager[webView], let captureType = WebMediaCaptureType(type) else {
      decisionHandler(.deny)
      return
    }

    if frame.securityOrigin.protocol.isEmpty || frame.securityOrigin.host.isEmpty {
      decisionHandler(.deny)
      return
    }

    let requestMediaPermissions: () -> Void = {
      Task {
        let permission = await self.tab(tab, requestMediaCapturePermissionsFor: captureType)
        decisionHandler(.init(permission))
      }
    }

    if webView.fullscreenState == .inFullscreen || webView.fullscreenState == .enteringFullscreen {
      webView.closeAllMediaPresentations {
        requestMediaPermissions()
      }
    } else {
      requestMediaPermissions()
    }
  }

  public func webView(
    _ webView: WKWebView,
    runJavaScriptAlertPanelWithMessage message: String,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping () -> Void
  ) {
    guard let tab = tabManager[webView], let url = frame.origin?.url else {
      completionHandler()
      return
    }
    Task {
      await self.tab(tab, runJavaScriptAlertPanelWithMessage: message, pageURL: url)
      completionHandler()
    }
  }

  public func webView(
    _ webView: WKWebView,
    runJavaScriptConfirmPanelWithMessage message: String,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping (Bool) -> Void
  ) {
    guard let tab = tabManager[webView], let url = frame.origin?.url else {
      completionHandler(false)
      return
    }
    Task {
      let result = await self.tab(tab, runJavaScriptConfirmPanelWithMessage: message, pageURL: url)
      completionHandler(result)
    }
  }

  public func webView(
    _ webView: WKWebView,
    runJavaScriptTextInputPanelWithPrompt prompt: String,
    defaultText: String?,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping (String?) -> Void
  ) {
    guard let tab = tabManager[webView], let url = frame.origin?.url else {
      completionHandler(nil)
      return
    }
    Task {
      let result = await self.tab(
        tab,
        runJavaScriptConfirmPanelWithPrompt: prompt,
        defaultText: defaultText,
        pageURL: url
      )
      completionHandler(result)
    }
  }

  @MainActor public func webView(
    _ webView: WKWebView,
    contextMenuConfigurationFor elementInfo: WKContextMenuElementInfo
  ) async -> UIContextMenuConfiguration? {
    guard let tab = tabManager[webView] else { return nil }
    return await self.tab(
      tab,
      contextMenuConfigurationForLinkURL: elementInfo.linkURL
    )
  }

  public func webView(
    _ webView: WKWebView,
    contextMenuForElement elementInfo: WKContextMenuElementInfo,
    willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating
  ) {
    guard let url = elementInfo.linkURL else { return }
    webView.load(URLRequest(url: url))
  }
}

extension BrowserViewController {
  /// Determines if the given url should be upgraded from http to https.
  func shouldUpgradeToHttps(url: URL, isPrivate: Bool) -> Bool {
    guard FeatureList.kBraveHttpsByDefault.enabled,
      let httpUpgradeService = HttpsUpgradeServiceFactory.get(privateMode: isPrivate),
      url.scheme == "http", let host = url.host
    else {
      return false
    }
    let isInUserAllowList = httpUpgradeService.isHttpAllowed(forHost: host)
    let shouldUpgrade: Bool
    switch ShieldPreferences.httpsUpgradeLevel {
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
  func handleInvalidHTTPSUpgrade(tab: Tab, responseURL: URL) -> URLRequest? {
    // Handle invalid upgrade to https
    guard let originalRequest = tab.upgradedHTTPSRequest,
      let originalURL = originalRequest.url,
      responseURL.baseDomain == originalURL.baseDomain
    else {
      return nil
    }

    if ShieldPreferences.httpsUpgradeLevel.isStrict,
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
}

extension WebMediaCaptureType {
  init?(_ captureType: WKMediaCaptureType) {
    switch captureType {
    case .camera:
      self = .camera
    case .microphone:
      self = .microphone
    case .cameraAndMicrophone:
      self = .cameraAndMicrophone
    @unknown default:
      return nil
    }
  }
}

extension WKPermissionDecision {
  init(_ permission: WebPermissionDecision) {
    switch permission {
    case .prompt:
      self = .prompt
    case .grant:
      self = .grant
    case .deny:
      self = .deny
    }
  }
}

extension WKFrameInfo {
  fileprivate var origin: URLOrigin? {
    if securityOrigin.protocol.isEmpty || securityOrigin.host.isEmpty {
      return nil
    }
    let defaultPort: UInt16 =
      switch securityOrigin.protocol {
      case "https": 443
      case "http": 80
      default: 0
      }
    return URLOrigin(
      scheme: securityOrigin.protocol,
      host: securityOrigin.host,
      port: securityOrigin.port == 0 ? defaultPort : UInt16(securityOrigin.port)
    )
  }
}

extension WKNavigationType: @retroactive CustomDebugStringConvertible {
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
