/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/search_result_ads/search_result_ad.h"

#include "base/check.h"
#include "bat/ads/internal/account/deposits/deposit_builder.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/conversions/conversion_builder.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_permission_rules.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
    const mojom::SearchResultAdPtr& ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) const {
  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);

  if (!ad.IsValid()) {
    BLOG(1, "Failed to fire event due to an invalid search result ad");
    NotifySearchResultAdEventFailed(ad, event_type, callback);
    return;
  }

  search_result_ads::frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Search result ad: Not allowed due to permission rules");
    NotifySearchResultAdEventFailed(ad, event_type, callback);
    return;
  }

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServed:
    case mojom::SearchResultAdEventType::kClicked: {
      FireEvent(ad, event_type, callback);
      break;
    }

    case mojom::SearchResultAdEventType::kViewed: {
      FireViewedEvent(ad_mojom, callback);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAd::FireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) const {
  const auto ad_event = search_result_ads::AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifySearchResultAdEvent(ad, event_type, callback);
}

void SearchResultAd::FireViewedEvent(
    const mojom::SearchResultAdPtr& ad_mojom,
    TriggerSearchResultAdEventCallback callback) const {
  const DepositInfo& deposit = BuildDeposit(ad_mojom);

  const absl::optional<ConversionInfo>& conversion_optional =
      BuildConversion(ad_mojom);

  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);

  database::table::Deposits deposits_database_table;
  deposits_database_table.Save(deposit, [=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to save deposits state");
      NotifySearchResultAdEventFailed(
          ad, mojom::SearchResultAdEventType::kViewed, callback);
      return;
    }

    BLOG(3, "Successfully saved deposits state");

    ConversionList conversions;
    if (conversion_optional) {
      const ConversionInfo& conversion = conversion_optional.value();
      conversions.push_back(conversion);
    }

    database::table::Conversions conversion_database_table;
    conversion_database_table.Save(conversions, [=](const bool success) {
      if (!success) {
        BLOG(0, "Failed to save conversions state");
        NotifySearchResultAdEventFailed(
            ad, mojom::SearchResultAdEventType::kViewed, callback);
        return;
      }

      BLOG(3, "Successfully saved conversions state");

      database::table::AdEvents database_table;
      database_table.GetForType(
          mojom::AdType::kSearchResultAd,
          [=](const bool success, const AdEventList& ad_events) {
            if (!success) {
              BLOG(1, "Search result ad: Failed to get ad events");
              NotifySearchResultAdEventFailed(
                  ad, mojom::SearchResultAdEventType::kViewed, callback);
              return;
            }

            if (HasFiredAdViewedEvent(ad, ad_events)) {
              BLOG(1,
                   "Search result ad: Not allowed as already fired a viewed "
                   "event for this placement id "
                       << ad.placement_id);
              NotifySearchResultAdEventFailed(
                  ad, mojom::SearchResultAdEventType::kViewed, callback);
              return;
            }

            // We must fire an ad served event due to search result ads not
            // being delivered by the library
            FireEvent(ad, mojom::SearchResultAdEventType::kServed, callback);

            FireEvent(ad, mojom::SearchResultAdEventType::kViewed, callback);
          });
    });
  });
}

void SearchResultAd::NotifySearchResultAdEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) const {
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

  callback(/* success */ true, ad.placement_id, event_type);
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
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) const {
  for (SearchResultAdObserver& observer : observers_) {
    observer.OnSearchResultAdEventFailed(ad, event_type);
  }

  callback(/* success */ false, ad.placement_id, event_type);
}

}  // namespace ads
