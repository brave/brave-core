/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "base/check_op.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/notification_ad_info.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/build_info.h"
#include "base/system/sys_info.h"
#endif

namespace ads {

namespace {

NotificationAdManager* g_notification_ads_instance = nullptr;

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

NotificationAdManager::NotificationAdManager() {
  DCHECK(!g_notification_ads_instance);
  g_notification_ads_instance = this;
}

NotificationAdManager::~NotificationAdManager() {
  DCHECK_EQ(this, g_notification_ads_instance);
  g_notification_ads_instance = nullptr;
}

// static
NotificationAdManager* NotificationAdManager::GetInstance() {
  DCHECK(g_notification_ads_instance);
  return g_notification_ads_instance;
}

// static
bool NotificationAdManager::HasInstance() {
  return !!g_notification_ads_instance;
}

void NotificationAdManager::Initialize(InitializeCallback callback) {
  callback_ = callback;

  Load();
}

absl::optional<NotificationAdInfo> NotificationAdManager::GetForPlacementId(
    const std::string& placement_id) const {
  DCHECK(is_initialized_);

  auto iter =
      std::find_if(notification_ads_.cbegin(), notification_ads_.cend(),
                   [&placement_id](const NotificationAdInfo& notification) {
                     return notification.placement_id == placement_id;
                   });
  if (iter == notification_ads_.end()) {
    return absl::nullopt;
  }

  NotificationAdInfo ad = *iter;
  ad.type = AdType::kNotificationAd;
  return ad;
}

void NotificationAdManager::PushBack(const NotificationAdInfo& ad) {
  DCHECK(is_initialized_);

  notification_ads_.push_back(ad);

  if (kMaximumNotificationAds > 0 && Count() > kMaximumNotificationAds) {
    PopFront(true);
  }

  Save();
}

void NotificationAdManager::PopFront(const bool should_dismiss) {
  if (!notification_ads_.empty()) {
    if (should_dismiss) {
      AdsClientHelper::GetInstance()->CloseNotificationAd(
          notification_ads_.front().placement_id);
    }
    notification_ads_.pop_front();
    Save();
  }
}

bool NotificationAdManager::Remove(const std::string& placement_id) {
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

void NotificationAdManager::RemoveAll() {
  DCHECK(is_initialized_);

  notification_ads_.clear();
  Save();
}

void NotificationAdManager::CloseAndRemoveAll() {
  DCHECK(is_initialized_);

  for (const auto& notification_ad : notification_ads_) {
    AdsClientHelper::GetInstance()->CloseNotificationAd(
        notification_ad.placement_id);
  }

  RemoveAll();
}

bool NotificationAdManager::Exists(const std::string& placement_id) const {
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

uint64_t NotificationAdManager::Count() const {
  return notification_ads_.size();
}

#if BUILDFLAG(IS_ANDROID)
void NotificationAdManager::RemoveAllAfterReboot() {
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

void NotificationAdManager::RemoveAllAfterUpdate() {
  const std::string current_version_code =
      base::android::BuildInfo::GetInstance()->package_version_code();

  const std::string last_version_code =
      ClientStateManager::GetInstance()->GetVersionCode();

  if (last_version_code == current_version_code) {
    return;
  }

  ClientStateManager::GetInstance()->SetVersionCode(current_version_code);

  RemoveAll();
}
#endif

///////////////////////////////////////////////////////////////////////////////

base::circular_deque<NotificationAdInfo>
NotificationAdManager::GetNotificationsFromList(
    const base::Value::List& list) const {
  base::circular_deque<NotificationAdInfo> notifications;

  for (auto& item : list) {
    const auto* dict = item.GetIfDict();
    if (!dict) {
      continue;
    }

    NotificationAdInfo notification_info;
    if (!GetNotificationFromDictionary(*dict, &notification_info)) {
      continue;
    }

    notifications.push_back(notification_info);
  }

  return notifications;
}

bool NotificationAdManager::GetNotificationFromDictionary(
    const base::Value::Dict& dict,
    NotificationAdInfo* notification_ad) const {
  NotificationAdInfo new_notification_ad;

  if (!GetPlacementIdFromDictionary(dict, &new_notification_ad.placement_id)) {
    return false;
  }

  if (!GetCreativeInstanceIdFromDictionary(
          dict, &new_notification_ad.creative_instance_id)) {
    return false;
  }

  if (!GetCreativeSetIdFromDictionary(dict,
                                      &new_notification_ad.creative_set_id)) {
    return false;
  }

  if (!GetCampaignIdFromDictionary(dict, &new_notification_ad.campaign_id)) {
    // Migrate for legacy notifications
    new_notification_ad.campaign_id = "";
  }

  if (!GetAdvertiserIdFromDictionary(dict,
                                     &new_notification_ad.advertiser_id)) {
    // Migrate for legacy notifications
    new_notification_ad.advertiser_id = "";
  }

  if (!GetSegmentFromDictionary(dict, &new_notification_ad.segment)) {
    // Migrate for legacy notifications
    if (!GetStringFromDictionary("category", dict,
                                 &new_notification_ad.segment)) {
      return false;
    }
  }

  if (!GetTitleFromDictionary(dict, &new_notification_ad.title)) {
    return false;
  }

  if (!GetBodyFromDictionary(dict, &new_notification_ad.body)) {
    return false;
  }

  std::string target_url;
  if (!GetTargetUrlFromDictionary(dict, &target_url)) {
    return false;
  }
  new_notification_ad.target_url = GURL(target_url);

  *notification_ad = new_notification_ad;

  return true;
}

bool NotificationAdManager::GetPlacementIdFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationPlacementIdKey, dict, value);
}

bool NotificationAdManager::GetCreativeInstanceIdFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeInstanceIdKey, dict,
                                 value);
}

bool NotificationAdManager::GetCreativeSetIdFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCreativeSetIdKey, dict, value);
}

bool NotificationAdManager::GetCampaignIdFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationCampaignIdKey, dict, value);
}

bool NotificationAdManager::GetAdvertiserIdFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationAdvertiserIdKey, dict, value);
}

bool NotificationAdManager::GetSegmentFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationSegmentKey, dict, value);
}

bool NotificationAdManager::GetTitleFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTitleKey, dict, value);
}

bool NotificationAdManager::GetBodyFromDictionary(const base::Value::Dict& dict,
                                                  std::string* value) const {
  return GetStringFromDictionary(kNotificationBodyKey, dict, value);
}

bool NotificationAdManager::GetTargetUrlFromDictionary(
    const base::Value::Dict& dict,
    std::string* value) const {
  return GetStringFromDictionary(kNotificationTargetUrlKey, dict, value);
}

bool NotificationAdManager::GetStringFromDictionary(
    const std::string& key,
    const base::Value::Dict& dict,
    std::string* string) const {
  DCHECK(string);

  const std::string* value = dict.FindString(key);
  if (!value) {
    return false;
  }

  *string = *value;

  return true;
}

void NotificationAdManager::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving notification ads state");

  std::string json = ToJson();
  auto callback =
      std::bind(&NotificationAdManager::OnSaved, this, std::placeholders::_1);
  AdsClientHelper::GetInstance()->Save(kNotificationsFilename, json, callback);
}

void NotificationAdManager::OnSaved(const bool success) {
  if (!success) {
    BLOG(0, "Failed to save notification ads state");
    return;
  }

  BLOG(9, "Successfully saved notification ads state");
}

void NotificationAdManager::Load() {
  BLOG(3, "Loading notification ads state");

  AdsClientHelper::GetInstance()->Load(
      kNotificationsFilename,
      base::BindOnce(&NotificationAdManager::OnLoaded, base::Unretained(this)));
}

void NotificationAdManager::OnLoaded(const bool success,
                                     const std::string& json) {
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

bool NotificationAdManager::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::Value::Dict& dict = value->GetDict();

  if (!GetNotificationsFromDictionary(dict)) {
    return false;
  }

  Save();

  return true;
}

bool NotificationAdManager::GetNotificationsFromDictionary(
    const base::Value::Dict& dict) {
  auto* value = dict.FindList(kNotificationsListKey);
  if (!value) {
    return false;
  }

  notification_ads_ = GetNotificationsFromList(*value);

  return true;
}

std::string NotificationAdManager::ToJson() {
  base::Value::Dict dict;
  dict.Set(kNotificationsListKey, GetAsList());

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

base::Value::List NotificationAdManager::GetAsList() {
  base::Value::List list;

  for (const auto& notification_ad : notification_ads_) {
    base::Value::Dict dict;

    dict.Set(kNotificationPlacementIdKey, notification_ad.placement_id);
    dict.Set(kNotificationCreativeInstanceIdKey,
             notification_ad.creative_instance_id);
    dict.Set(kNotificationCreativeSetIdKey, notification_ad.creative_set_id);
    dict.Set(kNotificationCampaignIdKey, notification_ad.campaign_id);
    dict.Set(kNotificationAdvertiserIdKey, notification_ad.advertiser_id);
    dict.Set(kNotificationSegmentKey, notification_ad.segment);
    dict.Set(kNotificationTitleKey, notification_ad.title);
    dict.Set(kNotificationBodyKey, notification_ad.body);
    dict.Set(kNotificationTargetUrlKey, notification_ad.target_url.spec());

    list.Append(std::move(dict));
  }

  return list;
}

}  // namespace ads
