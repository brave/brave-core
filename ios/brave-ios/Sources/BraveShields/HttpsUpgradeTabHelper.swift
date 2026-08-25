// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Preferences
import Shared
import Web
import os

extension TabDataValues {
  private struct HttpsUpgradeTabHelperKey: TabDataKey {
    static var defaultValue: HttpsUpgradeTabHelper?
  }
  public var httpsUpgradeHelper: HttpsUpgradeTabHelper? {
    get { self[HttpsUpgradeTabHelperKey.self] }
    set { self[HttpsUpgradeTabHelperKey.self] = newValue }
  }
}

/// Upgrades main frame `http` navigations to `https` and rolls the upgrade
/// back (or shows the blocked interstitial in strict mode) when the upgraded
/// navigation fails.
@MainActor
public class HttpsUpgradeTabHelper {
  /// Fallback to http timeout timer duration
  private static let upgradeTimeout: TimeInterval = 3.seconds

  private weak var tab: (any TabState)?
  private let httpsUpgradeExceptionsService: HTTPSUpgradeExceptionsService

  /// The request that was upgraded to HTTPS.
  ///
  /// This allows us to rollback the upgrade when the upgraded navigation fails.
  public private(set) var upgradedHTTPSRequest: URLRequest?

  /// A timer that's started on HTTPS upgrade. If the upgrade hasn't completed
  /// within `upgradeTimeout`, it is cancelled and we fallback to HTTP or show
  /// the interstitial (standard vs. strict).
  private var upgradeTimeoutTimer: Timer?

  public init(
    tab: some TabState,
    httpsUpgradeExceptionsService: HTTPSUpgradeExceptionsService
  ) {
    self.tab = tab
    self.httpsUpgradeExceptionsService = httpsUpgradeExceptionsService
    tab.addPolicyDecider(self)
    tab.addObserver(self)
  }

  /// Allow `http` for the given url's host.
  public func allowHttp(for url: URL) {
    guard let tab, let host = url.host(percentEncoded: false) else { return }
    HttpsUpgradeServiceFactory.get(privateMode: tab.isPrivate)?.allowHttp(forHost: host)
  }

  /// Clear any stored upgrade without rolling it back.
  public func cancelUpgrade() {
    upgradedHTTPSRequest = nil
    upgradeTimeoutTimer?.invalidate()
    upgradeTimeoutTimer = nil
  }

  /// Determines if the given url should be upgraded from http to https.
  private func shouldUpgradeToHttps(url: URL, isPrivate: Bool) -> Bool {
    guard let httpsUpgradeService = HttpsUpgradeServiceFactory.get(privateMode: isPrivate),
      url.scheme == "http", let host = url.host
    else {
      return false
    }
    let isInUserAllowList = httpsUpgradeService.isHttpAllowed(forHost: host)
    switch Preferences.Shields.httpsUpgradeLevel {
    case .strict:
      // Always upgrade for Strict HTTPS upgrade unless previously allowed by user.
      return !isInUserAllowList
    case .standard:
      // Upgrade for Standard HTTPS upgrade if host is not on the exceptions list
      // and not previously allowed by user.
      return httpsUpgradeExceptionsService.canUpgradeToHTTPS(for: url) && !isInUserAllowList
    case .disabled:
      return false
    }
  }

  /// Upon an invalid response, check if we need to roll back any HTTPS upgrade
  /// or show the interstitial page (if user is using strict mode)
  private func handleInvalidHTTPSUpgrade(responseURL: URL) -> URLRequest? {
    guard let originalRequest = upgradedHTTPSRequest,
      let originalURL = originalRequest.url,
      responseURL.baseDomain == originalURL.baseDomain
    else {
      return nil
    }

    if Preferences.Shields.httpsUpgradeLevel.isStrict,
      let url = originalURL.encodeEmbeddedInternalURL(for: .httpBlocked)
    {
      Logger.module.debug("Show http blocked interstitial for `\(originalURL.absoluteString)`")
      return PrivilegedRequest(url: url) as URLRequest
    }

    Logger.module.debug("Revert HTTPS upgrade for `\(originalURL.absoluteString)`")
    cancelUpgrade()
    allowHttp(for: originalURL)
    return originalRequest
  }
}

// MARK: - TabPolicyDecider
extension HttpsUpgradeTabHelper: TabPolicyDecider {
  public func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard requestInfo.isMainFrame,
      let requestURL = request.url,
      let mainDocumentURL = request.mainDocumentURL,
      requestURL.isWebPage(includeDataURIs: false),
      tab.braveShieldsHelper?.shieldLevel(
        for: mainDocumentURL,
        considerAllShieldsOption: true
      ).isEnabled ?? true,
      shouldUpgradeToHttps(url: requestURL, isPrivate: tab.isPrivate)
    else {
      return .allow
    }

    if let existingUpgradeRequestURL = upgradedHTTPSRequest?.url,
      existingUpgradeRequestURL == requestURL
    {
      // If the server redirected https -> http, the https load never fails, and
      // the request policy is decided before the server redirect is reported,
      // so we must prevent an upgrade loop here.
      guard let fallbackRequest = handleInvalidHTTPSUpgrade(responseURL: requestURL)
      else {
        return .allow
      }
      tab.loadRequest(fallbackRequest)
      return .cancel
    }

    guard var urlComponents = URLComponents(url: requestURL, resolvingAgainstBaseURL: true) else {
      return .allow
    }
    urlComponents.scheme = "https"
    guard let upgradedURL = urlComponents.url else {
      return .allow
    }

    Logger.module.debug("Upgrading `\(requestURL.absoluteString)` to HTTPS")

    upgradedHTTPSRequest = request
    upgradeTimeoutTimer?.invalidate()

    var modifiedRequest = request
    modifiedRequest.url = upgradedURL

    upgradeTimeoutTimer = Timer.scheduledTimer(
      withTimeInterval: Self.upgradeTimeout,
      repeats: false,
      block: { [weak self, weak tab] _ in
        guard let self, let tab,
          let fallbackRequest = self.handleInvalidHTTPSUpgrade(responseURL: upgradedURL)
        else { return }
        tab.stopLoading()
        tab.loadRequest(fallbackRequest)
      }
    )

    tab.loadRequest(modifiedRequest)
    return .cancel
  }
}

// MARK: - TabObserver
extension HttpsUpgradeTabHelper: TabObserver {
  public func tabDidCommitNavigation(_ tab: some TabState) {
    // Reset the stored http request now that the load has committed.
    cancelUpgrade()
  }

  public func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    let error = error as NSError
    if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
      // Load cancelled / user stopped load. Cancel the https upgrade fallback timer.
      cancelUpgrade()
      return
    }

    guard let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL,
      url.scheme == "https",  // verify failing url was https
      let fallbackRequest = handleInvalidHTTPSUpgrade(responseURL: url)
    else {
      return
    }
    // Load the original request or the strict mode interstitial
    tab.loadRequest(fallbackRequest)
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    cancelUpgrade()
    tab.removePolicyDecider(self)
    tab.removeObserver(self)
  }
}
