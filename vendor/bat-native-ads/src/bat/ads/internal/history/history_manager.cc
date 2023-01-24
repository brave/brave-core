/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_manager.h"

#include <memory>

#include "base/check_op.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "bat/ads/ad_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/history/filters/date_range_history_filter.h"
#include "bat/ads/internal/history/filters/history_filter_factory.h"
#include "bat/ads/internal/history/history_util.h"
#include "bat/ads/internal/history/sorts/history_sort_factory.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {

namespace {
HistoryManager* g_history_manager_instance = nullptr;
}  // namespace

HistoryManager::HistoryManager() {
  DCHECK(!g_history_manager_instance);
  g_history_manager_instance = this;
}

HistoryManager::~HistoryManager() {
  DCHECK_EQ(this, g_history_manager_instance);
  g_history_manager_instance = nullptr;
}

// static
HistoryManager* HistoryManager::GetInstance() {
  DCHECK(g_history_manager_instance);
  return g_history_manager_instance;
}

// static
bool HistoryManager::HasInstance() {
  return !!g_history_manager_instance;
}

void HistoryManager::AddObserver(HistoryManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void HistoryManager::RemoveObserver(HistoryManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

// static
HistoryItemList HistoryManager::Get(const HistoryFilterType filter_type,
                                    const HistorySortType sort_type,
                                    const base::Time from_time,
                                    const base::Time to_time) {
  HistoryItemList history_items =
      ClientStateManager::GetInstance()->GetHistory();

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

HistoryItemInfo HistoryManager::Add(
    const InlineContentAdInfo& ad,
    const ConfirmationType& confirmation_type) const {
  HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.description);
  NotifyHistoryDidChange();
  return history_item;
}

HistoryItemInfo HistoryManager::Add(
    const NewTabPageAdInfo& ad,
    const ConfirmationType& confirmation_type) const {
  HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.company_name, ad.alt);
  NotifyHistoryDidChange();
  return history_item;
}

HistoryItemInfo HistoryManager::Add(
    const NotificationAdInfo& ad,
    const ConfirmationType& confirmation_type) const {
  HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.body);
  NotifyHistoryDidChange();
  return history_item;
}

HistoryItemInfo HistoryManager::Add(
    const PromotedContentAdInfo& ad,
    const ConfirmationType& confirmation_type) const {
  HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.title, ad.description);
  NotifyHistoryDidChange();
  return history_item;
}

HistoryItemInfo HistoryManager::Add(
    const SearchResultAdInfo& ad,
    const ConfirmationType& confirmation_type) const {
  HistoryItemInfo history_item =
      AddHistory(ad, confirmation_type, ad.headline_text, ad.description);
  NotifyHistoryDidChange();
  return history_item;
}

AdContentLikeActionType HistoryManager::LikeAd(
    const AdContentInfo& ad_content) const {
  const AdContentLikeActionType action_type =
      ClientStateManager::GetInstance()->ToggleAdThumbUp(ad_content);
  if (action_type == AdContentLikeActionType::kThumbsUp) {
    NotifyDidLikeAd(ad_content);
  }

  return action_type;
}

AdContentLikeActionType HistoryManager::DislikeAd(
    const AdContentInfo& ad_content) const {
  const AdContentLikeActionType action_type =
      ClientStateManager::GetInstance()->ToggleAdThumbDown(ad_content);
  if (action_type == AdContentLikeActionType::kThumbsDown) {
    NotifyDidDislikeAd(ad_content);
  }

  return action_type;
}

CategoryContentOptActionType
HistoryManager::MarkToNoLongerReceiveAdsForCategory(
    const std::string& category,
    const CategoryContentOptActionType& action_type) const {
  const CategoryContentOptActionType toggled_action_type =
      ClientStateManager::GetInstance()->ToggleAdOptOut(category, action_type);
  if (toggled_action_type == CategoryContentOptActionType::kOptOut) {
    NotifyDidMarkToNoLongerReceiveAdsForCategory(category);
  }

  return toggled_action_type;
}

CategoryContentOptActionType HistoryManager::MarkToReceiveAdsForCategory(
    const std::string& category,
    const CategoryContentOptActionType& action_type) const {
  const CategoryContentOptActionType toggled_action_type =
      ClientStateManager::GetInstance()->ToggleAdOptIn(category, action_type);
  if (toggled_action_type == CategoryContentOptActionType::kOptIn) {
    NotifyDidMarkToReceiveAdsForCategory(category);
  }

  return toggled_action_type;
}

bool HistoryManager::ToggleMarkAdAsInappropriate(
    const AdContentInfo& ad_content) const {
  const bool is_marked =
      ClientStateManager::GetInstance()->ToggleFlaggedAd(ad_content);

  if (is_marked) {
    NotifyDidMarkAdAsInappropriate(ad_content);
  } else {
    NotifyDidMarkAdAsAppropriate(ad_content);
  }

  return is_marked;
}

bool HistoryManager::ToggleSavedAd(const AdContentInfo& ad_content) const {
  const bool is_saved =
      ClientStateManager::GetInstance()->ToggleSavedAd(ad_content);
  if (is_saved) {
    NotifyDidSaveAd(ad_content);
  } else {
    NotifyDidUnsaveAd(ad_content);
  }

  return is_saved;
}

///////////////////////////////////////////////////////////////////////////////

void HistoryManager::NotifyHistoryDidChange() const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnHistoryDidChange();
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

void HistoryManager::NotifyDidMarkToNoLongerReceiveAdsForCategory(
    const std::string& category) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkToNoLongerReceiveAdsForCategory(category);
  }
}

void HistoryManager::NotifyDidMarkToReceiveAdsForCategory(
    const std::string& category) const {
  for (HistoryManagerObserver& observer : observers_) {
    observer.OnDidMarkToReceiveAdsForCategory(category);
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

}  // namespace ads
