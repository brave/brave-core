/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/account/deposits/deposit_builder.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/conversions/conversion_builder.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"
#include "bat/ads/public/interfaces/ads.mojom.h"  // IWYU pragma: keep

namespace ads::search_result_ads {

namespace {

bool ShouldDebounceViewedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kViewed &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed);
}

bool ShouldDebounceClickedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kClicked &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked);
}

bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kServed ||
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed);
}

bool IsAdPlaced(const AdInfo& ad,
                const AdEventList& ad_events,
                const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kServed ||
         event_type == mojom::SearchResultAdEventType::kViewed ||
         (HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed) &&
          HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::SearchResultAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return ShouldDebounceViewedAdEvent(ad, ad_events, event_type) ||
         ShouldDebounceClickedAdEvent(ad, ad_events, event_type) ||
         !IsAdPlaced(ad, ad_events, event_type);
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
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (event_type == mojom::SearchResultAdEventType::kServed &&
      !PermissionRules::HasPermission()) {
    BLOG(1, "Search result ad: Not allowed due to permission rules");
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServed: {
      FireEvent(ad, event_type, std::move(callback));
      break;
    }

    case mojom::SearchResultAdEventType::kViewed: {
      FireViewedEvent(std::move(ad_mojom), std::move(callback));
      break;
    }

    case mojom::SearchResultAdEventType::kClicked: {
      FireClickedEvent(ad, std::move(callback));
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
                     std::move(ad_mojom), std::move(callback)));
}

void EventHandler::OnSaveDeposits(mojom::SearchResultAdInfoPtr ad_mojom,
                                  FireAdEventHandlerCallback callback,
                                  const bool success) const {
  DCHECK(ad_mojom);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  if (!success) {
    BLOG(0, "Failed to save deposits state");
    return FailedToFireEvent(ad, mojom::SearchResultAdEventType::kViewed,
                             std::move(callback));
  }

  BLOG(3, "Successfully saved deposits state");

  ConversionList conversions;
  if (const auto conversion = BuildConversion(ad_mojom)) {
    conversions.push_back(*conversion);
  }

  database::table::Conversions conversion_database_table;
  conversion_database_table.Save(
      conversions,
      base::BindOnce(&EventHandler::OnSaveConversions, base::Unretained(this),
                     ad, std::move(callback)));
}

void EventHandler::OnSaveConversions(const SearchResultAdInfo& ad,
                                     FireAdEventHandlerCallback callback,
                                     const bool success) const {
  if (!success) {
    BLOG(0, "Failed to save conversions state");
    return FailedToFireEvent(ad, mojom::SearchResultAdEventType::kViewed,
                             std::move(callback));
  }

  BLOG(3, "Successfully saved conversions state");

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      base::BindOnce(&EventHandler::OnGetAdEventsForViewedSearchResultAd,
                     base::Unretained(this), ad, std::move(callback)));
}

void EventHandler::OnGetAdEventsForViewedSearchResultAd(
    const SearchResultAdInfo& ad,
    FireAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) const {
  const mojom::SearchResultAdEventType event_type =
      mojom::SearchResultAdEventType::kViewed;

  if (!success) {
    BLOG(1, "Search result ad: Failed to get ad events");
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Search result ad: Not allowed because an ad was not served "
         "for placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Search result ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  FireEvent(ad, event_type, std::move(callback));
}

void EventHandler::FireClickedEvent(const SearchResultAdInfo& ad,
                                    FireAdEventHandlerCallback callback) const {
  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      base::BindOnce(&EventHandler::OnGetAdEventsForClickedSearchResultAd,
                     base::Unretained(this), ad, std::move(callback)));
}

void EventHandler::OnGetAdEventsForClickedSearchResultAd(
    const SearchResultAdInfo& ad,
    FireAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) const {
  const mojom::SearchResultAdEventType event_type =
      mojom::SearchResultAdEventType::kClicked;

  if (!success) {
    BLOG(1, "Search result ad: Failed to get ad events");
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Search result ad: Not allowed because an ad was not served "
         "for placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Search result ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  FireEvent(ad, event_type, std::move(callback));
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

  std::move(callback).Run(/*success*/ true, ad.placement_id, event_type);
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

  std::move(callback).Run(/*success*/ false, ad.placement_id, event_type);
}

}  // namespace ads::search_result_ads
