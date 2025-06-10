/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_value_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"

namespace brave_ads {

AdHistoryManager::AdHistoryManager() = default;

AdHistoryManager::~AdHistoryManager() = default;

// static
AdHistoryManager& AdHistoryManager::GetInstance() {
  return GlobalState::GetInstance()->GetHistoryManager();
}

void AdHistoryManager::AddObserver(AdHistoryManagerObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void AdHistoryManager::RemoveObserver(
    AdHistoryManagerObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

// static
void AdHistoryManager::Get(base::Time from_time,
                           base::Time to_time,
                           GetAdHistoryCallback callback) {
  database::table::AdHistory database_table;
  database_table.GetForDateRange(from_time, to_time, std::move(callback));
}

// static
void AdHistoryManager::GetForUI(base::Time from_time,
                                base::Time to_time,
                                GetAdHistoryForUICallback callback) {
  database::table::AdHistory database_table;
  database_table.GetHighestRankedPlacementsForDateRange(
      from_time, to_time,
      base::BindOnce(&GetForUICallback, std::move(callback)));
}

void AdHistoryManager::Add(
    const InlineContentAdInfo& ad,
    mojom::ConfirmationType mojom_confirmation_type) const {
  MaybeAdd(ad, mojom_confirmation_type, ad.title, ad.description);
}

void AdHistoryManager::Add(
    const NewTabPageAdInfo& ad,
    mojom::ConfirmationType mojom_confirmation_type) const {
  MaybeAdd(ad, mojom_confirmation_type, ad.company_name, ad.alt);
}

void AdHistoryManager::Add(
    const NotificationAdInfo& ad,
    mojom::ConfirmationType mojom_confirmation_type) const {
  MaybeAdd(ad, mojom_confirmation_type, ad.title, ad.body);
}

void AdHistoryManager::Add(
    const PromotedContentAdInfo& ad,
    mojom::ConfirmationType mojom_confirmation_type) const {
  MaybeAdd(ad, mojom_confirmation_type, ad.title, ad.description);
}

void AdHistoryManager::Add(
    const SearchResultAdInfo& ad,
    mojom::ConfirmationType mojom_confirmation_type) const {
  MaybeAdd(ad, mojom_confirmation_type, ad.headline_text, ad.description);
}

///////////////////////////////////////////////////////////////////////////////

void AdHistoryManager::MaybeAdd(const AdInfo& ad,
                                mojom::ConfirmationType mojom_confirmation_type,
                                const std::string& title,
                                const std::string& description) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return;
  }

  const AdHistoryItemInfo ad_history_item =
      BuildAdHistoryItem(ad, mojom_confirmation_type, title, description);
  database::SaveAdHistory({ad_history_item});

  NotifyDidAddAdHistoryItem(ad_history_item);
}

// static
void AdHistoryManager::GetForUICallback(
    GetAdHistoryForUICallback callback,
    std::optional<AdHistoryList> ad_history) {
  if (!ad_history) {
    return std::move(callback).Run(/*ad_history=*/std::nullopt);
  }

  std::move(callback).Run(AdHistoryToValue(*ad_history));
}

void AdHistoryManager::NotifyDidAddAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidAddAdHistoryItem(ad_history_item);
  }
}

}  // namespace brave_ads
