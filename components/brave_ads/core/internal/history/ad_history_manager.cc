/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"

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
void AdHistoryManager::Get(const base::Time from_time,
                           const base::Time to_time,
                           GetAdHistoryCallback callback) {
  database::table::AdHistory database_table;
  database_table.GetForDateRange(from_time, to_time, std::move(callback));
}

// static
void AdHistoryManager::GetForUI(const base::Time from_time,
                                const base::Time to_time,
                                GetAdHistoryForUICallback callback) {
  database::table::AdHistory database_table;
  database_table.GetHighestRankedPlacementsForDateRange(
      from_time, to_time,
      base::BindOnce(&GetForUICallback, std::move(callback)));
}

void AdHistoryManager::Add(const InlineContentAdInfo& ad,
                           const ConfirmationType confirmation_type) const {
  MaybeAdd(ad, confirmation_type, ad.title, ad.description);
}

void AdHistoryManager::Add(const NewTabPageAdInfo& ad,
                           const ConfirmationType confirmation_type) const {
  MaybeAdd(ad, confirmation_type, ad.company_name, ad.alt);
}

void AdHistoryManager::Add(const NotificationAdInfo& ad,
                           const ConfirmationType confirmation_type) const {
  MaybeAdd(ad, confirmation_type, ad.title, ad.body);
}

void AdHistoryManager::Add(const PromotedContentAdInfo& ad,
                           const ConfirmationType confirmation_type) const {
  MaybeAdd(ad, confirmation_type, ad.title, ad.description);
}

void AdHistoryManager::Add(const SearchResultAdInfo& ad,
                           const ConfirmationType confirmation_type) const {
  MaybeAdd(ad, confirmation_type, ad.headline_text, ad.description);
}

void AdHistoryManager::LikeAd(const AdHistoryItemInfo& ad_history_item,
                              ToggleReactionCallback callback) const {
  NotifyDidLikeAd(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::DislikeAd(const AdHistoryItemInfo& ad_history_item,
                                 ToggleReactionCallback callback) const {
  NotifyDidDislikeAd(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::LikeSegment(const AdHistoryItemInfo& ad_history_item,
                                   ToggleReactionCallback callback) const {
  NotifyDidLikeSegment(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::DislikeSegment(const AdHistoryItemInfo& ad_history_item,
                                      ToggleReactionCallback callback) const {
  NotifyDidDislikeSegment(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::ToggleSaveAd(const AdHistoryItemInfo& ad_history_item,
                                    ToggleReactionCallback callback) const {
  NotifyDidToggleSaveAd(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::ToggleMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item,
    ToggleReactionCallback callback) const {
  NotifyDidToggleMarkAdAsInappropriate(ad_history_item);
  std::move(callback).Run(/*success=*/true);
}

///////////////////////////////////////////////////////////////////////////////

void AdHistoryManager::MaybeAdd(const AdInfo& ad,
                                const ConfirmationType confirmation_type,
                                const std::string& title,
                                const std::string& description) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return;
  }

  const AdHistoryItemInfo ad_history_item =
      BuildAdHistoryItem(ad, confirmation_type, title, description);
  SaveAdHistory({ad_history_item});

  NotifyDidAddAdHistoryItem(ad_history_item);
}

// static
void AdHistoryManager::GetForUICallback(
    GetAdHistoryForUICallback callback,
    const std::optional<AdHistoryList>& ad_history) {
  if (!ad_history) {
    return std::move(callback).Run(/*ad_history*/ std::nullopt);
  }

  std::move(callback).Run(AdHistoryToValue(*ad_history));
}

void AdHistoryManager::NotifyDidAddAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidAddAdHistoryItem(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidLikeAd(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidLikeAd(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidDislikeAd(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidDislikeAd(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidLikeSegment(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidLikeSegment(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidDislikeSegment(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidDislikeSegment(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidToggleSaveAd(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidToggleSaveAd(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidToggleMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidToggleMarkAdAsInappropriate(ad_history_item);
  }
}

}  // namespace brave_ads
