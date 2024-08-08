/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"
#include "brave/components/brave_ads/core/internal/history/filters/ad_history_date_range_filter.h"
#include "brave/components/brave_ads/core/internal/history/filters/ad_history_filter_factory.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ad_history_sort_factory.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
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
const AdHistoryList& AdHistoryManager::Get() {
  return ClientStateManager::GetInstance().GetAdHistory();
}

// static
AdHistoryList AdHistoryManager::Get(const AdHistoryFilterType filter_type,
                                    const AdHistorySortType sort_type,
                                    const base::Time from_time,
                                    const base::Time to_time) {
  AdHistoryList ad_history = Get();

  const auto date_range_filter =
      std::make_unique<AdHistoryDateRangeFilter>(from_time, to_time);
  date_range_filter->Apply(ad_history);

  if (const auto filter = AdHistoryFilterFactory::Build(filter_type)) {
    filter->Apply(ad_history);
  }

  if (const auto sort = AdHistorySortFactory::Build(sort_type)) {
    sort->Apply(ad_history);
  }

  return ad_history;
}

std::optional<AdHistoryItemInfo> AdHistoryManager::Add(
    const InlineContentAdInfo& ad,
    const ConfirmationType confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return std::nullopt;
  }

  AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, confirmation_type, ad.title, ad.description);
  NotifyDidAppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

std::optional<AdHistoryItemInfo> AdHistoryManager::Add(
    const NewTabPageAdInfo& ad,
    const ConfirmationType confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return std::nullopt;
  }

  AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, confirmation_type, ad.company_name, ad.alt);
  NotifyDidAppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

std::optional<AdHistoryItemInfo> AdHistoryManager::Add(
    const NotificationAdInfo& ad,
    const ConfirmationType confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return std::nullopt;
  }

  AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, confirmation_type, ad.title, ad.body);
  NotifyDidAppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

std::optional<AdHistoryItemInfo> AdHistoryManager::Add(
    const PromotedContentAdInfo& ad,
    const ConfirmationType confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return std::nullopt;
  }

  AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, confirmation_type, ad.title, ad.description);
  NotifyDidAppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

std::optional<AdHistoryItemInfo> AdHistoryManager::Add(
    const SearchResultAdInfo& ad,
    const ConfirmationType confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to add history.
    return std::nullopt;
  }

  AdHistoryItemInfo ad_history_item = AppendAdHistoryItem(
      ad, confirmation_type, ad.headline_text, ad.description);
  NotifyDidAppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

void AdHistoryManager::LikeAd(const AdHistoryItemInfo& ad_history_item,
                              ToggleReactionCallback callback) const {
  AdHistoryItemInfo mutable_ad_history_item = ad_history_item;
  mutable_ad_history_item.ad_reaction_type =
      ClientStateManager::GetInstance().ToggleLikeAd(mutable_ad_history_item);

  if (mutable_ad_history_item.ad_reaction_type == mojom::ReactionType::kLiked) {
    NotifyDidLikeAd(mutable_ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::DislikeAd(const AdHistoryItemInfo& ad_history_item,
                                 ToggleReactionCallback callback) const {
  AdHistoryItemInfo mutable_ad_history_item = ad_history_item;
  mutable_ad_history_item.ad_reaction_type =
      ClientStateManager::GetInstance().ToggleDislikeAd(ad_history_item);

  if (mutable_ad_history_item.ad_reaction_type ==
      mojom::ReactionType::kDisliked) {
    NotifyDidDislikeAd(mutable_ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::LikeSegment(const AdHistoryItemInfo& ad_history_item,
                                   ToggleReactionCallback callback) const {
  const mojom::ReactionType toggled_reaction_type =
      ClientStateManager::GetInstance().ToggleLikeSegment(ad_history_item);
  if (toggled_reaction_type == mojom::ReactionType::kLiked) {
    NotifyDidLikeSegment(ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::DislikeSegment(const AdHistoryItemInfo& ad_history_item,
                                      ToggleReactionCallback callback) const {
  const mojom::ReactionType toggled_reaction_type =
      ClientStateManager::GetInstance().ToggleDislikeSegment(ad_history_item);
  if (toggled_reaction_type == mojom::ReactionType::kDisliked) {
    NotifyDidDislikeSegment(ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::ToggleSaveAd(const AdHistoryItemInfo& ad_history_item,
                                    ToggleReactionCallback callback) const {
  AdHistoryItemInfo mutable_ad_history_item = ad_history_item;
  mutable_ad_history_item.is_saved =
      ClientStateManager::GetInstance().ToggleSaveAd(ad_history_item);

  if (mutable_ad_history_item.is_saved) {
    NotifyDidSaveAd(mutable_ad_history_item);
  } else {
    NotifyDidUnsaveAd(mutable_ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

void AdHistoryManager::ToggleMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item,
    ToggleReactionCallback callback) const {
  AdHistoryItemInfo mutable_ad_history_item = ad_history_item;
  mutable_ad_history_item.is_marked_as_inappropriate =
      ClientStateManager::GetInstance().ToggleMarkAdAsInappropriate(
          ad_history_item);

  if (mutable_ad_history_item.is_marked_as_inappropriate) {
    NotifyDidMarkAdAsInappropriate(mutable_ad_history_item);
  } else {
    NotifyDidMarkAdAsAppropriate(mutable_ad_history_item);
  }

  std::move(callback).Run(/*success=*/true);
}

///////////////////////////////////////////////////////////////////////////////

void AdHistoryManager::NotifyDidAppendAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidAppendAdHistoryItem(ad_history_item);
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

void AdHistoryManager::NotifyDidSaveAd(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidSaveAd(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidUnsaveAd(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidUnsaveAd(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkAdAsInappropriate(ad_history_item);
  }
}

void AdHistoryManager::NotifyDidMarkAdAsAppropriate(
    const AdHistoryItemInfo& ad_history_item) const {
  for (AdHistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkAdAsAppropriate(ad_history_item);
  }
}

}  // namespace brave_ads
