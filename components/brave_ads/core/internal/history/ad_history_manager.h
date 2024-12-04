/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"

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
           mojom::ConfirmationType mojom_confirmation_type) const;
  void Add(const NewTabPageAdInfo& ad,
           mojom::ConfirmationType mojom_confirmation_type) const;
  void Add(const NotificationAdInfo& ad,
           mojom::ConfirmationType mojom_confirmation_type) const;
  void Add(const PromotedContentAdInfo& ad,
           mojom::ConfirmationType mojom_confirmation_type) const;
  void Add(const SearchResultAdInfo& ad,
           mojom::ConfirmationType mojom_confirmation_type) const;

 private:
  void MaybeAdd(const AdInfo& ad,
                mojom::ConfirmationType mojom_confirmation_type,
                const std::string& title,
                const std::string& description) const;

  static void GetForUICallback(GetAdHistoryForUICallback callback,
                               std::optional<AdHistoryList> ad_history);

  void NotifyDidAddAdHistoryItem(
      const AdHistoryItemInfo& ad_history_item) const;

  base::ObserverList<AdHistoryManagerObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_H_
