/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/internal/history/history_manager_observer.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

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

  HistoryManager(const HistoryManager& other) = delete;
  HistoryManager& operator=(const HistoryManager& other) = delete;

  HistoryManager(HistoryManager&& other) noexcept = delete;
  HistoryManager& operator=(HistoryManager&& other) noexcept = delete;

  ~HistoryManager();

  static HistoryManager* GetInstance();

  static bool HasInstance();

  void AddObserver(HistoryManagerObserver* observer);
  void RemoveObserver(HistoryManagerObserver* observer);

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

  CategoryContentOptActionType MarkToNoLongerReceiveAdsForCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) const;
  CategoryContentOptActionType MarkToReceiveAdsForCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) const;

  bool ToggleMarkAdAsInappropriate(const AdContentInfo& ad_content) const;

  bool ToggleSavedAd(const AdContentInfo& ad_content) const;

 private:
  void NotifyHistoryDidChange() const;
  void NotifyDidLikeAd(const AdContentInfo& ad_content) const;
  void NotifyDidDislikeAd(const AdContentInfo& ad_content) const;
  void NotifyDidMarkToNoLongerReceiveAdsForCategory(
      const std::string& category) const;
  void NotifyDidMarkToReceiveAdsForCategory(const std::string& category) const;
  void NotifyDidMarkAdAsInappropriate(const AdContentInfo& ad_content) const;
  void NotifyDidMarkAdAsAppropriate(const AdContentInfo& ad_content) const;
  void NotifyDidSaveAd(const AdContentInfo& ad_content) const;
  void NotifyDidUnsaveAd(const AdContentInfo& ad_content) const;

  base::ObserverList<HistoryManagerObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_H_
