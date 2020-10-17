/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_
#define BAT_ADS_INTERNAL_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_

#include <deque>
#include <string>

#include "base/values.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads.h"

namespace ads {

class AdsImpl;

class AdNotifications {
 public:
  AdNotifications(
      AdsImpl* ads);

  ~AdNotifications();

  void Initialize(
      InitializeCallback callback);

  bool Get(
      const std::string& uuid,
      AdNotificationInfo* info) const;

  void PushBack(
      const AdNotificationInfo& info);
  void PopFront(
      const bool should_dismiss);

  bool Remove(
      const std::string& uuid,
      const bool should_dismiss);
  void RemoveAll(
      const bool should_dismiss);

  bool Exists(
      const std::string& uuid) const;

  uint64_t Count() const;

 private:
  bool is_initialized_;

  InitializeCallback callback_;

  std::deque<AdNotificationInfo> ad_notifications_;

  std::deque<AdNotificationInfo> GetNotificationsFromList(
      base::ListValue* list) const;

  bool GetNotificationFromDictionary(
      base::DictionaryValue* dictionary,
      AdNotificationInfo* info) const;

  bool GetUuidFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCreativeInstanceIdFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCreativeSetIdFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCampaignIdFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCategoryFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetTitleFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetBodyFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetTargetUrlFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetGeoTargetFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;

  bool GetStringFromDictionary(
    const std::string& key,
    base::DictionaryValue* dictionary,
    std::string* string) const;

  void Save();
  void OnSaved(
      const Result result);

  void Load();
  void OnLoaded(
      const Result result,
      const std::string& json);

  bool FromJson(
      const std::string& json);
  bool GetNotificationsFromDictionary(
      base::DictionaryValue* dictionary);

  std::string ToJson();
  base::Value GetAsList();

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_NOTIFICATIONS_AD_NOTIFICATIONS_H_
