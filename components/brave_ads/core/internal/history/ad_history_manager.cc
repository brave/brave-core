/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

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

void AdHistoryManager::Add(const NewTabPageAdInfo& ad,
                           mojom::ConfirmationType mojom_confirmation_type) {
  MaybeAdd(ad, mojom_confirmation_type, ad.company_name, ad.alt);
}

void AdHistoryManager::Add(const NotificationAdInfo& ad,
                           mojom::ConfirmationType mojom_confirmation_type) {
  MaybeAdd(ad, mojom_confirmation_type, ad.title, ad.body);
}

void AdHistoryManager::Add(const SearchResultAdInfo& ad,
                           mojom::ConfirmationType mojom_confirmation_type) {
  MaybeAdd(ad, mojom_confirmation_type, ad.headline_text, ad.description);
}

///////////////////////////////////////////////////////////////////////////////

void AdHistoryManager::MaybeAdd(const AdInfo& ad,
                                mojom::ConfirmationType mojom_confirmation_type,
                                const std::string& title,
                                const std::string& description) {
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

  const Reactions& reactions = GetReactions();

  std::vector<mojom::AdHistoryItemInfoPtr> mojom_items;
  mojom_items.reserve(ad_history->size());
  for (const AdHistoryItemInfo& item : *ad_history) {
    auto mojom_item = mojom::AdHistoryItemInfo::New();
    mojom_item->created_at = item.created_at;
    mojom_item->type = item.type;
    mojom_item->confirmation_type = item.confirmation_type;
    mojom_item->placement_id = item.placement_id;
    mojom_item->creative_instance_id = item.creative_instance_id;
    mojom_item->creative_set_id = item.creative_set_id;
    mojom_item->campaign_id = item.campaign_id;
    mojom_item->advertiser_id = item.advertiser_id;
    mojom_item->segment = item.segment;
    mojom_item->title = item.title;
    mojom_item->description = item.description;
    mojom_item->target_url = item.target_url;
    mojom_item->like_ad_reaction =
        reactions.AdReactionTypeForId(item.advertiser_id);
    mojom_item->is_saved = reactions.IsAdSaved(item.creative_instance_id);
    mojom_item->is_flagged =
        reactions.IsAdMarkedAsInappropriate(item.creative_set_id);
    mojom_item->like_segment_reaction =
        reactions.SegmentReactionTypeForId(item.segment);
    mojom_items.push_back(std::move(mojom_item));
  }

  std::move(callback).Run(std::move(mojom_items));
}

void AdHistoryManager::NotifyDidAddAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
  observers_.Notify(&AdHistoryManagerObserver::OnDidAddAdHistoryItem,
                    ad_history_item);
}

}  // namespace brave_ads
