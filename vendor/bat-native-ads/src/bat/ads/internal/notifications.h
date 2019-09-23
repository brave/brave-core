/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_NOTIFICATIONS_H_
#define BAT_ADS_INTERNAL_NOTIFICATIONS_H_

#include <string>
#include <deque>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/notification_info.h"

#include "base/values.h"

namespace ads {

class AdsImpl;

class Notifications {
 public:
  Notifications(AdsImpl* ads, AdsClient* ads_client);
  ~Notifications();

  void Initialize(InitializeCallback callback);

  bool Get(const std::string& id, NotificationInfo* info) const;

  void PushBack(const NotificationInfo& info);
  void PopFront(bool should_dismiss);

  bool Remove(const std::string& id, bool should_dismiss);
  void RemoveAll(bool should_dismiss);

  bool Exists(const std::string& id) const;

  uint64_t Count() const;

 private:
  bool is_initialized_;

  InitializeCallback callback_;

  std::deque<NotificationInfo> notifications_;

  std::deque<NotificationInfo> GetNotificationsFromList(
      base::ListValue* list) const;

  bool GetNotificationFromDictionary(
      base::DictionaryValue* dictionary,
      NotificationInfo* info) const;

  bool GetIdFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCreativeSetIdFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetCategoryFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetAdvertiserFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetTextFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetUrlFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;
  bool GetUuidFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* value) const;

  bool GetStringFromDictionary(
    const std::string& key,
    base::DictionaryValue* dictionary,
    std::string* string) const;

  void SaveState();
  void OnStateSaved(const Result result);

  void LoadState();
  void OnStateLoaded(const Result result, const std::string& json);

  bool FromJson(const std::string& json);
  bool GetNotificationsFromJson(base::DictionaryValue* dictionary);

  std::string ToJson();
  base::Value GetAsList();

  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_NOTIFICATIONS_H_
