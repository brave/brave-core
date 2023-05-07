/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/search_result_ad_handler.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"

namespace brave_ads {

namespace {

SearchResultAd* g_deferred_search_result_ad_for_testing = nullptr;
bool g_defer_triggering_of_ad_viewed_event_for_testing = false;

}  // namespace

SearchResultAd::SearchResultAd(Account& account, Transfer& transfer)
    : account_(account), transfer_(transfer) {
  event_handler_.SetDelegate(this);
}

SearchResultAd::~SearchResultAd() = default;

void SearchResultAd::TriggerEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (event_type == mojom::SearchResultAdEventType::kViewed) {
    ad_viewed_event_queue_.push_front(std::move(ad_mojom));
    return MaybeTriggerAdViewedEventFromQueue();
  }

  event_handler_.FireEvent(
      std::move(ad_mojom), event_type,
      base::BindOnce([](const bool success, const std::string& placement_id,
                        const mojom::SearchResultAdEventType event_type) {
        // Intentionally do nothing.
      }));
}

// static
void SearchResultAd::DeferTriggeringOfAdViewedEventForTesting() {
  CHECK(!g_defer_triggering_of_ad_viewed_event_for_testing);
  g_defer_triggering_of_ad_viewed_event_for_testing = true;
}

// static
void SearchResultAd::TriggerDeferredAdViewedEventForTesting() {
  CHECK(g_defer_triggering_of_ad_viewed_event_for_testing);
  CHECK(g_deferred_search_result_ad_for_testing);
  g_defer_triggering_of_ad_viewed_event_for_testing = false;
  g_deferred_search_result_ad_for_testing
      ->trigger_ad_viewed_event_in_progress_ = false;
  g_deferred_search_result_ad_for_testing->MaybeTriggerAdViewedEventFromQueue();
  g_deferred_search_result_ad_for_testing = nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAd::MaybeTriggerAdViewedEventFromQueue() {
  CHECK((!ad_viewed_event_queue_.empty() ||
         !trigger_ad_viewed_event_in_progress_));

  if (ad_viewed_event_queue_.empty() || trigger_ad_viewed_event_in_progress_) {
    return;
  }
  trigger_ad_viewed_event_in_progress_ = true;

  mojom::SearchResultAdInfoPtr ad_mojom =
      std::move(ad_viewed_event_queue_.back());
  ad_viewed_event_queue_.pop_back();

  event_handler_.FireEvent(std::move(ad_mojom),
                           mojom::SearchResultAdEventType::kViewed,
                           base::BindOnce(&SearchResultAd::OnFireAdViewedEvent,
                                          weak_factory_.GetWeakPtr()));
}

void SearchResultAd::OnFireAdViewedEvent(
    const bool /*success*/,
    const std::string& /*placement_id*/,
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_EQ(event_type, mojom::SearchResultAdEventType::kViewed);

  if (g_defer_triggering_of_ad_viewed_event_for_testing) {
    g_deferred_search_result_ad_for_testing = this;
    return;
  }
  trigger_ad_viewed_event_in_progress_ = false;
  MaybeTriggerAdViewedEventFromQueue();
}

void SearchResultAd::OnDidFireSearchResultAdViewedEvent(
    const SearchResultAdInfo& ad) {
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kViewed);
}

void SearchResultAd::OnDidFireSearchResultAdClickedEvent(
    const SearchResultAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads
