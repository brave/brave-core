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
class AdsTextContentTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  private let rewards: BraveRewards

  private var redirectChain: [URL] = []
  private var isSameDocumentNavigation = false
  private var hadNavigationError = false

  init(tab: some TabState, rewards: BraveRewards) {
    self.tab = tab
    self.rewards = rewards
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    redirectChain = tab.visibleURL.map { [$0] } ?? []
    isSameDocumentNavigation = false
    hadNavigationError = false
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
    guard shouldReportTextContentDidChange(for: tab) else { return }
    Task { [redirectChain] in
      await reportTextContentDidChange(in: tab, redirectChain: redirectChain)
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  private func shouldReportTextContentDidChange(for tab: some TabState) -> Bool {
    // Mirrors `AdsTabHelper::ShouldNotifyTabContentDidChange`, except for HTTP status/new
    // navigation checks, which are not exposed to Swift on the legacy path.
    !tab.isRestoring && !isSameDocumentNavigation && !hadNavigationError && !redirectChain.isEmpty
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
