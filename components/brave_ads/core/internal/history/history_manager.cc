/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_manager.h"

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/filters/date_range_history_filter.h"
#include "brave/components/brave_ads/core/internal/history/filters/history_filter_factory.h"
#include "brave/components/brave_ads/core/internal/history/history_util.h"
#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_factory.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

namespace brave_ads {

HistoryManager::HistoryManager() = default;

HistoryManager::~HistoryManager() = default;

// static
HistoryManager& HistoryManager::GetInstance() {
  return GlobalState::GetInstance()->GetHistoryManager();
}

void HistoryManager::AddObserver(HistoryManagerObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void HistoryManager::RemoveObserver(HistoryManagerObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

// static
const HistoryItemList& HistoryManager::Get() {
  return ClientStateManager::GetInstance().GetHistory();
}

// static
HistoryItemList HistoryManager::Get(const HistoryFilterType filter_type,
                                    const HistorySortType sort_type,
                                    const base::Time from_time,
                                    const base::Time to_time) {
  HistoryItemList history_items = Get();

  const auto date_range_filter =
      std::make_unique<DateRangeHistoryFilter>(from_time, to_time);
  history_items = date_range_filter->Apply(history_items);

  const auto filter = HistoryFilterFactory::Build(filter_type);
  if (filter) {
    history_items = filter->Apply(history_items);
  }

  const auto sort = HistorySortFactory::Build(sort_type);
  if (sort) {
    history_items = sort->Apply(history_items);
  }

  return history_items;
}

void HistoryManager::Add(const InlineContentAdInfo& ad,
                         const ConfirmationType& confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  const HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.description);
  NotifyDidAddHistory(history_item);
}

void HistoryManager::Add(const NewTabPageAdInfo& ad,
                         const ConfirmationType& confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  const HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.company_name, ad.alt);
  NotifyDidAddHistory(history_item);
}

void HistoryManager::Add(const NotificationAdInfo& ad,
                         const ConfirmationType& confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  const HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.body);
  NotifyDidAddHistory(history_item);
}

void HistoryManager::Add(const PromotedContentAdInfo& ad,
                         const ConfirmationType& confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  const HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.description);
  NotifyDidAddHistory(history_item);
}

void HistoryManager::Add(const SearchResultAdInfo& ad,
                         const ConfirmationType& confirmation_type) const {
  if (!UserHasJoinedBraveRewards()) {
    return;
  }

  const HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.headline_text, ad.description);
  NotifyDidAddHistory(history_item);
}

mojom::UserReactionType HistoryManager::LikeAd(
    const AdContentInfo& ad_content) const {
  AdContentInfo mutable_ad_content = ad_content;
  mutable_ad_content.user_reaction_type =
      ClientStateManager::GetInstance().ToggleLikeAd(ad_content);

  if (mutable_ad_content.user_reaction_type == mojom::UserReactionType::kLike) {
    NotifyDidLikeAd(mutable_ad_content);
  }

  return mutable_ad_content.user_reaction_type;
}

mojom::UserReactionType HistoryManager::DislikeAd(
    const AdContentInfo& ad_content) const {
  AdContentInfo mutable_ad_content = ad_content;
  mutable_ad_content.user_reaction_type =
      ClientStateManager::GetInstance().ToggleDislikeAd(ad_content);

  if (mutable_ad_content.user_reaction_type ==
      mojom::UserReactionType::kDislike) {
    NotifyDidDislikeAd(mutable_ad_content);
  }

  return mutable_ad_content.user_reaction_type;
}

mojom::UserReactionType HistoryManager::LikeCategory(
    const CategoryContentInfo& category_content) const {
  const mojom::UserReactionType toggled_user_reaction_type =
      ClientStateManager::GetInstance().ToggleLikeCategory(category_content);
  if (toggled_user_reaction_type == mojom::UserReactionType::kLike) {
    NotifyDidLikeCategory(category_content.category);
  }

  return toggled_user_reaction_type;
}

mojom::UserReactionType HistoryManager::DislikeCategory(
    const CategoryContentInfo& category_content) const {
  const mojom::UserReactionType toggled_user_reaction_type =
      ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);
  if (toggled_user_reaction_type == mojom::UserReactionType::kDislike) {
    NotifyDidDislikeCategory(category_content.category);
  }

  return toggled_user_reaction_type;
}

bool HistoryManager::ToggleSaveAd(const AdContentInfo& ad_content) const {
  AdContentInfo mutable_ad_content = ad_content;
  mutable_ad_content.is_saved =
      ClientStateManager::GetInstance().ToggleSaveAd(ad_content);

  if (mutable_ad_content.is_saved) {
    NotifyDidSaveAd(mutable_ad_content);
  } else {
    NotifyDidUnsaveAd(mutable_ad_content);
  }

  return mutable_ad_content.is_saved;
}

bool HistoryManager::ToggleMarkAdAsInappropriate(
    const AdContentInfo& ad_content) const {
  AdContentInfo mutable_ad_content = ad_content;
  mutable_ad_content.is_flagged =
      ClientStateManager::GetInstance().ToggleMarkAdAsInappropriate(ad_content);

  if (mutable_ad_content.is_flagged) {
    NotifyDidMarkAdAsInappropriate(mutable_ad_content);
  } else {
    NotifyDidMarkAdAsAppropriate(mutable_ad_content);
  }

  return mutable_ad_content.is_flagged;
}

///////////////////////////////////////////////////////////////////////////////

void HistoryManager::NotifyDidAddHistory(
    const HistoryItemInfo& history_item) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidAddHistory(history_item);
  }
}

void HistoryManager::NotifyDidLikeAd(const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidLikeAd(ad_content);
  }
}

void HistoryManager::NotifyDidDislikeAd(const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidDislikeAd(ad_content);
  }
}

void HistoryManager::NotifyDidLikeCategory(const std::string& category) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidLikeCategory(category);
  }
}

void HistoryManager::NotifyDidDislikeCategory(
    const std::string& category) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidDislikeCategory(category);
  }
}

void HistoryManager::NotifyDidSaveAd(const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidSaveAd(ad_content);
  }
}

void HistoryManager::NotifyDidUnsaveAd(const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidUnsaveAd(ad_content);
  }
}

void HistoryManager::NotifyDidMarkAdAsInappropriate(
    const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkAdAsInappropriate(ad_content);
  }
}

void HistoryManager::NotifyDidMarkAdAsAppropriate(
    const AdContentInfo& ad_content) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkAdAsAppropriate(ad_content);
  }
}

}  // namespace brave_ads
