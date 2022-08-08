/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_

#include <cstdint>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/values.h"
#include "bat/ads/ads_callback.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct NotificationAdInfo;

class NotificationAdManager final {
 public:
  NotificationAdManager();
  ~NotificationAdManager();
  NotificationAdManager(const NotificationAdManager&) = delete;
  NotificationAdManager& operator=(const NotificationAdManager&) = delete;

  static NotificationAdManager* GetInstance();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  absl::optional<NotificationAdInfo> MaybeGetForPlacementId(
      const std::string& placement_id) const;

  void PushBack(const NotificationAdInfo& ad);
  void PopFront(const bool should_dismiss);

  bool Remove(const std::string& placement_id);
  void RemoveAll();

  void CloseAndRemoveAll();

  bool Exists(const std::string& placement_id) const;

  uint64_t Count() const;

#if BUILDFLAG(IS_ANDROID)
  void RemoveAllAfterReboot();
  void RemoveAllAfterUpdate();
#endif

 private:
  base::circular_deque<NotificationAdInfo> GetNotificationsFromList(
      const base::Value::List& list) const;

  bool GetNotificationFromDictionary(const base::Value::Dict& dict,
                                     NotificationAdInfo* notification_ad) const;

  bool GetPlacementIdFromDictionary(const base::Value::Dict& dict,
                                    std::string* value) const;
  bool GetCreativeInstanceIdFromDictionary(const base::Value::Dict& dict,
                                           std::string* value) const;
  bool GetCreativeSetIdFromDictionary(const base::Value::Dict& dict,
                                      std::string* value) const;
  bool GetCampaignIdFromDictionary(const base::Value::Dict& dict,
                                   std::string* value) const;
  bool GetAdvertiserIdFromDictionary(const base::Value::Dict& dict,
                                     std::string* value) const;
  bool GetSegmentFromDictionary(const base::Value::Dict& dict,
                                std::string* value) const;
  bool GetTitleFromDictionary(const base::Value::Dict& dict,
                              std::string* value) const;
  bool GetBodyFromDictionary(const base::Value::Dict& dict,
                             std::string* value) const;
  bool GetTargetUrlFromDictionary(const base::Value::Dict& dict,
                                  std::string* value) const;
  bool GetGeoTargetFromDictionary(const base::Value::Dict& dict,
                                  std::string* value) const;

  bool GetStringFromDictionary(const std::string& key,
                               const base::Value::Dict& dict,
                               std::string* string) const;

  void Save();
  void OnSaved(const bool success);

  void Load();
  void OnLoaded(const bool success, const std::string& json);

  bool FromJson(const std::string& json);
  bool GetNotificationsFromDictionary(const base::Value::Dict& dict);

  std::string ToJson();
  base::Value::List GetAsList();

  bool is_initialized_ = false;

  InitializeCallback callback_;

  base::circular_deque<NotificationAdInfo> notification_ads_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_MANAGER_H_
