/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/check.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/account/deposits/deposit_builder.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/conversions/conversion_builder.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"

namespace ads {
namespace search_result_ads {

namespace {

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (event_type == mojom::SearchResultAdEventType::kViewed &&
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed)) {
    return true;
  } else if (event_type == mojom::SearchResultAdEventType::kClicked &&
             HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked)) {
    return true;
  }

  return false;
}

}  // namespace

EventHandler::EventHandler() = default;

EventHandler::~EventHandler() = default;

void EventHandler::AddObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void EventHandler::RemoveObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void EventHandler::FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                             const mojom::SearchResultAdEventType event_type,
                             FireAdEventHandlerCallback callback) const {
  DCHECK(ad_mojom);
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  if (!ad.IsValid()) {
    BLOG(1, "Failed to fire event due to an invalid search result ad");
    FailedToFireEvent(ad, event_type, callback);
    return;
  }

  PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Search result ad: Not allowed due to permission rules");
    FailedToFireEvent(ad, event_type, callback);
    return;
  }

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServed: {
      FireEvent(ad, event_type, callback);
      break;
    }

    case mojom::SearchResultAdEventType::kViewed: {
      FireViewedEvent(std::move(ad_mojom), callback);
      break;
    }

    case mojom::SearchResultAdEventType::kClicked: {
      FireClickedEvent(ad, callback);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::FireEvent(const SearchResultAdInfo& ad,
                             const mojom::SearchResultAdEventType event_type,
                             FireAdEventHandlerCallback callback) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const auto ad_event = AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifySearchResultAdEvent(ad, event_type, std::move(callback));
}

void EventHandler::FireViewedEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                                   FireAdEventHandlerCallback callback) const {
  DCHECK(ad_mojom);

  const DepositInfo deposit = BuildDeposit(ad_mojom);

  database::table::Deposits deposits_database_table;
  deposits_database_table.Save(
      deposit,
      base::BindOnce(&EventHandler::OnSaveDeposits, base::Unretained(this),
                     std::move(ad_mojom), callback));
}

void EventHandler::OnSaveDeposits(mojom::SearchResultAdInfoPtr ad_mojom,
                                  FireAdEventHandlerCallback callback,
                                  const bool success) const {
  DCHECK(ad_mojom);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  if (!success) {
    BLOG(0, "Failed to save deposits state");
    FailedToFireEvent(ad, mojom::SearchResultAdEventType::kViewed, callback);
    return;
  }

  BLOG(3, "Successfully saved deposits state");

  ConversionList conversions;
  if (const auto conversion = BuildConversion(ad_mojom)) {
    conversions.push_back(*conversion);
  }

  database::table::Conversions conversion_database_table;
  conversion_database_table.Save(
      conversions, base::BindOnce(&EventHandler::OnSaveConversions,
                                  base::Unretained(this), ad, callback));
}

void EventHandler::OnSaveConversions(const SearchResultAdInfo& ad,
                                     FireAdEventHandlerCallback callback,
                                     const bool success) const {
  if (!success) {
    BLOG(0, "Failed to save conversions state");
    FailedToFireEvent(ad, mojom::SearchResultAdEventType::kViewed, callback);
    return;
  }

  BLOG(3, "Successfully saved conversions state");

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      [=](const bool success, const AdEventList& ad_events) {
        const mojom::SearchResultAdEventType event_type =
            mojom::SearchResultAdEventType::kViewed;

        if (!success) {
          BLOG(1, "Search result ad: Failed to get ad events");
          FailedToFireEvent(ad, event_type, callback);
          return;
        }

        if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
          BLOG(1, "Search result ad: Not allowed as already fired "
                      << event_type << " event for this placement id "
                      << ad.placement_id);
          FailedToFireEvent(ad, event_type, callback);
          return;
        }

        // We must fire an ad served event due to search result ads not
        // being delivered by the library.
        FireEvent(ad, mojom::SearchResultAdEventType::kServed, callback);

        FireEvent(ad, event_type, callback);
      });
}

void EventHandler::FireClickedEvent(const SearchResultAdInfo& ad,
                                    FireAdEventHandlerCallback callback) const {
  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      [=](const bool success, const AdEventList& ad_events) {
        const mojom::SearchResultAdEventType event_type =
            mojom::SearchResultAdEventType::kClicked;

        if (!success) {
          BLOG(1, "Search result ad: Failed to get ad events");
          FailedToFireEvent(ad, event_type, callback);
          return;
        }

        if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
          BLOG(1, "Search result ad: Not allowed as already fired "
                      << event_type << " event for this placement id "
                      << ad.placement_id);
          FailedToFireEvent(ad, event_type, callback);
          return;
        }

        FireEvent(ad, event_type, callback);
      });
}

void EventHandler::FailedToFireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire search result ad "
              << event_type << " event for placement_id " << ad.placement_id
              << " and creative instance id " << ad.creative_instance_id);

  NotifySearchResultAdEventFailed(ad, event_type, std::move(callback));
}

void EventHandler::NotifySearchResultAdEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

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

void EventHandler::NotifySearchResultAdServed(
    const SearchResultAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnSearchResultAdServed(ad);
  }
}

void EventHandler::NotifySearchResultAdViewed(
    const SearchResultAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnSearchResultAdViewed(ad);
  }
}

void EventHandler::NotifySearchResultAdClicked(
    const SearchResultAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnSearchResultAdClicked(ad);
  }
}

void EventHandler::NotifySearchResultAdEventFailed(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  for (EventHandlerObserver& observer : observers_) {
    observer.OnSearchResultAdEventFailed(ad, event_type);
  }

  callback(/* success */ false, ad.placement_id, event_type);
}

}  // namespace search_result_ads
}  // namespace ads
