// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

// A helper class to handle Brave Search Result Ads.
class BraveSearchResultAdManager: NSObject {
  private static let searchResultAdClickedUrlPath = "/a/redirect"

  private static let placementId = "placement_id"

  let rewards: BraveRewards

  init?(url: URL, rewards: BraveRewards, isPrivateBrowsing: Bool, isAggressiveAdsBlocking: Bool) {
    if !BraveSearchResultAdManager.shouldTriggerSearchResultAd(
      url: url,
      isPrivateBrowsing: isPrivateBrowsing,
      isAggressiveAdsBlocking: isAggressiveAdsBlocking
    ) {
      return nil
    }

    self.rewards = rewards
  }

  func triggerSearchResultAdViewedEvent(
    placementId: String,
    searchResultAd: BraveAds.CreativeSearchResultAdInfo
  ) {
    rewards.ads.triggerSearchResultAdEvent(
      searchResultAd,
      eventType: .viewedImpression,
      completion: { _ in }
    )
  }

  static func maybeTriggerSearchResultAdClickedEvent(
    _ url: URL,
    rewards: BraveRewards,
    completion: @escaping ((Bool) -> Void)
  ) {
    guard let placementId = getPlacementID(url) else {
      return
    }

    rewards.ads.triggerSearchResultAdClickedEvent(
      placementId,
      completion: completion
    )
  }

  static func shouldTriggerSearchResultAdClickedEvent(
    _ url: URL,
    isPrivateBrowsing: Bool,
    isAggressiveAdsBlocking: Bool
  ) -> Bool {
    return shouldTriggerSearchResultAd(
      url: url,
      isPrivateBrowsing: isPrivateBrowsing,
      isAggressiveAdsBlocking: isAggressiveAdsBlocking
    ) && getPlacementID(url) != nil
  }

  private static func getPlacementID(_ url: URL) -> String? {
    if !BraveSearchManager.isValidURL(url) || url.path != searchResultAdClickedUrlPath {
      return nil
    }
    guard let queryItems = URLComponents(url: url, resolvingAgainstBaseURL: false)?.queryItems
    else {
      return nil
    }
    return queryItems.first(where: { $0.name == placementId })?.value
  }

  private static func shouldTriggerSearchResultAd(
    url: URL,
    isPrivateBrowsing: Bool,
    isAggressiveAdsBlocking: Bool
  ) -> Bool {
    return !isPrivateBrowsing && !isAggressiveAdsBlocking && BraveSearchManager.isValidURL(url)
      && BraveAds.shouldSupportSearchResultAds()
  }
}
