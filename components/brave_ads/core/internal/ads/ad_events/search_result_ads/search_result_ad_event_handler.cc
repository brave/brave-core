/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_builder.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"

namespace brave_ads {

namespace {

bool ShouldDebounceViewedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::SearchResultAdEventType& event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kViewed &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed);
}

bool ShouldDebounceClickedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::SearchResultAdEventType& event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kClicked &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked);
}

bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const mojom::SearchResultAdEventType& event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kServed ||
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed);
}

bool IsAdPlaced(const AdInfo& ad,
                const AdEventList& ad_events,
                const mojom::SearchResultAdEventType& event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::SearchResultAdEventType::kServed ||
         event_type == mojom::SearchResultAdEventType::kViewed ||
         (HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed) &&
          HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::SearchResultAdEventType& event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  return ShouldDebounceViewedAdEvent(ad, ad_events, event_type) ||
         ShouldDebounceClickedAdEvent(ad, ad_events, event_type) ||
         !IsAdPlaced(ad, ad_events, event_type);
}

}  // namespace

SearchResultAdEventHandler::SearchResultAdEventHandler() = default;

SearchResultAdEventHandler::~SearchResultAdEventHandler() {
  delegate_ = nullptr;
}

void SearchResultAdEventHandler::FireEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  CHECK(ad_mojom);
  CHECK(mojom::IsKnownEnumValue(event_type));

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  if (!ad.IsValid()) {
    BLOG(1, "Failed to fire event due to an invalid search result ad");
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (event_type == mojom::SearchResultAdEventType::kServed &&
      !SearchResultAdPermissionRules::HasPermission()) {
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

void SearchResultAdEventHandler::FireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  const auto ad_event = SearchResultAdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  SuccessfullyFiredEvent(ad, event_type, std::move(callback));
}

void SearchResultAdEventHandler::FireViewedEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    FireAdEventHandlerCallback callback) const {
  CHECK(ad_mojom);

  const DepositInfo deposit = BuildDeposit(ad_mojom);

  database::table::Deposits deposits_database_table;
  deposits_database_table.Save(
      deposit, base::BindOnce(&SearchResultAdEventHandler::SaveDepositsCallback,
                              weak_factory_.GetWeakPtr(), std::move(ad_mojom),
                              std::move(callback)));
}

void SearchResultAdEventHandler::SaveDepositsCallback(
    mojom::SearchResultAdInfoPtr ad_mojom,
    FireAdEventHandlerCallback callback,
    const bool success) const {
  CHECK(ad_mojom);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  if (!success) {
    BLOG(0, "Failed to save deposits state");
    return FailedToFireEvent(ad, mojom::SearchResultAdEventType::kViewed,
                             std::move(callback));
  }

  BLOG(3, "Successfully saved deposits state");

  ConversionList conversions;
  if (const absl::optional<ConversionInfo> conversion =
          BuildConversion(ad_mojom)) {
    conversions.push_back(*conversion);
  }

  database::table::Conversions conversion_database_table;
  conversion_database_table.Save(
      conversions,
      base::BindOnce(&SearchResultAdEventHandler::SaveConversionsCallback,
                     weak_factory_.GetWeakPtr(), ad, std::move(callback)));
}

void SearchResultAdEventHandler::SaveConversionsCallback(
    const SearchResultAdInfo& ad,
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
      base::BindOnce(&SearchResultAdEventHandler::
                         GetAdEventsForViewedSearchResultAdCallback,
                     weak_factory_.GetWeakPtr(), ad, std::move(callback)));
}

void SearchResultAdEventHandler::GetAdEventsForViewedSearchResultAdCallback(
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

void SearchResultAdEventHandler::FireClickedEvent(
    const SearchResultAdInfo& ad,
    FireAdEventHandlerCallback callback) const {
  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kSearchResultAd,
      base::BindOnce(&SearchResultAdEventHandler::
                         GetAdEventsForClickedSearchResultAdCallback,
                     weak_factory_.GetWeakPtr(), ad, std::move(callback)));
}

void SearchResultAdEventHandler::GetAdEventsForClickedSearchResultAdCallback(
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

void SearchResultAdEventHandler::SuccessfullyFiredEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (delegate_) {
    switch (event_type) {
      case mojom::SearchResultAdEventType::kServed: {
        delegate_->OnDidFireSearchResultAdServedEvent(ad);
        break;
      }

      case mojom::SearchResultAdEventType::kViewed: {
        delegate_->OnDidFireSearchResultAdViewedEvent(ad);
        break;
      }

      case mojom::SearchResultAdEventType::kClicked: {
        delegate_->OnDidFireSearchResultAdClickedEvent(ad);
        break;
      }
    }
  }

  std::move(callback).Run(/*success*/ true, ad.placement_id, event_type);
}

void SearchResultAdEventHandler::FailedToFireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire search result ad "
              << event_type << " event for placement_id " << ad.placement_id
              << " and creative instance id " << ad.creative_instance_id);

  if (delegate_) {
    delegate_->OnFailedToFireSearchResultAdEvent(ad, event_type);
  }

  std::move(callback).Run(/*success*/ false, ad.placement_id, event_type);
}

}  // namespace brave_ads
