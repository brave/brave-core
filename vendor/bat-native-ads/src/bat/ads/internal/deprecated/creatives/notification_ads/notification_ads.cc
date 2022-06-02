/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/creatives/notification_ads/notification_ads.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "base/check_op.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/build_info.h"
#include "base/system/sys_info.h"
#endif

namespace ads {

namespace {

NotificationAds* g_notification_ads_instance = nullptr;

#if BUILDFLAG(IS_ANDROID)
constexpr int kMaximumNotificationAds = 3;
#else
constexpr int kMaximumNotificationAds = 0;  // No limit
#endif

constexpr char kNotificationsFilename[] = "notifications.json";

constexpr char kNotificationsListKey[] = "notifications";

constexpr char kNotificationPlacementIdKey[] = "id";
constexpr char kNotificationCreativeInstanceIdKey[] = "uuid";
constexpr char kNotificationCreativeSetIdKey[] = "creative_set_id";
constexpr char kNotificationCampaignIdKey[] = "campaign_id";
constexpr char kNotificationAdvertiserIdKey[] = "advertiser_id";
constexpr char kNotificationSegmentKey[] = "segment";
constexpr char kNotificationTitleKey[] = "advertiser";
constexpr char kNotificationBodyKey[] = "text";
constexpr char kNotificationTargetUrlKey[] = "url";

}  // namespace

NotificationAds::NotificationAds() {
  DCHECK(!g_notification_ads_instance);
  g_notification_ads_instance = this;
}

NotificationAds::~NotificationAds() {
  DCHECK_EQ(this, g_notification_ads_instance);
  g_notification_ads_instance = nullptr;
}

// static
NotificationAds* NotificationAds::Get() {
  DCHECK(g_notification_ads_instance);
  return g_notification_ads_instance;
}

// static
bool NotificationAds::HasInstance() {
  return !!g_notification_ads_instance;
}

void NotificationAds::Initialize(InitializeCallback callback) {
  callback_ = callback;

  Load();
}

bool NotificationAds::Get(const std::string& placement_id,
                          NotificationAdInfo* notification_ad) const {
  DCHECK(is_initialized_);
  DCHECK(notification_ad);

  auto iter =
      std::find_if(notification_ads_.cbegin(), notification_ads_.cend(),
                   [&placement_id](const NotificationAdInfo& notification) {
                     return notification.placement_id == placement_id;
                   });

  if (iter == notification_ads_.end()) {
    return false;
  }

  *notification_ad = *iter;

  notification_ad->type = AdType::kNotificationAd;

  return true;
}

void NotificationAds::PushBack(const NotificationAdInfo& info) {
  DCHECK(is_initialized_);

  notification_ads_.push_back(info);

  if (kMaximumNotificationAds > 0 && Count() > kMaximumNotificationAds) {
    PopFront(true);
  }

  Save();
}

void NotificationAds::PopFront(const bool should_dismiss) {
  if (!notification_ads_.empty()) {
    if (should_dismiss) {
      AdsClientHelper::Get()->CloseNotification(
          notification_ads_.front().placement_id);
    }
    notification_ads_.pop_front();
    Save();
  }
}

bool NotificationAds::Remove(const std::string& placement_id) {
  DCHECK(is_initialized_);

  auto iter =
      std::find_if(notification_ads_.cbegin(), notification_ads_.cend(),
                   [&placement_id](const NotificationAdInfo& notification) {
                     return notification.placement_id == placement_id;
                   });

  if (iter == notification_ads_.end()) {
    return false;
  }

  notification_ads_.erase(iter);
  Save();

  return true;
}

void NotificationAds::RemoveAll() {
  DCHECK(is_initialized_);

  notification_ads_.clear();
  Save();
}

void NotificationAds::CloseAndRemoveAll() {
  DCHECK(is_initialized_);

  for (const auto& notification_ad : notification_ads_) {
    AdsClientHelper::Get()->CloseNotification(notification_ad.placement_id);
  }

  RemoveAll();
}

bool NotificationAds::Exists(const std::string& placement_id) const {
  DCHECK(is_initialized_);

  auto iter =
      std::find_if(notification_ads_.cbegin(), notification_ads_.cend(),
                   [&placement_id](const NotificationAdInfo& notification) {
                     return notification.placement_id == placement_id;
                   });

  if (iter == notification_ads_.end()) {
    return false;
  }

  return true;
}

uint64_t NotificationAds::Count() const {
  return notification_ads_.size();
}

#if BUILDFLAG(IS_ANDROID)
void NotificationAds::RemoveAllAfterReboot() {
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

void NotificationAds::RemoveAllAfterUpdate() {
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

base::circular_deque<NotificationAdInfo>
NotificationAds::GetNotificationsFromList(base::Value* list) const {
  DCHECK(list);
  DCHECK(list->is_list());

  base::circular_deque<NotificationAdInfo> notifications;

  for (auto& item : list->GetList()) {
    base::DictionaryValue* dictionary = nullptr;
    if (!item.GetAsDictionary(&dictionary)) {
      continue;
    }

    NotificationAdInfo notification_info;
    if (!GetNotificationFromDictionary(dictionary, &notification_info)) {
      continue;
    }

    notifications.push_back(notification_info);
  }

  return notifications;
}

bool NotificationAds::GetNotificationFromDictionary(
    base::DictionaryValue* dictionary,
    NotificationAdInfo* notification_ad) const {
  NotificationAdInfo new_notification_ad;

  if (!GetPlacementIdFromDictionary(dictionary,
                                    &new_notification_ad.placement_id)) {
    return false;
  }

  if (!GetCreativeInstanceIdFromDictionary(
          dictionary, &new_notification_ad.creative_instance_id)) {
    return false;
  }

  if (!GetCreativeSetIdFromDictionary(dictionary,
                                      &new_notification_ad.creative_set_id)) {
    return false;
  }

  if (!GetCampaignIdFromDictionary(dictionary,
                                   &new_notification_ad.campaign_id)) {
    // Migrate for legacy notifications
    new_notification_ad.campaign_id = "";
  }

  if (!GetAdvertiserIdFromDictionary(dictionary,
                                     &new_notification_ad.advertiser_id)) {
    // Migrate for legacy notifications
    new_notification_ad.advertiser_id = "";
  }

  if (!GetSegmentFromDictionary(dictionary, &new_notification_ad.segment)) {
    // Migrate for legacy notifications
    if (!GetStringFromDictionary("category", dictionary,
                                 &new_notification_ad.segment)) {
      return false;
    }
  }

  if (!GetTitleFromDictionary(dictionary, &new_notification_ad.title)) {
    return false;
  }

  if (!GetBodyFromDictionary(dictionary, &new_notification_ad.body)) {
    return false;
  }

  std::string target_url;
  if (!GetTargetUrlFromDictionary(dictionary, &target_url)) {
    return false;
  }
  new_notification_ad.target_url = GURL(target_url);

  *notification_ad = new_notification_ad;

  return true;
}

bool NotificationAds::GetPlacementIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationPlacementIdKey, dictionary,
                                 value);
}

bool NotificationAds::GetCreativeInstanceIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeInstanceIdKey, dictionary,
                                 value);
}

bool NotificationAds::GetCreativeSetIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeSetIdKey, dictionary,
                                 value);
}

bool NotificationAds::GetCampaignIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCampaignIdKey, dictionary, value);
}

bool NotificationAds::GetAdvertiserIdFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationAdvertiserIdKey, dictionary,
                                 value);
}

bool NotificationAds::GetSegmentFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationSegmentKey, dictionary, value);
}

bool NotificationAds::GetTitleFromDictionary(base::DictionaryValue* dictionary,
                                             std::string* value) const {
  return GetStringFromDictionary(kNotificationTitleKey, dictionary, value);
}

bool NotificationAds::GetBodyFromDictionary(base::DictionaryValue* dictionary,
                                            std::string* value) const {
  return GetStringFromDictionary(kNotificationBodyKey, dictionary, value);
}

bool NotificationAds::GetTargetUrlFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTargetUrlKey, dictionary, value);
}

bool NotificationAds::GetStringFromDictionary(const std::string& key,
                                              base::DictionaryValue* dictionary,
                                              std::string* string) const {
  DCHECK(dictionary);
  DCHECK(string);

  std::string* value = dictionary->FindStringKey(key);
  if (!value) {
    return false;
  }

  *string = *value;

  return true;
}

void NotificationAds::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving notification ads state");

  std::string json = ToJson();
  auto callback =
      std::bind(&NotificationAds::OnSaved, this, std::placeholders::_1);
  AdsClientHelper::Get()->Save(kNotificationsFilename, json, callback);
}

void NotificationAds::OnSaved(const bool success) {
  if (!success) {
    BLOG(0, "Failed to save notification ads state");
    return;
  }

  BLOG(9, "Successfully saved notification ads state");
}

void NotificationAds::Load() {
  BLOG(3, "Loading notification ads state");

  auto callback = std::bind(&NotificationAds::OnLoaded, this,
                            std::placeholders::_1, std::placeholders::_2);
  AdsClientHelper::Get()->Load(kNotificationsFilename, callback);
}

void NotificationAds::OnLoaded(const bool success, const std::string& json) {
  if (!success) {
    BLOG(3, "Notification ads state does not exist, creating default state");

    is_initialized_ = true;

    notification_ads_.clear();
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load notification ads state");

      BLOG(3, "Failed to parse notification ads state: " << json);

      callback_(/* success */ false);
      return;
    }

    BLOG(3, "Successfully loaded notification ads state");

    is_initialized_ = true;
  }

  callback_(/* success */ true);
}

bool NotificationAds::FromJson(const std::string& json) {
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

bool NotificationAds::GetNotificationsFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  auto* value = dictionary->FindListKey(kNotificationsListKey);
  if (!value) {
    return false;
  }

  notification_ads_ = GetNotificationsFromList(value);

  return true;
}

std::string NotificationAds::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto notifications = GetAsList();
  dictionary.SetKey(kNotificationsListKey, std::move(notifications));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value NotificationAds::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& notification_ad : notification_ads_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetStringKey(kNotificationPlacementIdKey,
                            notification_ad.placement_id);
    dictionary.SetStringKey(kNotificationCreativeInstanceIdKey,
                            notification_ad.creative_instance_id);
    dictionary.SetStringKey(kNotificationCreativeSetIdKey,
                            notification_ad.creative_set_id);
    dictionary.SetStringKey(kNotificationCampaignIdKey,
                            notification_ad.campaign_id);
    dictionary.SetStringKey(kNotificationAdvertiserIdKey,
                            notification_ad.advertiser_id);
    dictionary.SetStringKey(kNotificationSegmentKey, notification_ad.segment);
    dictionary.SetStringKey(kNotificationTitleKey, notification_ad.title);
    dictionary.SetStringKey(kNotificationBodyKey, notification_ad.body);
    dictionary.SetStringKey(kNotificationTargetUrlKey,
                            notification_ad.target_url.spec());

    list.Append(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
