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

public class UserAgentTabHelper {
  private weak var tab: (any TabState)?
  private weak var braveUserAgentExceptions: BraveUserAgentExceptionsIOS?
  private struct DecidePolicyRequestInfo {
    let request: URLRequest
    let requestInfo: WebRequestInfo
  }
  private var requestsForCurrentNavigation: [DecidePolicyRequestInfo] = []

  public init(tab: any TabState, braveUserAgentExceptions: BraveUserAgentExceptionsIOS?) {
    self.tab = tab
    self.braveUserAgentExceptions = braveUserAgentExceptions
    tab.addObserver(self)
    tab.addPolicyDecider(self)
  }

  private func userAgent(for request: URLRequest) -> String {
    if !Preferences.Debug.userAgentOverride.value.isEmpty {
      return Preferences.Debug.userAgentOverride.value
    }
    let isBraveAllowedInUA =
      request.mainDocumentURL.flatMap {
        braveUserAgentExceptions?.canShowBrave($0)
      } ?? true

    let mobile: String
    let desktop: String
    if isBraveAllowedInUA {
      let userAgentType = GetDefaultBraveIOSUserAgentType()
      mobile = userAgentType.userAgentForMode(isMobile: true)
      desktop = userAgentType.userAgentForMode(isMobile: false)
    } else {
      mobile = UserAgent.mobileMasked
      desktop = UserAgent.desktopMasked
    }

    // TODO: respect correct desktop/mobile mode
    return mobile
  }
}

extension UserAgentTabHelper: TabObserver {
  public func tabDidStartNavigation(_ tab: some TabState) {
  }

  public func tabDidFinishNavigation(_ tab: some TabState) {
    requestsForCurrentNavigation.removeAll()
  }

  public func tabDidCommitNavigation(_ tab: some TabState) {
    requestsForCurrentNavigation.removeAll()
  }

  public func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    requestsForCurrentNavigation.removeAll()
  }

  public func tabDidRedirectNavigation(_ tab: some TabState) {
    guard FeatureList.kShouldCancelRequestsForUserAgentChange.enabled,
      requestsForCurrentNavigation.count > 1,
      let lastRequestInfo = requestsForCurrentNavigation.last,
      case let expectedUserAgentForFinalRequest = self.userAgent(for: lastRequestInfo.request),
      let headerUserAgent = lastRequestInfo.request.allHTTPHeaderFields?["User-Agent"],
      headerUserAgent != expectedUserAgentForFinalRequest
    else {
      return
    }
    tab.stopLoading()

    // When changing user agent, we must cancel & restart the request
    // as the headers will contain the old user agent which may result
    // in webcompat issues if we need to hide we are Brave from the
    // domain
    var modifiedRequest = lastRequestInfo.request
    modifiedRequest.setValue(
      expectedUserAgentForFinalRequest,
      forHTTPHeaderField: "User-Agent"
    )

    if let url = modifiedRequest.url {
      Logger.module.debug(
        "Cancelled and restarting request to `\(url.absoluteString, privacy: .private)`"
      )
    }

    tab.loadRequest(modifiedRequest)
  }
}

extension UserAgentTabHelper: TabPolicyDecider {
  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    // we only care about main frame navigations as the user agent is
    // determined by main frame domain.
    guard requestInfo.isMainFrame else { return .allow }
    requestsForCurrentNavigation.append(
      .init(
        request: request,
        requestInfo: requestInfo
      )
    )
    // We don't need to block / allow as our logic will reject/cancel/stop
    // in `tabDidRedirectNavigation` if needed
    return .allow
  }
}
