// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web
import WebKit

extension TabDataValues {
  private struct AdsTextContentTabHelperKey: TabDataKey {
    static var defaultValue: AdsTextContentTabHelper?
  }

  var adsTextContentTabHelper: AdsTextContentTabHelper? {
    get { self[AdsTextContentTabHelperKey.self] }
    set { self[AdsTextContentTabHelperKey.self] = newValue }
  }
}

/// Legacy (`WKWebView`) counterpart of `AdsTabHelper::MaybeNotifyTabTextContentDidChange` on the
/// new path. `AdsTabHelper` observes the underlying `web::WebState` in both configurations, but its
/// `TextContentDistillerJavaScriptFeature` script is only injected when
/// `FeatureList.kUseProfileWebViewConfiguration` is enabled, so text classification silently no-ops
/// on the legacy path. This mirrors `AdsTabHelper`'s gating so the distilled text is only reported
/// for genuine, successfully-loaded, non-restored, non-same-document navigations.
@MainActor
class AdsTextContentTabHelper: TabObserver, TabPolicyDecider {
  private weak var tab: (any TabState)?
  private let rewards: BraveRewards

  private var redirectChain: [URL] = []
  private var isSameDocumentNavigation = false
  private var hadNavigationError = false
  private var isNewNavigation = false
  private var httpStatusCode: Int?
  /// Snapshot of `tab.isRestoring` taken at navigation start. `tab.isRestoring` itself is cleared
  /// on commit (`TabCWVNavigationHandler.webViewDidCommitNavigation`), well before
  /// `tabDidStopLoading` fires, so it can't be checked live at that point.
  private var wasRestoring = false

  init(tab: some TabState, rewards: BraveRewards) {
    self.tab = tab
    self.rewards = rewards
    tab.addObserver(self)
    tab.addPolicyDecider(self)
  }

  deinit {
    tab?.removeObserver(self)
    tab?.removePolicyDecider(self)
  }

  func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    if requestInfo.isMainFrame {
      isNewNavigation = ![.reload, .backForward, .formResubmitted].contains(
        requestInfo.navigationType
      )
    }
    return .allow
  }

  func tab(
    _ tab: some TabState,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    if responseInfo.isForMainFrame {
      httpStatusCode = (response as? HTTPURLResponse)?.statusCode
    }
    return .allow
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    redirectChain = tab.visibleURL.map { [$0] } ?? []
    isSameDocumentNavigation = false
    hadNavigationError = false
    httpStatusCode = nil
    wasRestoring = tab.isRestoring
  }

  func tabDidRedirectNavigation(_ tab: some TabState) {
    if let url = tab.visibleURL {
      redirectChain.append(url)
    }
  }

  func tabDidCommitSameDocumentNavigation(_ tab: some TabState) {
    isSameDocumentNavigation = true
  }

  func tabDidCommitNavigation(_ tab: some TabState) {
    isSameDocumentNavigation = false
  }

  func tab(_ tab: some TabState, didFailNavigationWithError error: Error) {
    hadNavigationError = true
  }

  func tabDidStopLoading(_ tab: some TabState) {
    guard rewards.shouldReportTextContentDidChange, shouldReportTextContentDidChange else {
      return
    }
    Task { [weak self, weak tab, redirectChain] in
      guard let self, let tab else { return }
      await reportTextContentDidChange(in: tab, redirectChain: redirectChain)
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
    tab.removePolicyDecider(self)
  }

  private var shouldReportTextContentDidChange: Bool {
    // Mirrors `AdsTabHelper::ShouldNotifyTabContentDidChange`. No response headers were received
    // (e.g. a `file://` or `data:` navigation) but the navigation didn't error, so treat it as
    // HTTP OK, matching `AdsTabHelper::DidFinishNavigation`.
    !wasRestoring && isNewNavigation && !isSameDocumentNavigation && !hadNavigationError
      && !redirectChain.isEmpty
      && !isHttpErrorStatusCode(httpStatusCode ?? 200)
  }

  private func reportTextContentDidChange(in tab: some TabState, redirectChain: [URL]) async {
    guard
      let text =
        try? await tab.evaluateJavaScript(
          functionName: "window.__firefox__.\(AdsTextContentDistillerScriptHandler.getTextContent)",
          args: [AdsTextContentDistillerScriptHandler.scriptId],
          contentWorld: AdsTextContentDistillerScriptHandler.scriptSandbox,
          asFunction: true
        ) as? String
    else {
      return
    }
    rewards.reportTextContentDidChange(tab: tab, redirectChain: redirectChain, text: text)
  }
}

/// Mirrors `net::IsClientError`/`net::IsServerError`, used by
/// `AdsTabHelper::ShouldNotifyTabContentDidChange` in the native path to exclude 4xx/5xx pages.
private func isHttpErrorStatusCode(_ statusCode: Int) -> Bool {
  (400...599).contains(statusCode)
}
