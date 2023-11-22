/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/search_result_ad/search_result_ad_handler.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

SearchResultAd* g_deferred_search_result_ad_for_testing = nullptr;
bool g_defer_triggering_of_ad_viewed_event_for_testing = false;

void FireEventCallback(TriggerAdEventCallback callback,
                       const bool success,
                       const std::string& /*placement_id=*/,
                       const mojom::SearchResultAdEventType /*event_type=*/) {
  std::move(callback).Run(success);
}

}  // namespace

SearchResultAd::SearchResultAd(Account& account, Transfer& transfer)
    : account_(account), transfer_(transfer) {
  event_handler_.SetDelegate(this);
}

SearchResultAd::~SearchResultAd() = default;

void SearchResultAd::TriggerEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_NE(mojom::SearchResultAdEventType::kServed, event_type)
      << "Should not be called with kServed as this event is handled when "
         "calling TriggerEvent with kViewed";

  if (!UserHasJoinedBraveRewards() &&
      !ShouldAlwaysTriggerSearchResultAdEvents()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (event_type == mojom::SearchResultAdEventType::kViewed) {
    mojom::SearchResultAdInfoPtr ad_mojom_copy = ad_mojom.Clone();
    return event_handler_.FireEvent(
        std::move(ad_mojom_copy), mojom::SearchResultAdEventType::kServed,
        base::BindOnce(&SearchResultAd::FireServedEventCallback,
                       weak_factory_.GetWeakPtr(), std::move(ad_mojom),
                       std::move(callback)));
  }

  event_handler_.FireEvent(
      std::move(ad_mojom), event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

// static
void SearchResultAd::DeferTriggeringOfAdViewedEvent() {
  CHECK(!g_defer_triggering_of_ad_viewed_event_for_testing);
  g_defer_triggering_of_ad_viewed_event_for_testing = true;
}

// static
void SearchResultAd::TriggerDeferredAdViewedEvent() {
  CHECK(g_defer_triggering_of_ad_viewed_event_for_testing);
  CHECK(g_deferred_search_result_ad_for_testing);
  g_defer_triggering_of_ad_viewed_event_for_testing = false;
  g_deferred_search_result_ad_for_testing
      ->trigger_ad_viewed_event_in_progress_ = false;
  g_deferred_search_result_ad_for_testing->MaybeTriggerAdViewedEventFromQueue(
      /*intentional*/ base::DoNothing());
  g_deferred_search_result_ad_for_testing = nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAd::FireServedEventCallback(
    mojom::SearchResultAdInfoPtr ad_mojom,
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& /*placement_id=*/,
    const mojom::SearchResultAdEventType /*event_type=*/) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_viewed_event_queue_.push_front(std::move(ad_mojom));
  MaybeTriggerAdViewedEventFromQueue(std::move(callback));
}

void SearchResultAd::MaybeTriggerAdViewedEventFromQueue(
    TriggerAdEventCallback callback) {
  CHECK((!ad_viewed_event_queue_.empty() ||
         !trigger_ad_viewed_event_in_progress_));

  if (ad_viewed_event_queue_.empty() || trigger_ad_viewed_event_in_progress_) {
    return std::move(callback).Run(/*success=*/true);
  }
  trigger_ad_viewed_event_in_progress_ = true;

  mojom::SearchResultAdInfoPtr ad_mojom =
      std::move(ad_viewed_event_queue_.back());
  ad_viewed_event_queue_.pop_back();

  event_handler_.FireEvent(
      std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed,
      base::BindOnce(&SearchResultAd::FireAdViewedEventCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SearchResultAd::FireAdViewedEventCallback(
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& /*placement_id=*/,
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_EQ(event_type, mojom::SearchResultAdEventType::kViewed);

  if (g_defer_triggering_of_ad_viewed_event_for_testing) {
    g_deferred_search_result_ad_for_testing = this;
    return std::move(callback).Run(success);
  }

  trigger_ad_viewed_event_in_progress_ = false;
  MaybeTriggerAdViewedEventFromQueue(std::move(callback));
}

void SearchResultAd::OnDidFireSearchResultAdServedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Served search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);
}

void SearchResultAd::OnDidFireSearchResultAdViewedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Viewed search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewed);
}

void SearchResultAd::OnDidFireSearchResultAdClickedEvent(
    const SearchResultAdInfo& ad) {
  BLOG(3, "Clicked search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads
