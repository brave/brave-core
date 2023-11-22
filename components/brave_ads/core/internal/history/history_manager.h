/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/history_filter_types.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

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

  void Add(const InlineContentAdInfo& ad,
           ConfirmationType confirmation_type) const;
  void Add(const NewTabPageAdInfo& ad,
           ConfirmationType confirmation_type) const;
  void Add(const NotificationAdInfo& ad,
           ConfirmationType confirmation_type) const;
  void Add(const PromotedContentAdInfo& ad,
           ConfirmationType confirmation_type) const;
  void Add(const SearchResultAdInfo& ad,
           ConfirmationType confirmation_type) const;

  mojom::UserReactionType LikeAd(const AdContentInfo& ad_content) const;
  mojom::UserReactionType DislikeAd(const AdContentInfo& ad_content) const;

  mojom::UserReactionType LikeCategory(
      const CategoryContentInfo& category_content) const;
  mojom::UserReactionType DislikeCategory(
      const CategoryContentInfo& category_content) const;

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
