/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_

#include <cstdint>
#include <deque>
#include <string>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_aliases.h"
#include "build/build_config.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}  // namespace base

namespace ads {

class AdNotifications final {
 public:
  AdNotifications();
  ~AdNotifications();

  static AdNotifications* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  bool Get(const std::string& placement_id,
           AdNotificationInfo* ad_notification) const;

  void PushBack(const AdNotificationInfo& info);
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
  std::deque<AdNotificationInfo> GetNotificationsFromList(
      base::Value* list) const;

  bool GetNotificationFromDictionary(base::DictionaryValue* dictionary,
                                     AdNotificationInfo* ad_notification) const;

  bool GetPlacementIdFromDictionary(base::DictionaryValue* dictionary,
                                    std::string* value) const;
  bool GetCreativeInstanceIdFromDictionary(base::DictionaryValue* dictionary,
                                           std::string* value) const;
  bool GetCreativeSetIdFromDictionary(base::DictionaryValue* dictionary,
                                      std::string* value) const;
  bool GetCampaignIdFromDictionary(base::DictionaryValue* dictionary,
                                   std::string* value) const;
  bool GetAdvertiserIdFromDictionary(base::DictionaryValue* dictionary,
                                     std::string* value) const;
  bool GetSegmentFromDictionary(base::DictionaryValue* dictionary,
                                std::string* value) const;
  bool GetTitleFromDictionary(base::DictionaryValue* dictionary,
                              std::string* value) const;
  bool GetBodyFromDictionary(base::DictionaryValue* dictionary,
                             std::string* value) const;
  bool GetTargetUrlFromDictionary(base::DictionaryValue* dictionary,
                                  std::string* value) const;
  bool GetGeoTargetFromDictionary(base::DictionaryValue* dictionary,
                                  std::string* value) const;

  bool GetStringFromDictionary(const std::string& key,
                               base::DictionaryValue* dictionary,
                               std::string* string) const;

  void Save();
  void OnSaved(const bool success);

  void Load();
  void OnLoaded(const bool success, const std::string& json);

  bool FromJson(const std::string& json);
  bool GetNotificationsFromDictionary(base::DictionaryValue* dictionary);

  std::string ToJson();
  base::Value GetAsList();

  bool is_initialized_ = false;

  InitializeCallback callback_;

  std::deque<AdNotificationInfo> ad_notifications_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_
