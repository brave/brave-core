// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore

// A helper class to handle Brave Search Result Ads.
class BraveSearchResultAdManager: NSObject {
  private let rewards: BraveRewards

  init?(url: URL, rewards: BraveRewards, isPrivateBrowsing: Bool) {
    if !BraveAds.shouldSupportSearchResultAds() ||
       !BraveSearchManager.isValidURL(url) ||
       isPrivateBrowsing ||
       rewards.isEnabled {
      return nil
    }

    self.rewards = rewards
  }

  func triggerSearchResultAdViewedEvent(_ searchResultAdInfo: BraveAds.SearchResultAdInfo) {
      rewards.ads.triggerSearchResultAdEvent(
        searchResultAdInfo,
        eventType: .viewed,
        completion: { _ in })
  }
}
