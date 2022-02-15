/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"

#include <functional>
#include <utility>

#include "base/check_op.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/logging.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#include "base/system/sys_info.h"
#endif

namespace ads {

namespace {

AdNotifications* g_ad_notifications = nullptr;

#if defined(OS_ANDROID)
const int kMaximumAdNotifications = 3;
#else
const int kMaximumAdNotifications = 0;  // No limit
#endif

const char kNotificationsFilename[] = "notifications.json";

const char kNotificationsListKey[] = "notifications";

const char kNotificationUuidKey[] = "id";
const char kNotificationCreativeInstanceIdKey[] = "uuid";
const char kNotificationCreativeSetIdKey[] = "creative_set_id";
const char kNotificationCampaignIdKey[] = "campaign_id";
const char kNotificationAdvertiserIdKey[] = "advertiser_id";
const char kNotificationSegmentKey[] = "segment";
const char kNotificationTitleKey[] = "advertiser";
const char kNotificationBodyKey[] = "text";
const char kNotificationTargetUrlKey[] = "url";

}  // namespace

AdNotifications::AdNotifications() {
  DCHECK_EQ(g_ad_notifications, nullptr);
  g_ad_notifications = this;
}

AdNotifications::~AdNotifications() {
  DCHECK(g_ad_notifications);
  g_ad_notifications = nullptr;
}

// static
AdNotifications* AdNotifications::Get() {
  DCHECK(g_ad_notifications);
  return g_ad_notifications;
}

// static
bool AdNotifications::HasInstance() {
  return g_ad_notifications;
}

void AdNotifications::Initialize(InitializeCallback callback) {
  callback_ = callback;

  Load();
}

bool AdNotifications::Get(const std::string& uuid,
                          AdNotificationInfo* ad_notification) const {
  DCHECK(is_initialized_);
  DCHECK(ad_notification);

  auto iter = std::find_if(ad_notifications_.cbegin(), ad_notifications_.cend(),
                           [&uuid](const AdNotificationInfo& notification) {
                             return notification.uuid == uuid;
                           });

  if (iter == ad_notifications_.end()) {
    return false;
  }

  *ad_notification = *iter;

  ad_notification->type = AdType::kAdNotification;

  return true;
}

void AdNotifications::PushBack(const AdNotificationInfo& info) {
  DCHECK(is_initialized_);

  ad_notifications_.push_back(info);

  if (kMaximumAdNotifications > 0 && Count() > kMaximumAdNotifications) {
    PopFront(true);
  }

  Save();
}

void AdNotifications::PopFront(const bool should_dismiss) {
  if (!ad_notifications_.empty()) {
    if (should_dismiss) {
      AdsClientHelper::Get()->CloseNotification(ad_notifications_.front().uuid);
    }
    ad_notifications_.pop_front();
    Save();
  }
}

bool AdNotifications::Remove(const std::string& uuid) {
  DCHECK(is_initialized_);

  auto iter = std::find_if(ad_notifications_.cbegin(), ad_notifications_.cend(),
                           [&uuid](const AdNotificationInfo& notification) {
                             return notification.uuid == uuid;
                           });

  if (iter == ad_notifications_.end()) {
    return false;
  }

  ad_notifications_.erase(iter);
  Save();

  return true;
}

void AdNotifications::RemoveAll() {
  DCHECK(is_initialized_);

  ad_notifications_.clear();
  Save();
}

void AdNotifications::CloseAndRemoveAll() {
  DCHECK(is_initialized_);

  for (const auto& ad_notification : ad_notifications_) {
    AdsClientHelper::Get()->CloseNotification(ad_notification.uuid);
  }

  RemoveAll();
}

bool AdNotifications::Exists(const std::string& uuid) const {
  DCHECK(is_initialized_);

  auto iter = std::find_if(ad_notifications_.cbegin(), ad_notifications_.cend(),
                           [&uuid](const AdNotificationInfo& notification) {
                             return notification.uuid == uuid;
                           });

  if (iter == ad_notifications_.end()) {
    return false;
  }

  return true;
}

uint64_t AdNotifications::Count() const {
  return ad_notifications_.size();
}

#if defined(OS_ANDROID)
void AdNotifications::RemoveAllAfterReboot() {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "New tab page ad: Failed to get ad events");
      return;
    }

    if (ad_events.empty()) {
      return;
    }

    const AdEventInfo ad_event = ad_events.front();

    const base::Time system_uptime =
        base::Time::Now() - base::SysInfo::Uptime();

    if (ad_event.created_at <= system_uptime) {
      RemoveAll();
    }
  });
}

void AdNotifications::RemoveAllAfterUpdate() {
  const std::string current_version_code =
      base::android::BuildInfo::GetInstance()->package_version_code();

  const std::string last_version_code = Client::Get()->GetVersionCode();

  if (last_version_code == current_version_code) {
    return;
  }

  Client::Get()->SetVersionCode(current_version_code);

  RemoveAll();
}
#endif

///////////////////////////////////////////////////////////////////////////////

std::deque<AdNotificationInfo> AdNotifications::GetNotificationsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  std::deque<AdNotificationInfo> notifications;

  for (auto& item : list->GetListDeprecated()) {
    base::DictionaryValue* dictionary = nullptr;
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

bool AdNotifications::GetNotificationFromDictionary(
    base::DictionaryValue* dictionary,
    AdNotificationInfo* ad_notification) const {
  AdNotificationInfo new_ad_notification;

  if (!GetUuidFromDictionary(dictionary, &new_ad_notification.uuid)) {
    return false;
  }

  if (!GetCreativeInstanceIdFromDictionary(
          dictionary, &new_ad_notification.creative_instance_id)) {
    return false;
  }

  if (!GetCreativeSetIdFromDictionary(dictionary,
                                      &new_ad_notification.creative_set_id)) {
    return false;
  }

  if (!GetCampaignIdFromDictionary(dictionary,
                                   &new_ad_notification.campaign_id)) {
    // Migrate for legacy notifications
    new_ad_notification.campaign_id = "";
  }

  if (!GetAdvertiserIdFromDictionary(dictionary,
                                     &new_ad_notification.advertiser_id)) {
    // Migrate for legacy notifications
    new_ad_notification.advertiser_id = "";
  }

  if (!GetSegmentFromDictionary(dictionary, &new_ad_notification.segment)) {
    // Migrate for legacy notifications
    if (!GetStringFromDictionary("category", dictionary,
                                 &new_ad_notification.segment)) {
      return false;
    }
  }

  if (!GetTitleFromDictionary(dictionary, &new_ad_notification.title)) {
    return false;
  }

  if (!GetBodyFromDictionary(dictionary, &new_ad_notification.body)) {
    return false;
  }

  if (!GetTargetUrlFromDictionary(dictionary,
                                  &new_ad_notification.target_url)) {
    return false;
  }

  *ad_notification = new_ad_notification;

  return true;
}

bool AdNotifications::GetUuidFromDictionary(base::DictionaryValue* dictionary,
                                            std::string* value) const {
  return GetStringFromDictionary(kNotificationUuidKey, dictionary, value);
}

bool AdNotifications::GetCreativeInstanceIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeInstanceIdKey, dictionary,
                                 value);
}

bool AdNotifications::GetCreativeSetIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeSetIdKey, dictionary,
                                 value);
}

bool AdNotifications::GetCampaignIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCampaignIdKey, dictionary, value);
}

bool AdNotifications::GetAdvertiserIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationAdvertiserIdKey, dictionary,
                                 value);
}

bool AdNotifications::GetSegmentFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationSegmentKey, dictionary, value);
}

bool AdNotifications::GetTitleFromDictionary(base::DictionaryValue* dictionary,
                                             std::string* value) const {
  return GetStringFromDictionary(kNotificationTitleKey, dictionary, value);
}

bool AdNotifications::GetBodyFromDictionary(base::DictionaryValue* dictionary,
                                            std::string* value) const {
  return GetStringFromDictionary(kNotificationBodyKey, dictionary, value);
}

bool AdNotifications::GetTargetUrlFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTargetUrlKey, dictionary, value);
}

bool AdNotifications::GetStringFromDictionary(const std::string& key,
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

void AdNotifications::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving ad notifications state");

  std::string json = ToJson();
  auto callback =
      std::bind(&AdNotifications::OnSaved, this, std::placeholders::_1);
  AdsClientHelper::Get()->Save(kNotificationsFilename, json, callback);
}

void AdNotifications::OnSaved(const bool success) {
  if (!success) {
    BLOG(0, "Failed to save ad notifications state");
    return;
  }

  BLOG(9, "Successfully saved ad notifications state");
}

void AdNotifications::Load() {
  BLOG(3, "Loading ad notifications state");

  auto callback = std::bind(&AdNotifications::OnLoaded, this,
                            std::placeholders::_1, std::placeholders::_2);
  AdsClientHelper::Get()->Load(kNotificationsFilename, callback);
}

void AdNotifications::OnLoaded(const bool success, const std::string& json) {
  if (!success) {
    BLOG(3, "Ad notifications state does not exist, creating default state");

    is_initialized_ = true;

    ad_notifications_.clear();
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load ad notifications state");

      BLOG(3, "Failed to parse ad notifications state: " << json);

      callback_(/* success */ false);
      return;
    }

    BLOG(3, "Successfully loaded ad notifications state");

    is_initialized_ = true;
  }

  callback_(/* success */ true);
}

bool AdNotifications::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  if (!GetNotificationsFromDictionary(dictionary)) {
    return false;
  }

  Save();

  return true;
}

bool AdNotifications::GetNotificationsFromDictionary(
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

  ad_notifications_ = GetNotificationsFromList(list);

  return true;
}

std::string AdNotifications::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto notifications = GetAsList();
  dictionary.SetKey(kNotificationsListKey,
                    base::Value(std::move(notifications)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value AdNotifications::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& ad_notification : ad_notifications_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(kNotificationUuidKey, base::Value(ad_notification.uuid));
    dictionary.SetKey(kNotificationCreativeInstanceIdKey,
                      base::Value(ad_notification.creative_instance_id));
    dictionary.SetKey(kNotificationCreativeSetIdKey,
                      base::Value(ad_notification.creative_set_id));
    dictionary.SetKey(kNotificationCampaignIdKey,
                      base::Value(ad_notification.campaign_id));
    dictionary.SetKey(kNotificationAdvertiserIdKey,
                      base::Value(ad_notification.advertiser_id));
    dictionary.SetKey(kNotificationSegmentKey,
                      base::Value(ad_notification.segment));
    dictionary.SetKey(kNotificationTitleKey,
                      base::Value(ad_notification.title));
    dictionary.SetKey(kNotificationBodyKey, base::Value(ad_notification.body));
    dictionary.SetKey(kNotificationTargetUrlKey,
                      base::Value(ad_notification.target_url));

    list.Append(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
