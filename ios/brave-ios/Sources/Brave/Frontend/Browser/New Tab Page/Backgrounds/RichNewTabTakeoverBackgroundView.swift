// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SnapKit
import UIKit

class RichNewTabTakeoverBackgroundView: UIView, BraveWebViewUIDelegate, BraveWebViewNavigationDelegate {
  var webUIController: ChromeWebUIController?

  func resetRichNewTabTakeoverLayer() {
    webUIController = nil
  }

  func setupRichNewTabTakeoverLayer(
    braveProileController: BraveProfileController,
    richNewTabTakeoverURL: URL
  ) {
    if webUIController != nil {
      return
    }

    webUIController = ChromeWebUIController(
      braveCore: braveProileController,
      isPrivateBrowsing: false
    ).then {
      $0.webView.uiDelegate = self
      $0.webView.navigationDelegate = self
      $0.webView.load(URLRequest(url: richNewTabTakeoverURL))

      $0.view.backgroundColor = .black
      $0.webView.backgroundColor = UIColor.black
      $0.webView.internalWebView?.backgroundColor = .black
      $0.webView.internalWebView?.isOpaque = false

      addSubview($0.view)
      $0.view.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    clipsToBounds = true
    backgroundColor = .black
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: - BraveWebViewNavigationDelegate

  func webViewDidStartNavigation(_ webView: CWVWebView) {
    print("FOOBAR.webViewDidStartNavigation : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webViewDidCommitNavigation(_ webView: CWVWebView) {
    print("FOOBAR.webViewDidCommitNavigation : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webViewDidFinishNavigation(_ webView: CWVWebView) {
    print("FOOBAR.webViewDidFinishNavigation : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webView(_ webView: CWVWebView, didFailNavigationWithError error: any Error) {
    print("FOOBAR.didFailNavigationWithError : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webViewDidRedirectNavigation(_ webView: CWVWebView) {
    print("FOOBAR.webViewDidRedirectNavigation : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationAction: BraveNavigationAction,
    decisionHandler: @escaping (CWVNavigationActionPolicy) -> Void
  ) {
    print("FOOBAR.decidePolicyFor : \(Date.now.timeIntervalSince1970 * 1000)")
    decisionHandler(.allow)
  }

  func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationResponse: CWVNavigationResponse,
    decisionHandler: @escaping (CWVNavigationResponsePolicy) -> Void
  ) {
    print("FOOBAR.decidePolicyForResponse : \(Date.now.timeIntervalSince1970 * 1000)")
    decisionHandler(.allow)
  }

  func webViewWebContentProcessDidTerminate(_ webView: CWVWebView) {
    print("FOOBAR.webContentProcessDidTerminate : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webView(_ webView: CWVWebView, didRequestDownloadWith task: CWVDownloadTask) {
    print("FOOBAR.didRequestDownloadWith : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  // MARK: - BraveWebViewUIDelegate

  func webViewDidCreateNewWebView(_ webView: CWVWebView) {
    print("FOOBAR.webViewDidCreateNewWebView : \(Date.now.timeIntervalSince1970 * 1000)")
  }

  func webView(_ webView: CWVWebView, buildEditMenuWith builder: any UIMenuBuilder) {
    print("FOOBAR.buildEditMenuWith : \(Date.now.timeIntervalSince1970 * 1000)")
  }
}
