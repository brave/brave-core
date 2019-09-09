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

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&id](const auto& notification){return notification.id == id;});
  if (iter == notifications_.end()) {
    return false;
  }

  *info = *iter;

  return true;
}

void Notifications::PushBack(const NotificationInfo& info) {
  DCHECK(is_initialized_);

  notifications_.push_back(info);
  ads_client_->ShowNotification(std::make_unique<NotificationInfo>(info));

  SaveState();
}

void Notifications::PopFront(bool should_dismiss) {
  if (!notifications_.empty()) {
    if (should_dismiss){
      ads_client_->CloseNotification(notifications_.front().id);
    }
    notifications_.pop_front();
    SaveState();
  }
}

bool Notifications::Remove(const std::string& id, bool should_dismiss) {
  DCHECK(is_initialized_);

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&id](const auto& notification){return notification.id == id;});
  if (iter == notifications_.end()) {
    return false;
  }

  if (should_dismiss){
    ads_client_->CloseNotification(id);
  }
  notifications_.erase(iter);

  SaveState();

  return true;
}

void Notifications::RemoveAll(bool should_dismiss) {
  DCHECK(is_initialized_);

  if (should_dismiss){
    for (const auto& notification : notifications_) {
      ads_client_->CloseNotification(notification.id);
    }
  }
  notifications_.clear();

  SaveState();
}

bool Notifications::Exists(const std::string& id) const {
  DCHECK(is_initialized_);

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&id](const auto& notification){return notification.id == id;});
  if (iter == notifications_.end()) {
    return false;
  }

  return true;
}

uint64_t Notifications::Count() const {
  return notifications_.size();
}

///////////////////////////////////////////////////////////////////////////////

std::deque<NotificationInfo> Notifications::GetNotificationsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  std::deque<NotificationInfo> notifications;

  for (auto& item : *list) {
    base::DictionaryValue* dictionary;
    if (!item.GetAsDictionary(&dictionary)) {
      continue;
    }

    NotificationInfo notification_info;
    if (!GetNotificationFromDictionary(dictionary, &notification_info)) {
      continue;
    }

    notifications.push_back(notification_info);
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

  *info = notification_info;

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

    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(kNotificationIdKey,
        base::Value(notification.id));
    dictionary.SetKey(kNotificationCreativeSetIdKey,
        base::Value(notification.creative_set_id));
    dictionary.SetKey(kNotificationCategoryKey,
        base::Value(notification.category));
    dictionary.SetKey(kNotificationAdvertiserKey,
        base::Value(notification.advertiser));
    dictionary.SetKey(kNotificationTextKey,
        base::Value(notification.text));
    dictionary.SetKey(kNotificationUrlKey,
        base::Value(notification.url));
    dictionary.SetKey(kNotificationUuidKey,
        base::Value(notification.uuid));

    list.GetList().push_back(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
