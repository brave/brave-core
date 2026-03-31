// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Foundation
import OSLog
import Web

extension TabDataValues {
  private struct BraveSearchTabHelperKey: TabDataKey {
    static var defaultValue: BraveSearchTabHelper?
  }
  var braveSearch: BraveSearchTabHelper? {
    get { self[BraveSearchTabHelperKey.self] }
    set { self[BraveSearchTabHelperKey.self] = newValue }
  }
}

class BraveSearchTabHelper: TabObserver, TabPolicyDecider {
  private weak var tab: (any TabState)?
  private let rewards: BraveRewards

  /// A helper property that handles native to Brave Search communication.
  private var braveSearchManager: BraveSearchManager?

  /// A helper property that handles Brave Search Result Ads.
  private(set) var braveSearchResultAdManager: BraveSearchResultAdManager?

  var presentSearchResultClickedInfoBar: (() -> Void)?

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

  // MARK: - TabObserver

  func tabDidFinishNavigation(_ tab: some TabState) {
    // Second attempt to inject results to the BraveSearch.
    // This will be called if we got fallback results faster than
    // the page navigation.
    if let braveSearchManager = braveSearchManager {
      // Fallback results are ready before navigation finished,
      // they must be injected here.
      if !braveSearchManager.fallbackQueryResultsPending {
        injectResults(into: tab)
      }
    } else {
      // If not applicable, null results must be injected regardless.
      // The website waits on us until this is called with either results or null.
      injectResults(into: tab)
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
    tab.removePolicyDecider(self)
  }

  // MARK: - TabPolicyDecider

  @MainActor
  func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url else { return .allow }
    if requestInfo.isMainFrame,
      BraveSearchManager.isValidURL(requestURL)
    {
      // We fetch cookies to determine if backup search was enabled on the website.
      let cookies = await tab.configuration?.websiteDataStore.httpCookieStore.allCookies() ?? []
      braveSearchManager = BraveSearchManager(
        url: requestURL,
        cookies: cookies
      )

      let isAdBlockModeAggressive =
        tab.braveShieldsHelper?.shieldLevel(
          for: requestURL,
          considerAllShieldsOption: true
        ).isAggressive ?? true

      if BraveSearchResultAdManager.shouldTriggerSearchResultAdClickedEvent(
        requestURL,
        isPrivateBrowsing: tab.isPrivate,
        isAggressiveAdsBlocking: isAdBlockModeAggressive
      ) {
        let showSearchResultAdClickedPrivacyNotice =
          rewards.ads.shouldShowSearchResultAdClickedInfoBar()
        BraveSearchResultAdManager.maybeTriggerSearchResultAdClickedEvent(
          requestURL,
          rewards: rewards,
          completion: { [weak self] success in
            guard let self, success, showSearchResultAdClickedPrivacyNotice else {
              return
            }
            presentSearchResultClickedInfoBar?()
          }
        )
      } else {
        // The Brave-Search-Ads header should be added with a negative value when all
        // of the following conditions are met:
        //   - The current tab is not a Private tab
        //   - Brave Rewards is enabled.
        //   - The "Search Ads" is opted-out.
        //   - The requested URL host is one of the Brave Search domains.
        if !tab.isPrivate && rewards.isEnabled
          && !rewards.ads.isOptedInToSearchResultAds()
          && request.allHTTPHeaderFields?["Brave-Search-Ads"] == nil
        {
          var modifiedRequest = URLRequest(url: requestURL)
          modifiedRequest.setValue("?0", forHTTPHeaderField: "Brave-Search-Ads")
          tab.loadRequest(modifiedRequest)
          return .cancel
        }

        braveSearchResultAdManager = BraveSearchResultAdManager(
          url: requestURL,
          rewards: rewards,
          isPrivateBrowsing: tab.isPrivate,
          isAggressiveAdsBlocking: isAdBlockModeAggressive
        )
      }

      if let braveSearchManager = braveSearchManager {
        braveSearchManager.fallbackQueryResultsPending = true
        braveSearchManager.shouldUseFallback { backupQuery in
          guard let query = backupQuery else {
            braveSearchManager.fallbackQueryResultsPending = false
            return
          }

          if query.found {
            braveSearchManager.fallbackQueryResultsPending = false
          } else {
            braveSearchManager.backupSearch(with: query) { [weak self] completion in
              guard let self, let tab = self.tab else { return }
              braveSearchManager.fallbackQueryResultsPending = false
              injectResults(into: tab)
            }
          }
        }
      }
    } else {
      braveSearchManager = nil
      braveSearchResultAdManager = nil
    }

    return .allow
  }

  func processSearchResultAds(
    _ searchResultAds: SearchResultAdResponse
  ) {
    guard let braveSearchResultAdManager else { return }
    for ad in searchResultAds.creatives {
      guard let rewardsValue = Double(ad.rewardsValue)
      else {
        Logger.module.error("Failed to process search result ads JSON-LD")
        return
      }

      var conversion: BraveAds.CreativeSetConversionInfo?
      if let conversionUrlPatternValue = ad.conversionUrlPatternValue,
        let conversionObservationWindowValue = ad.conversionObservationWindowValue
      {
        let timeInterval = TimeInterval(conversionObservationWindowValue) * 1.days
        conversion = .init(
          urlPattern: conversionUrlPatternValue,
          verifiableAdvertiserPublicKeyBase64: ad.conversionAdvertiserPublicKeyValue,
          observationWindow: Date(timeIntervalSince1970: timeInterval)
        )
      }

      let searchResultAd: BraveAds.CreativeSearchResultAdInfo = .init(
        type: .searchResultAd,
        placementId: ad.placementId,
        creativeInstanceId: ad.creativeInstanceId,
        creativeSetId: ad.creativeSetId,
        campaignId: ad.campaignId,
        advertiserId: ad.advertiserId,
        targetUrl: ad.landingPage,
        headlineText: ad.headlineText,
        description: ad.description,
        value: rewardsValue,
        creativeSetConversion: conversion
      )

      braveSearchResultAdManager.triggerSearchResultAdViewedEvent(
        placementId: ad.placementId,
        searchResultAd: searchResultAd
      )
    }
  }

  /// Call the api on the Brave Search website and passes the fallback results to it.
  /// Important: This method is also called when there is no fallback results
  /// or when the fallback call should not happen at all.
  /// The website expects the iOS device to always call this method(blocks on it).
  private func injectResults(into tab: some TabState) {
    DispatchQueue.main.async {
      // If the backup search results happen before the Brave Search loads
      // The method we pass data to is undefined.
      // For such case we do not call that method or remove the search backup manager.
      tab.evaluateJavaScript(
        functionName: "window.onFetchedBackupResults === undefined",
        contentWorld: BraveSearchScriptHandler.scriptSandbox,
        asFunction: false
      ) { (result, error) in

        if let error = error {
          Logger.module.error(
            "onFetchedBackupResults existence check error: \(error.localizedDescription, privacy: .public)"
          )
        }

        guard let methodUndefined = result as? Bool else {
          Logger.module.error(
            "onFetchedBackupResults existence check, failed to unwrap bool result value"
          )
          return
        }

        if methodUndefined {
          Logger.module.info("Search Backup results are ready but the page has not been loaded yet")
          return
        }

        var queryResult = "null"

        if let url = tab.visibleURL,
          BraveSearchManager.isValidURL(url),
          let result = self.braveSearchManager?.fallbackQueryResult
        {
          queryResult = result
        }

        tab.evaluateJavaScript(
          functionName: "window.onFetchedBackupResults",
          args: [queryResult],
          contentWorld: BraveSearchScriptHandler.scriptSandbox,
          escapeArgs: false
        )

        // Cleanup
        self.braveSearchManager = nil
      }
    }
  }
}

struct SearchResultAdResponse: Decodable {
  struct SearchResultAd: Decodable {
    let creativeInstanceId: String
    let placementId: String
    let creativeSetId: String
    let campaignId: String
    let advertiserId: String
    let landingPage: URL
    let headlineText: String
    let description: String
    let rewardsValue: String
    let conversionUrlPatternValue: String?
    let conversionAdvertiserPublicKeyValue: String?
    let conversionObservationWindowValue: Int?
  }

  let creatives: [SearchResultAd]
}
