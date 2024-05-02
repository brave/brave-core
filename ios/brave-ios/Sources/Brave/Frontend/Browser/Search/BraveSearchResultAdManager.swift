// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

// A helper class to handle Brave Search Result Ads.
class BraveSearchResultAdManager: NSObject {
  private let searchResultAdClickedUrlPath = "/a/redirect"

  private let placementId = "placement_id"

  private let rewards: BraveRewards

  private var searchResultAds = [String: BraveAds.SearchResultAdInfo]()

  init?(url: URL, rewards: BraveRewards, isPrivateBrowsing: Bool) {
    if !BraveAds.shouldSupportSearchResultAds() || !BraveSearchManager.isValidURL(url)
      || isPrivateBrowsing
    {
      return nil
    }

    self.rewards = rewards
  }

  func isSearchResultAdClickedURL(_ url: URL) -> Bool {
    return getPlacementID(url) != nil
  }

  func triggerSearchResultAdViewedEvent(
    placementId: String,
    searchResultAd: BraveAds.SearchResultAdInfo
  ) {
    searchResultAds[placementId] = searchResultAd

    guard let searchResultAd = searchResultAds[placementId] else {
      return
    }

    rewards.ads.triggerSearchResultAdEvent(
      searchResultAd,
      eventType: .viewedImpression,
      completion: { _ in }
    )
  }

  func maybeTriggerSearchResultAdClickedEvent(_ url: URL) {
    guard let placementId = getPlacementID(url) else {
      return
    }

    guard let searchResultAd = searchResultAds[placementId] else {
      return
    }

    rewards.ads.triggerSearchResultAdEvent(
      searchResultAd,
      eventType: .clicked,
      completion: { _ in }
    )
  }

  private func getPlacementID(_ url: URL) -> String? {
    if !BraveSearchManager.isValidURL(url) || url.path != searchResultAdClickedUrlPath {
      return nil
    }
    guard let queryItems = URLComponents(url: url, resolvingAgainstBaseURL: false)?.queryItems
    else {
      return nil
    }
    return queryItems.first(where: { $0.name == placementId })?.value
  }
}
