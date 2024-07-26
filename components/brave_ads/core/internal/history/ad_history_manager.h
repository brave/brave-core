/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_

#include <optional>

#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_history_filter_types.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_sort_types.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct NotificationAdInfo;
struct PromotedContentAdInfo;
struct SearchResultAdInfo;

class AdHistoryManager final {
 public:
  AdHistoryManager();

  AdHistoryManager(const AdHistoryManager&) = delete;
  AdHistoryManager& operator=(const AdHistoryManager&) = delete;

  AdHistoryManager(AdHistoryManager&&) noexcept = delete;
  AdHistoryManager& operator=(AdHistoryManager&&) noexcept = delete;

  ~AdHistoryManager();

  static AdHistoryManager& GetInstance();

  void AddObserver(AdHistoryManagerObserver* observer);
  void RemoveObserver(AdHistoryManagerObserver* observer);

  static const AdHistoryList& Get();
  static AdHistoryList Get(AdHistoryFilterType filter_type,
                           AdHistorySortType sort_type,
                           base::Time from_time,
                           base::Time to_time);

  std::optional<AdHistoryItemInfo> Add(
      const InlineContentAdInfo& ad,
      ConfirmationType confirmation_type) const;
  std::optional<AdHistoryItemInfo> Add(
      const NewTabPageAdInfo& ad,
      ConfirmationType confirmation_type) const;
  std::optional<AdHistoryItemInfo> Add(
      const NotificationAdInfo& ad,
      ConfirmationType confirmation_type) const;
  std::optional<AdHistoryItemInfo> Add(
      const PromotedContentAdInfo& ad,
      ConfirmationType confirmation_type) const;
  std::optional<AdHistoryItemInfo> Add(
      const SearchResultAdInfo& ad,
      ConfirmationType confirmation_type) const;

  void LikeAd(const AdHistoryItemInfo& ad_history_item,
              ToggleUserReactionCallback callback) const;
  void DislikeAd(const AdHistoryItemInfo& ad_history_item,
                 ToggleUserReactionCallback callback) const;

  void LikeCategory(const AdHistoryItemInfo& ad_history_item,
                    ToggleUserReactionCallback callback) const;
  void DislikeCategory(const AdHistoryItemInfo& ad_history_item,
                       ToggleUserReactionCallback callback) const;

  void ToggleSaveAd(const AdHistoryItemInfo& ad_history_item,
                    ToggleUserReactionCallback callback) const;

  void ToggleMarkAdAsInappropriate(const AdHistoryItemInfo& ad_history_item,
                                   ToggleUserReactionCallback callback) const;

 private:
  void NotifyDidAppendAdHistoryItem(
      const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidLikeAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidDislikeAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidLikeCategory(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidDislikeCategory(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidSaveAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidUnsaveAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidMarkAdAsInappropriate(
      const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidMarkAdAsAppropriate(
      const AdHistoryItemInfo& ad_history_item) const;

  base::ObserverList<AdHistoryManagerObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
