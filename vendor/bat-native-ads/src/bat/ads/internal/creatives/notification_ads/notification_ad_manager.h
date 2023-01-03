/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_

#include <string>

#include "absl/types/optional.h"
#include "base/containers/circular_deque.h"
#include "bat/ads/notification_ad_info.h"

namespace ads {

class NotificationAdManager final {
 public:
  NotificationAdManager();

  NotificationAdManager(const NotificationAdManager& other) = delete;
  NotificationAdManager& operator=(const NotificationAdManager& other) = delete;

  NotificationAdManager(NotificationAdManager&& other) noexcept = delete;
  NotificationAdManager& operator=(NotificationAdManager&& other) noexcept =
      delete;

  ~NotificationAdManager();

  static NotificationAdManager* GetInstance();

  static bool HasInstance();

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

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
