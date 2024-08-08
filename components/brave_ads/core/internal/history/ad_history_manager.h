/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

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

  static void Get(base::Time from_time,
                  base::Time to_time,
                  GetAdHistoryCallback callback);
  static void GetForUI(base::Time from_time,
                       base::Time to_time,
                       GetAdHistoryForUICallback callback);

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

  // TODO(https://github.com/brave/brave-browser/issues/40241): Decouple
  // reactions from ads history.
  void LikeAd(const AdHistoryItemInfo& ad_history_item,
              ToggleReactionCallback callback) const;
  void DislikeAd(const AdHistoryItemInfo& ad_history_item,
                 ToggleReactionCallback callback) const;
  void LikeSegment(const AdHistoryItemInfo& ad_history_item,
                   ToggleReactionCallback callback) const;
  void DislikeSegment(const AdHistoryItemInfo& ad_history_item,
                      ToggleReactionCallback callback) const;
  void ToggleSaveAd(const AdHistoryItemInfo& ad_history_item,
                    ToggleReactionCallback callback) const;
  void ToggleMarkAdAsInappropriate(const AdHistoryItemInfo& ad_history_item,
                                   ToggleReactionCallback callback) const;

 private:
  void MaybeAdd(const AdInfo& ad,
                ConfirmationType confirmation_type,
                const std::string& title,
                const std::string& description) const;

  static void GetForUICallback(GetAdHistoryForUICallback callback,
                               const std::optional<AdHistoryList>& ad_history);

  void NotifyDidAddAdHistoryItem(
      const AdHistoryItemInfo& ad_history_item) const;

  // TODO(https://github.com/brave/brave-browser/issues/40241): Decouple
  // reactions from ads history.
  void NotifyDidLikeAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidDislikeAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidLikeSegment(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidDislikeSegment(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidToggleSaveAd(const AdHistoryItemInfo& ad_history_item) const;
  void NotifyDidToggleMarkAdAsInappropriate(
      const AdHistoryItemInfo& ad_history_item) const;

  base::ObserverList<AdHistoryManagerObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
