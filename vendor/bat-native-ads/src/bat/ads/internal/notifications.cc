/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ads/internal/notifications.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/ads_impl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ads {

const char kNotificationsStateName[] = "notifications.json";

const char kNotificationsListKey[] = "notifications";

const char kNotificationIdKey[] = "id";
const char kNotificationCreativeSetIdKey[] = "creative_set_id";
const char kNotificationCategoryKey[] = "category";
const char kNotificationAdvertiserKey[] = "advertiser";
const char kNotificationTextKey[] = "text";
const char kNotificationUrlKey[] = "url";
const char kNotificationUuidKey[] = "uuid";

Notifications::Notifications(AdsImpl* ads, AdsClient* ads_client) :
    is_initialized_(false),
    notifications_({}),
    ads_(ads),
    ads_client_(ads_client) {
  (void)ads_;
}

Notifications::~Notifications() = default;

void Notifications::Initialize(InitializeCallback callback) {
  callback_ = callback;

  LoadState();
}

bool Notifications::Get(const std::string& id, NotificationInfo* info) const {
  DCHECK(is_initialized_);

  auto notification = notifications_.find(id);
  if (notification == notifications_.end()) {
    return false;
  }

  *info = notification->second;

  return true;
}

void Notifications::Add(const NotificationInfo& info) {
  DCHECK(is_initialized_);

  notifications_.insert({info.id, info});

  SaveState();
}

bool Notifications::Remove(const std::string& id) {
  DCHECK(is_initialized_);

  if (!Exists(id)) {
    return false;
  }

  ads_client_->CloseNotification(id);
  notifications_.erase(id);

  SaveState();

  return true;
}

void Notifications::RemoveAll() {
  DCHECK(is_initialized_);

  for (const auto& notification : notifications_) {
    auto id = notification.first;
    Remove(id);
  }

  SaveState();
}

void Notifications::CloseAll() const {
  DCHECK(is_initialized_);

  for (const auto& notification : notifications_) {
    auto id = notification.first;
    ads_client_->CloseNotification(id);
  }
}

bool Notifications::Exists(const std::string& id) const {
  DCHECK(is_initialized_);

  if (notifications_.find(id) == notifications_.end()) {
    return false;
  }

  return true;
}

uint64_t Notifications::Count() const {
  return notifications_.size();
}

///////////////////////////////////////////////////////////////////////////////

std::map<std::string, NotificationInfo> Notifications::GetNotificationsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  std::map<std::string, NotificationInfo> notifications;

  for (auto& item : *list) {
    base::DictionaryValue* dictionary;
    if (!item.GetAsDictionary(&dictionary)) {
      continue;
    }

    NotificationInfo notification_info;
    if (!GetNotificationFromDictionary(dictionary, &notification_info)) {
      continue;
    }

    notifications.insert({notification_info.id, notification_info});
  }

  return notifications;
}

bool Notifications::GetNotificationFromDictionary(
    base::DictionaryValue* dictionary,
    NotificationInfo* info) const {
  NotificationInfo notification_info;

  if (!GetIdFromDictionary(dictionary, &notification_info.id)) {
    return false;
  }

  if (!GetCreativeSetIdFromDictionary(dictionary,
      &notification_info.creative_set_id)) {
    return false;
  }

  if (!GetCategoryFromDictionary(dictionary, &notification_info.category)) {
    return false;
  }

  if (!GetAdvertiserFromDictionary(dictionary, &notification_info.advertiser)) {
    return false;
  }

  if (!GetTextFromDictionary(dictionary, &notification_info.text)) {
    return false;
  }

  if (!GetUrlFromDictionary(dictionary, &notification_info.url)) {
    return false;
  }

  if (!GetUuidFromDictionary(dictionary, &notification_info.uuid)) {
    return false;
  }

  *info = NotificationInfo(notification_info);

  return true;
}

bool Notifications::GetIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationIdKey, dictionary, value);
}

bool Notifications::GetCreativeSetIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeSetIdKey,
      dictionary, value);
}

bool Notifications::GetCategoryFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCategoryKey, dictionary, value);
}

bool Notifications::GetAdvertiserFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationAdvertiserKey, dictionary, value);
}

bool Notifications::GetTextFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTextKey, dictionary, value);
}

bool Notifications::GetUrlFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationUrlKey, dictionary, value);
}

bool Notifications::GetUuidFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationUuidKey, dictionary, value);
}

bool Notifications::GetStringFromDictionary(
    const std::string& key,
    base::DictionaryValue* dictionary,
    std::string* string) const {
  DCHECK(dictionary);
  DCHECK(string);

  auto* value = dictionary->FindKey(key);
  if (!value || !value->is_string()) {
    return false;
  }

  auto string_value = value->GetString();

  *string = string_value;

  return true;
}

void Notifications::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(INFO) << "Saving notifications state";

  std::string json = ToJson();
  auto callback = std::bind(&Notifications::OnStateSaved, this, _1);
  ads_client_->Save(kNotificationsStateName, json, callback);
}

void Notifications::OnStateSaved(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save notifications state";
    return;
  }

  BLOG(INFO) << "Successfully saved notifications state";
}

void Notifications::LoadState() {
  auto callback = std::bind(&Notifications::OnStateLoaded, this, _1, _2);
  ads_client_->Load(kNotificationsStateName, callback);
}

void Notifications::OnStateLoaded(
    const Result result,
    const std::string& json) {
  is_initialized_ = true;

  if (result != SUCCESS) {
    BLOG(ERROR)
        << "Failed to load notifications state, resetting to default values";

    notifications_.clear();
  } else {
    if (!FromJson(json)) {
      BLOG(ERROR) << "Failed to parse notifications state: " << json;

      callback_(FAILED);
      return;
    }

    BLOG(INFO) << "Successfully loaded notifications state";
  }

  callback_(SUCCESS);
}

bool Notifications::FromJson(const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  if (!GetNotificationsFromJson(dictionary)) {
    BLOG(WARNING) << "Failed to get notifications from JSON: " << json;
    return false;
  }

  SaveState();

  return true;
}

bool Notifications::GetNotificationsFromJson(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  auto* value = dictionary->FindKey(kNotificationsListKey);
  if (!value || !value->is_list()) {
    return false;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    return false;
  }

  notifications_ = GetNotificationsFromList(list);

  return true;
}

std::string Notifications::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto notifications = GetAsList();
  dictionary.SetKey(kNotificationsListKey,
      base::Value(std::move(notifications)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value Notifications::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& notification : notifications_) {
    NotificationInfo notification_info = NotificationInfo(notification.second);

    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(kNotificationIdKey,
        base::Value(notification_info.id));
    dictionary.SetKey(kNotificationCreativeSetIdKey,
        base::Value(notification_info.creative_set_id));
    dictionary.SetKey(kNotificationCategoryKey,
        base::Value(notification_info.category));
    dictionary.SetKey(kNotificationAdvertiserKey,
        base::Value(notification_info.advertiser));
    dictionary.SetKey(kNotificationTextKey,
        base::Value(notification_info.text));
    dictionary.SetKey(kNotificationUrlKey,
        base::Value(notification_info.url));
    dictionary.SetKey(kNotificationUuidKey,
        base::Value(notification_info.uuid));

    list.GetList().push_back(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
