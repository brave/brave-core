/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

bool IsAllowedToLandOnPage(mojom::AdType mojom_ad_type) {
  switch (mojom_ad_type) {
    case mojom::AdType::kInlineContentAd:
    case mojom::AdType::kPromotedContentAd: {
      // Only if:
      // - The user has joined Brave News.
      return UserHasOptedInToBraveNewsAds();
    }

    case mojom::AdType::kNewTabPageAd: {
      // Only if:
      // - The user has opted into new tab page ads and has joined Brave
      //   Rewards.
      return UserHasJoinedBraveRewards() && UserHasOptedInToNewTabPageAds();
    }

    case mojom::AdType::kNotificationAd: {
      // Only if:
      // - The user has opted into notification ads. Users cannot opt into
      //   notification ads without joining Brave Rewards.
      return UserHasOptedInToNotificationAds();
    }

    case mojom::AdType::kSearchResultAd: {
      // Only if:
      // - The user has opted into search result ads and has joined Brave
      //   Rewards.
      return UserHasJoinedBraveRewards() && UserHasOptedInToSearchResultAds();
    }

    case mojom::AdType::kUndefined: {
      break;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::AdType: "
               << base::to_underlying(mojom_ad_type);
}

bool ShouldResumePageLand(int32_t tab_id) {
  return TabManager::GetInstance().IsVisible(tab_id) &&
         BrowserManager::GetInstance().IsActive() &&
         BrowserManager::GetInstance().IsInForeground();
}

bool DidLandOnPage(int32_t tab_id, const GURL& url) {
  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(tab_id);
  if (!tab) {
    // The tab has been closed.
    return false;
  }

  return DomainOrHostExists(tab->redirect_chain, url);
}

}  // namespace brave_ads
