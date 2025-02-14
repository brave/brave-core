// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data

@MainActor
class ChromiumNavigationDelegateAdapter: NSObject, @preconcurrency BraveWebViewNavigationDelegate {
  private let tabManager: TabManager
  private let privateBrowsingManager: PrivateBrowsingManager
  private weak var delegate: TabWebDelegate?
  private weak var navigationDelegate: TabWebNavigationDelegate?
  private weak var policyDecider: TabWebPolicyDecider?

  init(
    tabManager: TabManager,
    privateBrowsingManager: PrivateBrowsingManager,
    delegate: TabWebNavigationDelegate?,
    policyDecider: TabWebPolicyDecider?
  ) {
    self.tabManager = tabManager
    self.privateBrowsingManager = privateBrowsingManager
    self.navigationDelegate = delegate
    self.policyDecider = policyDecider
  }

  public func webView(
    _ webView: CWVWebView,
    shouldBlockJavaScriptFor request: URLRequest
  ) -> Bool {
    let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
    guard let url = request.mainDocumentURL ?? request.url else {
      return false
    }
    let domainForShields = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
    let isScriptsBlocked = domainForShields.isShieldExpected(
      .noScript,
      considerAllShieldsOption: true
    )
    return isScriptsBlocked
  }

  public func webView(
    _ webView: CWVWebView,
    shouldBlockUniversalLinksFor request: URLRequest
  ) -> Bool {
    return shouldBlockUniversalLinksFor(
      request: request,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
  }

  public func webView(
    _ webView: CWVWebView,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential: URLCredential,
    completionHandler handler: @escaping (String?, String?) -> Void
  ) {
    Task {
      guard let tab = tabManager[webView] else {
        handler(nil, nil)
        return
      }
      let resolvedCredential = await delegate?.tab(
        tab,
        didRequestHTTPAuthFor: protectionSpace,
        proposedCredential: proposedCredential,
        previousFailureCount: 0
      )
      handler(resolvedCredential?.user, resolvedCredential?.password)
    }
  }

  public func webViewDidStartNavigation(_ webView: CWVWebView) {
    guard let tab = tabManager[webView] else { return }
    navigationDelegate?.tabDidStartWebViewNavigation(tab)
  }

  public func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationAction: CWVNavigationAction,
    decisionHandler: @escaping (CWVNavigationActionPolicy) -> Void
  ) {
    guard let tab = tabManager[webView] else { return }
    var navigationType: WebNavigationType = {
      if navigationAction.navigationType.contains(.forwardBack) {
        return .backForward
      }
      switch navigationAction.navigationType.coreType {
      case .link:
        return .linkActivated
      case .reload:
        return .reload
      case .formSubmit:
        return .formSubmitted
      default:
        return .other
      }
    }()

    Task { @MainActor in
      // Setup braveSearchManager on the tab as it requires accessing WKWebView cookies
      if let url = navigationAction.request.url,
        navigationAction.navigationType.isMainFrame, BraveSearchManager.isValidURL(url)
      {
        // We fetch cookies to determine if backup search was enabled on the website.
        //      let profile = self.profile
        let cookies = await webView.wkConfiguration.websiteDataStore.httpCookieStore.allCookies()
        tab.braveSearchManager = BraveSearchManager(
          url: url,
          cookies: cookies
        )
      } else {
        tab.braveSearchManager = nil
      }

      let policy = await policyDecider?.tab(
        tab,
        shouldAllowRequest: navigationAction.request,
        requestInfo: .init(
          navigationType: navigationType,
          isMainFrame: navigationAction.navigationType.isMainFrame,
          isNewWindow: navigationAction.navigationType == .newWindow,
          isUserInitiated: navigationAction.isUserInitiated
        )
      )
      decisionHandler(policy == .allow ? .allow : .cancel)
    }
  }

  public func webViewDidCommitNavigation(_ webView: CWVWebView) {
    guard let tab = tabManager[webView] else { return }
    // Set the committed url which will also set tab.url
    tab.committedURL = webView.lastCommittedURL
    navigationDelegate?.tabDidCommitWebViewNavigation(tab)
  }

  public func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationResponse: CWVNavigationResponse,
    decisionHandler: @escaping (CWVNavigationResponsePolicy) -> Void
  ) {
    guard let tab = tabManager[webView] else { return }
    Task { @MainActor in
      let policy = await self.policyDecider?.tab(
        tab,
        shouldAllowResponse: navigationResponse.response,
        responseInfo: .init(isForMainFrame: navigationResponse.isForMainFrame)
      )
      decisionHandler(policy == .allow ? .allow : .cancel)
    }
  }

  public func webViewDidFinishNavigation(_ webView: CWVWebView) {
    guard let tab = tabManager[webView] else { return }
    navigationDelegate?.tabDidFinishWebViewNavigation(tab)
  }

  public func webView(_ webView: CWVWebView, didFailNavigationWithError error: any Error) {
    guard let tab = tabManager[webView] else { return }
    _ = navigationDelegate?.tab(tab, didFailWebViewNavigationWithError: error)
  }

  public func webView(_ webView: CWVWebView, didRequestDownloadWith task: CWVDownloadTask) {

  }
}

@MainActor
class ChromiumUIDelegateAdapter: NSObject, @preconcurrency CWVUIDelegate {
  private let tabManager: TabManager
  private weak var delegate: TabWebDelegate?

  init(tabManager: TabManager, delegate: TabWebDelegate? = nil) {
    self.tabManager = tabManager
    self.delegate = delegate
  }

  func webView(
    _ webView: CWVWebView,
    createWebViewWith configuration: CWVWebViewConfiguration,
    for action: CWVNavigationAction
  ) -> CWVWebView? {
    guard let tab = tabManager[webView] else { return nil }

    guard !action.request.isInternalUnprivileged,
      let navigationURL = action.request.url,
      navigationURL.shouldRequestBeOpenedAsPopup()
    else {
      print("Denying popup from request: \(action.request)")
      return nil
    }

    // FIXME: Add Tab creation when creating a Tab with a CWVWebView is available
    return nil
  }

  func webViewDidClose(_ webView: CWVWebView) {
    guard let tab = tabManager[webView] else { return }
    delegate?.tabWebViewDidClose(tab)
  }

  func webView(
    _ webView: CWVWebView,
    contextMenuConfigurationFor element: CWVHTMLElement,
    completionHandler: @escaping (UIContextMenuConfiguration?) -> Void
  ) {
    guard let tab = tabManager[webView] else {
      completionHandler(nil)
      return
    }
    Task {
      let configuration = await delegate?.tab(
        tab,
        contextMenuConfigurationForLinkURL: element.hyperlink
      )
      completionHandler(configuration)
    }
  }

  func webView(
    _ webView: CWVWebView,
    requestMediaCapturePermissionFor type: CWVMediaCaptureType,
    decisionHandler: @escaping (CWVPermissionDecision) -> Void
  ) {
    guard let tab = tabManager[webView], let captureType = WebMediaCaptureType(type), let delegate
    else {
      decisionHandler(.prompt)
      return
    }
    Task {
      let permission = await delegate.tab(tab, requestMediaCapturePermissionsFor: captureType)
      decisionHandler(.init(permission))
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping () -> Void
  ) {
    guard let tab = tabManager[webView] else {
      completionHandler()
      return
    }
    Task {
      await delegate?.tab(tab, runJavaScriptAlertPanelWithMessage: message, pageURL: url)
      completionHandler()
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping (Bool) -> Void
  ) {
    guard let tab = tabManager[webView], let delegate else {
      completionHandler(false)
      return
    }
    Task {
      let result = await delegate.tab(
        tab,
        runJavaScriptConfirmPanelWithMessage: message,
        pageURL: url
      )
      completionHandler(result)
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptTextInputPanelWithPrompt prompt: String,
    defaultText: String,
    pageURL URL: URL,
    completionHandler: @escaping (String?) -> Void
  ) {
    guard let tab = tabManager[webView], let delegate else {
      completionHandler(nil)
      return
    }
    Task {
      let result = await delegate.tab(
        tab,
        runJavaScriptConfirmPanelWithPrompt: prompt,
        defaultText: defaultText,
        pageURL: URL
      )
      completionHandler(result)
    }
  }
}

extension WebMediaCaptureType {
  init?(_ mediaCaptureType: CWVMediaCaptureType) {
    switch mediaCaptureType {
    case .microphone:
      self = .microphone
    case .camera:
      self = .camera
    case .cameraAndMicrophone:
      self = .cameraAndMicrophone
    default:
      return nil
    }
  }
}

extension CWVPermissionDecision {
  init(_ decision: WebPermissionDecision) {
    switch decision {
    case .prompt:
      self = .prompt
    case .grant:
      self = .grant
    case .deny:
      self = .deny
    }
  }
}
