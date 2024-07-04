/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

SearchResultAdHandler* g_deferred_search_result_ad_for_testing = nullptr;
bool g_defer_triggering_of_ad_viewed_event_for_testing = false;

void FireEventCallback(TriggerAdEventCallback callback,
                       const bool success,
                       const std::string& /*placement_id*/,
                       const mojom::SearchResultAdEventType /*event_type*/) {
  std::move(callback).Run(success);
}

}  // namespace

SearchResultAdHandler::SearchResultAdHandler(Account& account,
                                             SiteVisit& site_visit)
    : account_(account), site_visit_(site_visit) {
  event_handler_.SetDelegate(this);
}

SearchResultAdHandler::~SearchResultAdHandler() = default;

// static
void SearchResultAdHandler::DeferTriggeringOfAdViewedEventForTesting() {
  CHECK_IS_TEST();
  CHECK(!g_defer_triggering_of_ad_viewed_event_for_testing);

  g_defer_triggering_of_ad_viewed_event_for_testing = true;
}

// static
void SearchResultAdHandler::TriggerDeferredAdViewedEventForTesting() {
  CHECK_IS_TEST();
  CHECK(g_defer_triggering_of_ad_viewed_event_for_testing);
  CHECK(g_deferred_search_result_ad_for_testing);

  g_defer_triggering_of_ad_viewed_event_for_testing = false;

  g_deferred_search_result_ad_for_testing
      ->trigger_ad_viewed_event_in_progress_ = false;

  g_deferred_search_result_ad_for_testing->MaybeTriggerAdViewedEventFromQueue(
      /*intentional*/ base::DoNothing());
  g_deferred_search_result_ad_for_testing = nullptr;
}

void SearchResultAdHandler::TriggerEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK_NE(mojom::SearchResultAdEventType::kServedImpression, event_type)
      << "Should not be called with kServedImpression as this event is handled "
         "when calling TriggerEvent with kViewedImpression";

  if (!UserHasJoinedBraveRewards() &&
      !ShouldAlwaysTriggerSearchResultAdEvents()) {
    // No-op if we should not trigger events for non-Rewards users.
    return std::move(callback).Run(/*success=*/false);
  }

  if (event_type == mojom::SearchResultAdEventType::kViewedImpression) {
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad_copy =
        mojom_creative_ad.Clone();

    return event_handler_.FireEvent(
        std::move(mojom_creative_ad_copy),
        mojom::SearchResultAdEventType::kServedImpression,
        base::BindOnce(&SearchResultAdHandler::FireServedEventCallback,
                       weak_factory_.GetWeakPtr(), std::move(mojom_creative_ad),
                       std::move(callback)));
  }

  event_handler_.FireEvent(
      std::move(mojom_creative_ad), event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAdHandler::FireServedEventCallback(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& /*placement_id*/,
    const mojom::SearchResultAdEventType /*event_type*/) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_viewed_event_queue_.push_front(std::move(mojom_creative_ad));

  MaybeTriggerAdViewedEventFromQueue(std::move(callback));
}

void SearchResultAdHandler::MaybeTriggerAdViewedEventFromQueue(
    TriggerAdEventCallback callback) {
  CHECK((!ad_viewed_event_queue_.empty() ||
         !trigger_ad_viewed_event_in_progress_));

  if (ad_viewed_event_queue_.empty() || trigger_ad_viewed_event_in_progress_) {
    return std::move(callback).Run(/*success=*/true);
  }
  trigger_ad_viewed_event_in_progress_ = true;

  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      std::move(ad_viewed_event_queue_.back());
  ad_viewed_event_queue_.pop_back();

  event_handler_.FireEvent(
      std::move(mojom_creative_ad),
      mojom::SearchResultAdEventType::kViewedImpression,
      base::BindOnce(&SearchResultAdHandler::FireAdViewedEventCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SearchResultAdHandler::FireAdViewedEventCallback(
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& /*placement_id*/,
    const mojom::SearchResultAdEventType event_type) {
  CHECK_EQ(event_type, mojom::SearchResultAdEventType::kViewedImpression);

  if (g_defer_triggering_of_ad_viewed_event_for_testing) {
    CHECK_IS_TEST();

    g_deferred_search_result_ad_for_testing = this;
    return std::move(callback).Run(success);
  }

  trigger_ad_viewed_event_in_progress_ = false;
  MaybeTriggerAdViewedEventFromQueue(std::move(callback));
}

void SearchResultAdHandler::OnDidFireSearchResultAdServedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Served search result ad impression with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);
}

void SearchResultAdHandler::OnDidFireSearchResultAdViewedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Viewed search result ad impression with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewedImpression);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewedImpression);
}

void SearchResultAdHandler::OnDidFireSearchResultAdClickedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Clicked search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  site_visit_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads
