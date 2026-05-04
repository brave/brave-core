// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Web
import os.log

extension TabDataValues {
  private struct UserAgentTabHelperKey: TabDataKey {
    static var defaultValue: UserAgentTabHelper?
  }
  public var userAgentTabHelper: UserAgentTabHelper? {
    get { self[UserAgentTabHelperKey.self] }
    set { self[UserAgentTabHelperKey.self] = newValue }
  }
}

@MainActor public class UserAgentTabHelper {
  private weak var tab: (any TabState)?
  private weak var braveUserAgentExceptions: BraveUserAgentExceptionsIOS?
  private var userAgentForRequest: (URLRequest) -> String
  private struct DecidePolicyRequestInfo {
    let request: URLRequest
    let requestInfo: WebRequestInfo
  }
  private var requestsForCurrentNavigation: [DecidePolicyRequestInfo] = []

  public init(
    tab: any TabState,
    braveUserAgentExceptions: BraveUserAgentExceptionsIOS?,
    userAgentForRequest: @escaping (URLRequest) -> String
  ) {
    self.tab = tab
    self.braveUserAgentExceptions = braveUserAgentExceptions
    self.userAgentForRequest = userAgentForRequest
    tab.addObserver(self)
    tab.addPolicyDecider(self)
  }
}

extension UserAgentTabHelper: @MainActor TabObserver {
  public func tabDidFinishNavigation(_ tab: some TabState) {
    Logger.module.info(
      "UserAgentTabHelper.tabDidFinishNavigation(_:) - resetting requestsForCurrentNavigation"
    )
    requestsForCurrentNavigation.removeAll()
  }

  public func tabDidCommitNavigation(_ tab: some TabState) {
    Logger.module.info(
      "UserAgentTabHelper.tabDidCommitNavigation(_:) - resetting requestsForCurrentNavigation"
    )
    requestsForCurrentNavigation.removeAll()
  }

  public func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    Logger.module.info(
      "UserAgentTabHelper.tab(_:didFailNavigationWithError:) - resetting requestsForCurrentNavigation"
    )
    requestsForCurrentNavigation.removeAll()
  }
}

extension UserAgentTabHelper: TabPolicyDecider {
  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard FeatureList.kShouldCancelRequestsForUserAgentChange.enabled else { return .allow }
    Logger.module.info(
      "UserAgentTabHelper.tab(_:shouldAllowRequest:requestInfo:) - request.mainDocumentURL=\(request.mainDocumentURL?.baseDomain ?? "", privacy: .public)"
    )
    // we only care about main frame navigations as the user agent is
    // determined by main frame domain.
    guard requestInfo.isMainFrame else { return .allow }
    requestsForCurrentNavigation.append(
      .init(
        request: request,
        requestInfo: requestInfo
      )
    )
    if requestsForCurrentNavigation.count > 1,
      let lastRequestInfo = requestsForCurrentNavigation.last,
      case let expectedUserAgentForFinalRequest = self.userAgentForRequest(lastRequestInfo.request),
      let headerUserAgent = lastRequestInfo.request.allHTTPHeaderFields?[kHeaderUserAgent],
      headerUserAgent != expectedUserAgentForFinalRequest,
      // attempt to confirm redirect
      lastRequestInfo.requestInfo.navigationType == .linkActivated,
      lastRequestInfo.request.allHTTPHeaderFields?[kHeaderSecFetchMode] == "navigate",
      lastRequestInfo.request.allHTTPHeaderFields?[kHeaderSecFetchDest] == "document"
    {
      // When changing user agent, we must cancel & restart the request
      // as the headers will contain the old user agent which may result
      // in webcompat issues if we need to hide we are Brave from the
      // domain
      var modifiedRequest = lastRequestInfo.request
      modifiedRequest.setValue(
        expectedUserAgentForFinalRequest,
        forHTTPHeaderField: kHeaderUserAgent
      )
      if let url = modifiedRequest.url {
        Logger.module.info(
          "UserAgentTabHelper.tab(_:shouldAllowRequest:requestInfo:) - Detected incorrect UA in header. Cancelled and recreating request to `\(url.baseDomain ?? "", privacy: .public)` for user agent change from `\(headerUserAgent, privacy: .public)` to `\(expectedUserAgentForFinalRequest, privacy: .public)`."
        )
      }
      // Alert tester request is being cancelled and re-created
      UIImpactFeedbackGenerator(style: .heavy).vibrate()
      for count in 0..<10 {
        DispatchQueue.main.asyncAfter(deadline: .now() + Double(count) * 0.1) {
          UIImpactFeedbackGenerator(style: .heavy).vibrate()
        }
      }
      tab.loadRequest(modifiedRequest)
      return .cancel
    }
    return .allow
  }
}

private let kHeaderUserAgent = "User-Agent"
private let kHeaderSecFetchMode = "Sec-Fetch-Mode"
private let kHeaderSecFetchDest = "Sec-Fetch-Dest"
