// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CertificateUtilities
import Foundation
import OSLog
import Storage
import WebKit

class TabWKNavigationHandler: NSObject, WKNavigationDelegate {
  weak var tab: WebKitTabState?

  private var downloadHandlers: Set<TabWKDownloadHandler> = []
  private var sslPinningError: Error?
  private var pendingMIMEType: String?

  init(tab: WebKitTabState) {
    self.tab = tab
  }

  private var shouldDownloadNavigationResponse: Bool = false

  func webView(
    _ webView: WKWebView,
    didStartProvisionalNavigation navigation: WKNavigation
  ) {
    guard let tab else { return }
    sslPinningError = nil

    // Reset redirect chain
    tab.redirectChain = []
    if let url = webView.url {
      tab.redirectChain.append(url)
    }

    tab.didStartNavigation()
  }

  private func defaultAllowPolicy(
    for tab: some TabState,
    request: URLRequest
  ) -> WKNavigationActionPolicy {
    if let delegate = tab.delegate,
      delegate.tab(tab, shouldBlockUniversalLinksForRequest: request)
    {
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
  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationAction: WKNavigationAction,
    preferences: WKWebpagePreferences
  ) async -> (WKNavigationActionPolicy, WKWebpagePreferences) {
    guard let tab,
      let requestURL = navigationAction.request.url
    else {
      return (.cancel, preferences)
    }
    let isMainFrame =
      navigationAction.targetFrame?.isMainFrame ?? navigationAction.sourceFrame.isMainFrame

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

    let policy =
      await tab.shouldAllowRequest(
        navigationAction.request,
        requestInfo: .init(
          navigationType: navigationType,
          isMainFrame: isMainFrame,
          isNewWindow: navigationAction.targetFrame == nil,
          isUserInitiated: !navigationAction.isSyntheticClick
        )
      )

    if policy == .cancel {
      return (.cancel, preferences)
    }

    self.shouldDownloadNavigationResponse = false

    if ["http", "https", "data", "blob", "file"].contains(requestURL.scheme) {
      // Handle updating the user agent
      if navigationAction.targetFrame?.isMainFrame == true {
        let userAgentType = tab.userAgentTypeForURL(requestURL)
        webView.customUserAgent = tab.delegate?.tab(
          tab,
          userAgentForType: userAgentType,
          request: navigationAction.request
        )
      }

      // Handle blocking JS
      if let delegate = tab.delegate,
        delegate.tab(tab, shouldBlockJavaScriptForRequest: navigationAction.request)
      {
        preferences.allowsContentJavaScript = false
      }

      if #unavailable(iOS 18.0) {
        // In prior versions of iOS, `WKWebpagePreferences.allowsContentJavaScript` does not work
        // correctly in all cases: https://github.com/brave/brave-ios/issues/8585
        webView.configuration.preferences.javaScriptEnabled = preferences.allowsContentJavaScript
      }

      // Downloads
      self.shouldDownloadNavigationResponse = navigationAction.shouldPerformDownload
    }

    return (defaultAllowPolicy(for: tab, request: navigationAction.request), preferences)
  }

  @MainActor
  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationResponse: WKNavigationResponse
  ) async -> WKNavigationResponsePolicy {
    guard let tab else { return .cancel }
    let response = navigationResponse.response

    let shouldRenderResponse: Bool = {
      if !navigationResponse.canShowMIMEType {
        return false
      }
      if shouldDownloadNavigationResponse {
        return false
      }
      let octetStream = "application/octet-stream"
      let mimeType = response.mimeType ?? octetStream
      if let contentDisposition = (response as? HTTPURLResponse)?.value(
        forHTTPHeaderField: "Content-Disposition"
      ), contentDisposition.starts(with: "attachment") {
        return false
      }
      return mimeType != octetStream
    }()

    if navigationResponse.isForMainFrame {
      pendingMIMEType = response.mimeType
    }

    if shouldRenderResponse {
      let decision: WebPolicyDecision = await tab.shouldAllowResponse(
        navigationResponse.response,
        responseInfo: .init(isForMainFrame: navigationResponse.isForMainFrame)
      )
      return decision == .cancel ? .cancel : .allow
    }

    return .download
  }

  func webView(
    _ webView: WKWebView,
    navigationAction: WKNavigationAction,
    didBecome download: WKDownload
  ) {
    Logger.module.error(
      "ERROR: Should Never download NavigationAction since we never return .download from decidePolicyForAction."
    )
  }

  func webView(
    _ webView: WKWebView,
    navigationResponse: WKNavigationResponse,
    didBecome download: WKDownload
  ) {
    var handler: TabWKDownloadHandler?
    handler = TabWKDownloadHandler(
      didCreateDownload: { [weak tab] download in
        guard let tab else { return }
        tab.downloadDelegate?.tab(tab, didCreateDownload: download)
      },
      didFinishDownload: { [weak self, weak handler] download, error in
        guard let self else { return }
        if let handler {
          downloadHandlers.remove(handler)
        }
        if let tab = self.tab, let downloadDelegate = tab.downloadDelegate {
          downloadDelegate.tab(tab, didFinishDownload: download, error: error)
        }
      }
    )
    download.delegate = handler
    downloadHandlers.insert(handler!)
  }

  @MainActor
  func webView(
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

      if let cert = cert, let certStore = tab?.certificateStore,
        certStore.containsCertificate(cert, forOrigin: origin)
      {
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
        sslPinningError = error

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
      let tab
    else {
      return (.performDefaultHandling, nil)
    }

    let resolvedCredential = await tab.delegate?.tab(
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

  func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    guard let tab else { return }

    // Set the committed url which will also set tab.visibleURL
    tab.lastCommittedURL = webView.url
    tab.isRestoring = false
    tab.contentsMimeType = pendingMIMEType

    pendingMIMEType = nil

    tab.didCommitNavigation()
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    tab?.didFinishNavigation()
  }

  func webViewWebContentProcessDidTerminate(_ webView: WKWebView) {
    tab?.renderProcessTerminated()
  }

  func webView(
    _ webView: WKWebView,
    didReceiveServerRedirectForProvisionalNavigation navigation: WKNavigation!
  ) {
    guard let tab, let url = webView.url else { return }
    tab.redirectChain.append(url)
    tab.didRedirectNavigation()
  }

  /// Invoked when an error occurs while starting to load data for the main frame.
  func webView(
    _ webView: WKWebView,
    didFailProvisionalNavigation navigation: WKNavigation!,
    withError error: Error
  ) {
    guard let tab else { return }

    var error = error as NSError
    if error.domain == "WebKitErrorDomain" {
      if error.code == 102 {
        // Ignore the "Frame load interrupted" error that is triggered when we cancel a request
        // to open an external application and hand it over to UIApplication.openURL(). The result
        // will be that we switch to the external app, for example the app store, while keeping the
        // original web page in the tab instead of replacing it with an error page.
        return
      }
    }

    if let sslPinningError = sslPinningError {
      error = sslPinningError as NSError
    }

    tab.didFailNavigation(with: error)
  }

  func webView(
    _ webView: WKWebView,
    shouldAllowDeprecatedTLSFor challenge: URLAuthenticationChallenge
  ) async -> Bool {
    return false
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
