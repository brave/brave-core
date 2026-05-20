// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import Web

extension TabDataValues {
  private struct QuickViewTabHelperKey: TabDataKey {
    static var defaultValue: QuickViewTabHelper?
  }

  var quickViewTabHelper: QuickViewTabHelper? {
    get { self[QuickViewTabHelperKey.self] }
    set { self[QuickViewTabHelperKey.self] = newValue }
  }
}

@MainActor
class QuickViewTabHelper: TabPolicyDecider {
  var presentInQuickView: ((URL, any TabState) -> Void)?

  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url else {
      return .allow  // let other deciders to cancel
    }

    if FeatureList.kQuickViewEnabled.enabled,
      shouldOpenInQuickView(requestURL: requestURL, requestInfo: requestInfo, tab: tab)
    {
      presentInQuickView?(requestURL, tab)
      return .cancel
    }

    return .allow
  }

  private func shouldOpenInQuickView(
    requestURL: URL,
    requestInfo: WebRequestInfo,
    tab: some TabState
  ) -> Bool {
    guard requestInfo.isMainFrame,
      requestInfo.navigationType == .linkActivated,
      requestInfo.isUserInitiated,
      let sourceURL = tab.lastCommittedURL,
      !BraveSearchManager.isValidURL(requestURL),  // don't intercept same-domain nav
      isQuickViewEntryPoint(sourceURL),
      !InternalURL.isValid(url: requestURL),
      requestURL.isWebPage(includeDataURIs: false)
    else { return false }
    return true
  }

  private func isQuickViewEntryPoint(_ url: URL) -> Bool {
    guard BraveSearchManager.isValidURL(url),
      let path = URLComponents(url: url, resolvingAgainstBaseURL: false)?.path
    else { return false }
    return path == "/search" || path == "/ask"
  }
}
