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

namespace {

mojom::AdHistoryItemInfoPtr AdHistoryItemToMojom(
    const AdHistoryItemInfo& ad_history_item,
    const Reactions& reactions) {
  auto mojom_ad_history_item = mojom::AdHistoryItemInfo::New();
  mojom_ad_history_item->created_at = ad_history_item.created_at;
  mojom_ad_history_item->type = ad_history_item.type;
  mojom_ad_history_item->confirmation_type = ad_history_item.confirmation_type;
  mojom_ad_history_item->placement_id = ad_history_item.placement_id;
  mojom_ad_history_item->creative_instance_id =
      ad_history_item.creative_instance_id;
  mojom_ad_history_item->creative_set_id = ad_history_item.creative_set_id;
  mojom_ad_history_item->campaign_id = ad_history_item.campaign_id;
  mojom_ad_history_item->advertiser_id = ad_history_item.advertiser_id;
  mojom_ad_history_item->segment = ad_history_item.segment;
  mojom_ad_history_item->title = ad_history_item.title;
  mojom_ad_history_item->description = ad_history_item.description;
  mojom_ad_history_item->target_url = ad_history_item.target_url;
  mojom_ad_history_item->like_ad_reaction =
      reactions.AdReactionTypeForId(ad_history_item.advertiser_id);
  mojom_ad_history_item->is_saved =
      reactions.IsAdSaved(ad_history_item.creative_instance_id);
  mojom_ad_history_item->is_flagged =
      reactions.IsAdMarkedAsInappropriate(ad_history_item.creative_set_id);
  mojom_ad_history_item->like_segment_reaction =
      reactions.SegmentReactionTypeForId(ad_history_item.segment);
  return mojom_ad_history_item;
}

std::vector<mojom::AdHistoryItemInfoPtr> AdHistoryToMojom(
    const AdHistoryList& ad_history,
    const Reactions& reactions) {
  std::vector<mojom::AdHistoryItemInfoPtr> mojom_ad_history;
  mojom_ad_history.reserve(ad_history.size());
  for (const AdHistoryItemInfo& ad_history_item : ad_history) {
    mojom_ad_history.push_back(
        AdHistoryItemToMojom(ad_history_item, reactions));
  }
  return mojom_ad_history;
}

}  // namespace

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
    return std::move(callback).Run(/*mojom_ad_history=*/std::nullopt);
  }

  std::move(callback).Run(AdHistoryToMojom(*ad_history, GetReactions()));
}

void AdHistoryManager::NotifyDidAddAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
  observers_.Notify(&AdHistoryManagerObserver::OnDidAddAdHistoryItem,
                    ad_history_item);
}

}  // namespace brave_ads
