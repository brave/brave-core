/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ads/search_result_ad.h"

#include "base/check.h"
#include "bat/ads/internal/ad_events/ad_event.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_permission_rules.h"
#include "bat/ads/internal/bundle/creative_search_result_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_search_result_ads_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/search_result_ad_info.h"

namespace ads {

SearchResultAd::SearchResultAd() = default;

SearchResultAd::~SearchResultAd() = default;

void SearchResultAd::AddObserver(SearchResultAdObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void SearchResultAd::RemoveObserver(SearchResultAdObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void SearchResultAd::FireEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::SearchResultAdEventType event_type) {
  if (uuid.empty() || creative_instance_id.empty()) {
    BLOG(1, "Failed to fire search result ad event due to invalid uuid "
                << uuid << " or creative instance id " << creative_instance_id);
    NotifySearchResultAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  search_result_ads::frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Search result ad: Not allowed due to permission rules");
    NotifySearchResultAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  database::table::CreativeSearchResultAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success, const std::string& creative_instance_id,
          const CreativeSearchResultAdInfo& creative_ad) {
        if (!success) {
          BLOG(1,
               "Failed to fire search result ad event due to missing "
               "creative instance id "
                   << creative_instance_id);
          NotifySearchResultAdEventFailed(uuid, creative_instance_id,
                                          event_type);
          return;
        }

        const SearchResultAdInfo& ad = BuildSearchResultAd(creative_ad, uuid);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAd::FireEvent(
    const SearchResultAdInfo& ad,
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::SearchResultAdEventType event_type) {
  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Search result ad: Failed to get ad events");
          NotifySearchResultAdEventFailed(uuid, creative_instance_id,
                                          event_type);
          return;
        }

        if (event_type == mojom::SearchResultAdEventType::kViewed &&
            HasFiredAdViewedEvent(ad, ad_events)) {
          BLOG(1,
               "Search result ad: Not allowed as already viewed uuid " << uuid);
          NotifySearchResultAdEventFailed(uuid, creative_instance_id,
                                          event_type);
          return;
        }

        if (event_type == mojom::SearchResultAdEventType::kViewed) {
          // TODO(tmancey): We need to fire an ad served event until search
          // result ads are served by the ads library
          FireEvent(uuid, creative_instance_id,
                    mojom::SearchResultAdEventType::kServed);
        }

        const auto ad_event =
            search_result_ads::AdEventFactory::Build(event_type);
        ad_event->FireEvent(ad);

        NotifySearchResultAdEvent(ad, event_type);
      });
}

void SearchResultAd::NotifySearchResultAdEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type) const {
  switch (event_type) {
    case mojom::SearchResultAdEventType::kServed: {
      NotifySearchResultAdServed(ad);
      break;
    }

    case mojom::SearchResultAdEventType::kViewed: {
      NotifySearchResultAdViewed(ad);
      break;
    }

    case mojom::SearchResultAdEventType::kClicked: {
      NotifySearchResultAdClicked(ad);
      break;
    }
  }
}

void SearchResultAd::NotifySearchResultAdServed(
    const SearchResultAdInfo& ad) const {
  for (SearchResultAdObserver& observer : observers_) {
    observer.OnSearchResultAdServed(ad);
  }
}

void SearchResultAd::NotifySearchResultAdViewed(
    const SearchResultAdInfo& ad) const {
  for (SearchResultAdObserver& observer : observers_) {
    observer.OnSearchResultAdViewed(ad);
  }
}

void SearchResultAd::NotifySearchResultAdClicked(
    const SearchResultAdInfo& ad) const {
  for (SearchResultAdObserver& observer : observers_) {
    observer.OnSearchResultAdClicked(ad);
  }
}

void SearchResultAd::NotifySearchResultAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::SearchResultAdEventType event_type) const {
  for (SearchResultAdObserver& observer : observers_) {
    observer.OnSearchResultAdEventFailed(uuid, creative_instance_id,
                                         event_type);
  }
}

}  // namespace ads
