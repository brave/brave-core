// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

class TabCWVNavigationHandler: NSObject, BraveWebViewNavigationDelegate {
  private weak var tab: ChromiumTabState?

  init(tab: ChromiumTabState) {
    self.tab = tab
  }

  public func webView(
    _ webView: CWVWebView,
    shouldBlockJavaScriptFor request: URLRequest
  ) -> Bool {
    guard let tab, let delegate = tab.delegate else { return false }
    return delegate.tab(tab, shouldBlockJavaScriptForRequest: request)
  }

  public func webView(
    _ webView: CWVWebView,
    shouldBlockUniversalLinksFor request: URLRequest
  ) -> Bool {
    guard let tab, let delegate = tab.delegate else { return false }
    return delegate.tab(tab, shouldBlockUniversalLinksForRequest: request)
  }

  func webView(
    _ webView: CWVWebView,
    userAgentForUserAgentType userAgentType: CWVUserAgentType,
    request: URLRequest
  ) -> String? {
    guard let tab, let delegate = tab.delegate else { return nil }
    return delegate.tab(tab, userAgentForType: .init(userAgentType), request: request)
  }

  public func webView(
    _ webView: CWVWebView,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential: URLCredential,
    completionHandler handler: @escaping (String?, String?) -> Void
  ) {
    Task {
      guard let tab, let delegate = tab.delegate else {
        handler(nil, nil)
        return
      }
      let resolvedCredential = await delegate.tab(
        tab,
        didRequestHTTPAuthFor: protectionSpace,
        proposedCredential: proposedCredential,
        previousFailureCount: 0
      )
      handler(resolvedCredential?.user, resolvedCredential?.password)
    }
  }

  public func webViewDidStartNavigation(_ webView: CWVWebView) {
    guard let tab else { return }

    // Reset redirect chain
    tab.redirectChain = []
    if let url = webView.visibleURL {
      tab.redirectChain.append(url)
    }

    tab.didStartNavigation()
  }

  public func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationAction: CWVNavigationAction,
    decisionHandler: @escaping (CWVNavigationActionPolicy) -> Void
  ) {
    guard let tab else { return }
    let navigationType: WebNavigationType = {
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
      let policy = await tab.shouldAllowRequest(
        navigationAction.request,
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
    guard let tab else { return }
    tab.isRestoring = false
    tab.didCommitNavigation()
  }

  public func webViewDidRedirectNavigation(_ webView: CWVWebView) {
    guard let tab else { return }
    if let url = webView.visibleURL {
      tab.redirectChain.append(url)
    }
    tab.didRedirectNavigation()
  }

  public func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationResponse: CWVNavigationResponse,
    decisionHandler: @escaping (CWVNavigationResponsePolicy) -> Void
  ) {
    guard let tab else { return }
    Task { @MainActor in
      let policy = await tab.shouldAllowResponse(
        navigationResponse.response,
        responseInfo: .init(isForMainFrame: navigationResponse.isForMainFrame)
      )
      decisionHandler(policy == .allow ? .allow : .cancel)
    }
  }

  public func webViewDidFinishNavigation(_ webView: CWVWebView) {
    guard let tab else { return }
    tab.didFinishNavigation()
  }

  public func webView(_ webView: CWVWebView, didFailNavigationWithError error: any Error) {
    guard let tab else { return }
    tab.didFailNavigation(with: error)
  }

  public func webView(_ webView: CWVWebView, didRequestDownloadWith task: CWVDownloadTask) {
    guard let tab else { return }
    let pendingDownload = ChromiumDownload(
      downloadTask: task,
      didFinish: { [weak tab] download, error in
        guard let tab else { return }
        tab.downloadDelegate?.tab(tab, didFinishDownload: download, error: error)
      }
    )
    tab.downloadDelegate?.tab(tab, didCreateDownload: pendingDownload)
  }
}
