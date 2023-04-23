/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/ad_content_action_types.h"
#include "brave/components/brave_ads/core/category_content_action_types.h"
#include "brave/components/brave_ads/core/history_filter_types.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/history_sort_types.h"
#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class ConfirmationType;
struct AdContentInfo;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct NotificationAdInfo;
struct PromotedContentAdInfo;
struct SearchResultAdInfo;

class HistoryManager final {
 public:
  HistoryManager();

  HistoryManager(const HistoryManager&) = delete;
  HistoryManager& operator=(const HistoryManager&) = delete;

  HistoryManager(HistoryManager&&) noexcept = delete;
  HistoryManager& operator=(HistoryManager&&) noexcept = delete;

  ~HistoryManager();

  static HistoryManager& GetInstance();

  void AddObserver(HistoryManagerObserver* observer);
  void RemoveObserver(HistoryManagerObserver* observer);

  static const HistoryItemList& Get();
  static HistoryItemList Get(HistoryFilterType filter_type,
                             HistorySortType sort_type,
                             base::Time from_time,
                             base::Time to_time);

  HistoryItemInfo Add(const InlineContentAdInfo& ad,
                      const ConfirmationType& confirmation_type) const;
  HistoryItemInfo Add(const NewTabPageAdInfo& ad,
                      const ConfirmationType& confirmation_type) const;
  HistoryItemInfo Add(const NotificationAdInfo& ad,
                      const ConfirmationType& confirmation_type) const;
  HistoryItemInfo Add(const PromotedContentAdInfo& ad,
                      const ConfirmationType& confirmation_type) const;
  HistoryItemInfo Add(const SearchResultAdInfo& ad,
                      const ConfirmationType& confirmation_type) const;

  AdContentLikeActionType LikeAd(const AdContentInfo& ad_content) const;
  AdContentLikeActionType DislikeAd(const AdContentInfo& ad_content) const;

  CategoryContentOptActionType LikeCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) const;
  CategoryContentOptActionType DislikeCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) const;

  bool ToggleSaveAd(const AdContentInfo& ad_content) const;

  bool ToggleMarkAdAsInappropriate(const AdContentInfo& ad_content) const;

 private:
  void NotifyDidAddHistory(const HistoryItemInfo& history_item) const;
  void NotifyDidLikeAd(const AdContentInfo& ad_content) const;
  void NotifyDidDislikeAd(const AdContentInfo& ad_content) const;
  void NotifyDidLikeCategory(const std::string& category) const;
  void NotifyDidDislikeCategory(const std::string& category) const;
  void NotifyDidSaveAd(const AdContentInfo& ad_content) const;
  void NotifyDidUnsaveAd(const AdContentInfo& ad_content) const;
  void NotifyDidMarkAdAsInappropriate(const AdContentInfo& ad_content) const;
  void NotifyDidMarkAdAsAppropriate(const AdContentInfo& ad_content) const;

  base::ObserverList<HistoryManagerObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_H_
