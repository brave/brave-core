/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
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
const char kNotificationCreativeInstanceIdKey[] = "uuid";
const char kNotificationCreativeSetIdKey[] = "creative_set_id";
const char kNotificationCategoryKey[] = "category";
const char kNotificationTitleKey[] = "advertiser";
const char kNotificationBodyKey[] = "text";
const char kNotificationTargetUrlKey[] = "url";

Notifications::Notifications(
    AdsImpl* ads,
    AdsClient* ads_client)
    : is_initialized_(false),
      ads_(ads),
      ads_client_(ads_client) {
  (void)ads_;
}

Notifications::~Notifications() = default;

void Notifications::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  LoadState();
}

bool Notifications::Get(
    const std::string& uuid,
    AdNotificationInfo* info) const {
  DCHECK(is_initialized_);

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&uuid](const auto& notification){return notification.uuid == uuid;});
  if (iter == notifications_.end()) {
    return false;
  }

  *info = *iter;

  return true;
}

void Notifications::PushBack(const AdNotificationInfo& info) {
  DCHECK(is_initialized_);

  notifications_.push_back(info);
  ads_client_->ShowNotification(std::make_unique<AdNotificationInfo>(info));

  SaveState();
}

void Notifications::PopFront(
    bool should_dismiss) {
  if (!notifications_.empty()) {
    if (should_dismiss) {
      ads_client_->CloseNotification(notifications_.front().uuid);
    }
    notifications_.pop_front();
    SaveState();
  }
}

bool Notifications::Remove(
    const std::string& uuid,
    bool should_dismiss) {
  DCHECK(is_initialized_);

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&uuid](const auto& notification){return notification.uuid == uuid;});
  if (iter == notifications_.end()) {
    return false;
  }

  if (should_dismiss) {
    ads_client_->CloseNotification(uuid);
  }
  notifications_.erase(iter);

  SaveState();

  return true;
}

void Notifications::RemoveAll(
    bool should_dismiss) {
  DCHECK(is_initialized_);

  if (should_dismiss) {
    for (const auto& notification : notifications_) {
      ads_client_->CloseNotification(notification.uuid);
    }
  }
  notifications_.clear();

  SaveState();
}

bool Notifications::Exists(
    const std::string& uuid) const {
  DCHECK(is_initialized_);

  auto iter = std::find_if(notifications_.begin(), notifications_.end(),
      [&uuid](const auto& notification){return notification.uuid == uuid;});
  if (iter == notifications_.end()) {
    return false;
  }

  return true;
}

uint64_t Notifications::Count() const {
  return notifications_.size();
}

///////////////////////////////////////////////////////////////////////////////

std::deque<AdNotificationInfo> Notifications::GetNotificationsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  std::deque<AdNotificationInfo> notifications;

  for (auto& item : *list) {
    base::DictionaryValue* dictionary;
    if (!item.GetAsDictionary(&dictionary)) {
      continue;
    }

    AdNotificationInfo notification_info;
    if (!GetNotificationFromDictionary(dictionary, &notification_info)) {
      continue;
    }

    notifications.push_back(notification_info);
  }

  return notifications;
}

bool Notifications::GetNotificationFromDictionary(
    base::DictionaryValue* dictionary,
    AdNotificationInfo* info) const {
  AdNotificationInfo notification_info;

  if (!GetIdFromDictionary(dictionary, &notification_info.uuid)) {
    return false;
  }

  if (!GetCreativeSetIdFromDictionary(dictionary,
      &notification_info.creative_set_id)) {
    return false;
  }

  if (!GetCategoryFromDictionary(dictionary, &notification_info.category)) {
    return false;
  }

  if (!GetTitleFromDictionary(dictionary, &notification_info.title)) {
    return false;
  }

  if (!GetBodyFromDictionary(dictionary, &notification_info.body)) {
    return false;
  }

  if (!GetTargetUrlFromDictionary(dictionary, &notification_info.target_url)) {
    return false;
  }

  if (!GetCreativeInstanceIdFromDictionary(dictionary,
      &notification_info.creative_instance_id)) {
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

bool Notifications::GetCreativeInstanceIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeInstanceIdKey,
      dictionary, value);
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

bool Notifications::GetTitleFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTitleKey, dictionary, value);
}

bool Notifications::GetBodyFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationBodyKey, dictionary, value);
}

bool Notifications::GetTargetUrlFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTargetUrlKey, dictionary, value);
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
        base::Value(notification.uuid));
    dictionary.SetKey(kNotificationCreativeInstanceIdKey,
        base::Value(notification.creative_instance_id));
    dictionary.SetKey(kNotificationCreativeSetIdKey,
        base::Value(notification.creative_set_id));
    dictionary.SetKey(kNotificationCategoryKey,
        base::Value(notification.category));
    dictionary.SetKey(kNotificationTitleKey,
        base::Value(notification.title));
    dictionary.SetKey(kNotificationBodyKey,
        base::Value(notification.body));
    dictionary.SetKey(kNotificationTargetUrlKey,
        base::Value(notification.target_url));

    list.GetList().push_back(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
