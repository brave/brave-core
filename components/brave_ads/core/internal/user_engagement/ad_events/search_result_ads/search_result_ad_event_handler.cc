/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <utility>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_builder.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_factory.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep

namespace brave_ads {

namespace {

void CompletedCallback(mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                       FireSearchResultAdEventHandlerCallback callback,
                       const bool success,
                       const std::string& placement_id,
                       const mojom::SearchResultAdEventType event_type) {
  if (success) {
    MaybeBuildAndSaveCreativeSetConversion(mojom_creative_ad, event_type);
  }

  std::move(callback).Run(success, placement_id, event_type);
}

}  // namespace

SearchResultAdEventHandler::SearchResultAdEventHandler() = default;

SearchResultAdEventHandler::~SearchResultAdEventHandler() {
  delegate_ = nullptr;
}

void SearchResultAdEventHandler::FireEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback) const {
  CHECK(mojom_creative_ad);

  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);
  if (!ad.IsValid()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid search result ad");
    base::debug::DumpWithoutCrashing();

    BLOG(1,
         "Failed to fire search result ad event due to the ad being invalid");

    return FailedToFireEvent(
        ad, event_type,
        base::BindOnce(&CompletedCallback, std::move(mojom_creative_ad),
                       std::move(callback)));
  }

  if (!CanFireEvent(mojom_creative_ad, event_type)) {
    return FailedToFireEvent(
        ad, event_type,
        base::BindOnce(&CompletedCallback, std::move(mojom_creative_ad),
                       std::move(callback)));
  }

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServedImpression: {
      MaybeFireServedEvent(
          ad, base::BindOnce(&CompletedCallback, std::move(mojom_creative_ad),
                             std::move(callback)));
      break;
    }

    case mojom::SearchResultAdEventType::kViewedImpression: {
      MaybeFireViewedEvent(
          ad, FromMojomBuildDeposit(mojom_creative_ad),
          base::BindOnce(&CompletedCallback, std::move(mojom_creative_ad),
                         std::move(callback)));
      break;
    }

    case mojom::SearchResultAdEventType::kClicked: {
      MaybeFireEvent(
          ad, mojom::SearchResultAdEventType::kClicked,
          base::BindOnce(&CompletedCallback, std::move(mojom_creative_ad),
                         std::move(callback)));
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAdEventHandler::FireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback) const {
  const auto ad_event = SearchResultAdEventFactory::Build(event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&SearchResultAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, event_type,
                         std::move(callback)));
}

void SearchResultAdEventHandler::FireEventCallback(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback,
    const bool success) const {
  success ? SuccessfullyFiredEvent(ad, event_type, std::move(callback))
          : FailedToFireEvent(ad, event_type, std::move(callback));
}

void SearchResultAdEventHandler::MaybeFireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback) const {
  ad_events_database_table_.GetUnexpiredForType(
      mojom::AdType::kSearchResultAd,
      base::BindOnce(&SearchResultAdEventHandler::MaybeFireEventCallback,
                     weak_factory_.GetWeakPtr(), ad, event_type,
                     std::move(callback)));
}

void SearchResultAdEventHandler::MaybeFireEventCallback(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) const {
  if (!success) {
    BLOG(1, "Search result ad: Failed to get ad events");
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (UserHasJoinedBraveRewards() && !WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Search result ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  if (ShouldDeduplicateAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Search result ad: Not allowed as deduplicated "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad, event_type, std::move(callback));
  }

  FireEvent(ad, event_type, std::move(callback));
}

void SearchResultAdEventHandler::MaybeFireServedEvent(
    const SearchResultAdInfo& ad,
    FireSearchResultAdEventHandlerCallback callback) const {
  if (!SearchResultAdPermissionRules::HasPermission()) {
    BLOG(1, "Search result ad: Not allowed due to permission rules");
    return FailedToFireEvent(ad,
                             mojom::SearchResultAdEventType::kServedImpression,
                             std::move(callback));
  }

  FireEvent(ad, mojom::SearchResultAdEventType::kServedImpression,
            std::move(callback));
}

void SearchResultAdEventHandler::MaybeFireViewedEvent(
    const SearchResultAdInfo& ad,
    const DepositInfo& deposit,
    FireSearchResultAdEventHandlerCallback callback) const {
  database::table::Deposits database_table;
  database_table.Save(
      deposit,
      base::BindOnce(&SearchResultAdEventHandler::MaybeFireViewedEventCallback,
                     weak_factory_.GetWeakPtr(), ad, std::move(callback)));
}

void SearchResultAdEventHandler::MaybeFireViewedEventCallback(
    const SearchResultAdInfo& ad,
    FireSearchResultAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Failed to save search result ad deposit");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to save search result ad deposit");

    return FailedToFireEvent(ad,
                             mojom::SearchResultAdEventType::kViewedImpression,
                             std::move(callback));
  }

  BLOG(3, "Successfully saved search result ad deposit");

  MaybeFireEvent(ad, mojom::SearchResultAdEventType::kViewedImpression,
                 std::move(callback));
}

void SearchResultAdEventHandler::SuccessfullyFiredEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback) const {
  NotifyDidFireSearchResultAdEvent(ad, event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id, event_type);
}

void SearchResultAdEventHandler::FailedToFireEvent(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type,
    FireSearchResultAdEventHandlerCallback callback) const {
  BLOG(1, "Failed to fire search result ad "
              << event_type << " event for placement_id " << ad.placement_id
              << " and creative instance id " << ad.creative_instance_id);

  NotifyFailedToFireSearchResultAdEvent(ad, event_type);

  std::move(callback).Run(/*success=*/false, ad.placement_id, event_type);
}

void SearchResultAdEventHandler::NotifyDidFireSearchResultAdEvent(
    const SearchResultAdInfo& ad,
    mojom::SearchResultAdEventType event_type) const {
  if (!delegate_) {
    return;
  }

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServedImpression: {
      delegate_->OnDidFireSearchResultAdServedEvent(ad);
      break;
    }

    case mojom::SearchResultAdEventType::kViewedImpression: {
      delegate_->OnDidFireSearchResultAdViewedEvent(ad);
      break;
    }

    case mojom::SearchResultAdEventType::kClicked: {
      delegate_->OnDidFireSearchResultAdClickedEvent(ad);
      break;
    }
  }
}

void SearchResultAdEventHandler::NotifyFailedToFireSearchResultAdEvent(
    const SearchResultAdInfo& ad,
    mojom::SearchResultAdEventType event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFireSearchResultAdEvent(ad, event_type);
  }
}

}  // namespace brave_ads
