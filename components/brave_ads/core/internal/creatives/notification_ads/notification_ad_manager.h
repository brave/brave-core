/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "brave/components/brave_ads/core/public/ads/notification_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class NotificationAdManager final {
 public:
  NotificationAdManager();

  NotificationAdManager(const NotificationAdManager&) = delete;
  NotificationAdManager& operator=(const NotificationAdManager&) = delete;

  NotificationAdManager(NotificationAdManager&&) noexcept = delete;
  NotificationAdManager& operator=(NotificationAdManager&&) noexcept = delete;

  ~NotificationAdManager();

  static NotificationAdManager& GetInstance();

  absl::optional<NotificationAdInfo> MaybeGetForPlacementId(
      const std::string& placement_id) const;

  void Add(const NotificationAdInfo& ad);

  bool Remove(const std::string& placement_id);
  void RemoveAll();

  void CloseAll();

  bool Exists(const std::string& placement_id) const;

 private:
  void Initialize();

  void MaybeRemoveAll();

  base::circular_deque<NotificationAdInfo> ads_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
