/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"

#include <functional>
#include <map>
#include <memory>
#include <utility>

#include "bat/ads/ads.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ads {

namespace {

ads::URLRequestMethod ToAdsURLRequestMethod(int32_t method) {
  return (ads::URLRequestMethod)method;
}

base::flat_map<std::string, std::string> ToFlatMap(
    const std::map<std::string, std::string>& map) {
  base::flat_map<std::string, std::string> flat_map;

  for (const auto& it : map) {
    flat_map[it.first] = it.second;
  }

  return flat_map;
}

int32_t ToMojomResult(
    const ads::Result result) {
  return (int32_t)result;
}

}  // namespace

AdsClientMojoBridge::AdsClientMojoBridge(
    ads::AdsClient* ads_client)
    : ads_client_(ads_client) {
  DCHECK(ads_client_);
}

AdsClientMojoBridge::~AdsClientMojoBridge() = default;

bool AdsClientMojoBridge::IsEnabled(
    bool* out_is_enabled) {
  DCHECK(out_is_enabled);
  *out_is_enabled = ads_client_->IsEnabled();
  return true;
}

void AdsClientMojoBridge::IsEnabled(
    IsEnabledCallback callback) {
  std::move(callback).Run(ads_client_->IsEnabled());
}

bool AdsClientMojoBridge::ShouldAllowAdConversionTracking(
    bool* out_should_allow) {
  DCHECK(out_should_allow);
  *out_should_allow = ads_client_->ShouldAllowAdConversionTracking();
  return true;
}

void AdsClientMojoBridge::ShouldAllowAdConversionTracking(
    ShouldAllowAdConversionTrackingCallback callback) {
  std::move(callback).Run(ads_client_->ShouldAllowAdConversionTracking());
}

bool AdsClientMojoBridge::IsForeground(
    bool* out_is_foreground) {
  DCHECK(out_is_foreground);
  *out_is_foreground = ads_client_->IsForeground();
  return true;
}

void AdsClientMojoBridge::IsForeground(
    IsForegroundCallback callback) {
  std::move(callback).Run(ads_client_->IsForeground());
}

bool AdsClientMojoBridge::CanShowBackgroundNotifications(
    bool* out_can_show) {
  DCHECK(out_can_show);
  *out_can_show = ads_client_->CanShowBackgroundNotifications();
  return true;
}

void AdsClientMojoBridge::CanShowBackgroundNotifications(
    CanShowBackgroundNotificationsCallback callback) {
  std::move(callback).Run(ads_client_->CanShowBackgroundNotifications());
}

bool AdsClientMojoBridge::GetLocale(
    std::string* out_locale) {
  DCHECK(out_locale);
  *out_locale = ads_client_->GetLocale();
  return true;
}

void AdsClientMojoBridge::GetLocale(
    GetLocaleCallback callback) {
  std::move(callback).Run(ads_client_->GetLocale());
}

bool AdsClientMojoBridge::GetAdsPerHour(
    uint64_t* out_ads_per_hour) {
  DCHECK(out_ads_per_hour);
  *out_ads_per_hour = ads_client_->GetAdsPerHour();
  return true;
}

void AdsClientMojoBridge::GetAdsPerHour(
    GetAdsPerHourCallback callback) {
  std::move(callback).Run(ads_client_->GetAdsPerHour());
}

bool AdsClientMojoBridge::GetAdsPerDay(
    uint64_t* out_ads_per_day) {
  DCHECK(out_ads_per_day);
  *out_ads_per_day = ads_client_->GetAdsPerDay();
  return true;
}

void AdsClientMojoBridge::GetAdsPerDay(GetAdsPerDayCallback callback) {
  std::move(callback).Run(ads_client_->GetAdsPerDay());
}

bool AdsClientMojoBridge::IsNetworkConnectionAvailable(
    bool* out_is_available) {
  DCHECK(out_is_available);
  *out_is_available = ads_client_->IsNetworkConnectionAvailable();
  return true;
}

void AdsClientMojoBridge::IsNetworkConnectionAvailable(
    IsNetworkConnectionAvailableCallback callback) {
  std::move(callback).Run(ads_client_->IsNetworkConnectionAvailable());
}

bool AdsClientMojoBridge::ShouldShowNotifications(
    bool* out_should_show) {
  DCHECK(out_should_show);
  *out_should_show = ads_client_->ShouldShowNotifications();
  return true;
}

void AdsClientMojoBridge::ShouldShowNotifications(
    ShouldShowNotificationsCallback callback) {
  std::move(callback).Run(ads_client_->ShouldShowNotifications());
}

bool AdsClientMojoBridge::SetTimer(
    uint64_t time_offset,
    uint32_t* out_timer_id) {
  DCHECK(out_timer_id);
  *out_timer_id = ads_client_->SetTimer(time_offset);
  return true;
}

void AdsClientMojoBridge::SetTimer(
    uint64_t time_offset,
    SetTimerCallback callback) {
  std::move(callback).Run(ads_client_->SetTimer(time_offset));
}

bool AdsClientMojoBridge::LoadJsonSchema(
    const std::string& name,
    std::string* out_json) {
  DCHECK(out_json);
  *out_json = ads_client_->LoadJsonSchema(name);
  return true;
}

void AdsClientMojoBridge::LoadJsonSchema(
    const std::string& name,
    LoadJsonSchemaCallback callback) {
  std::move(callback).Run(ads_client_->LoadJsonSchema(name));
}

bool AdsClientMojoBridge::GetUserModelLanguages(
    std::vector<std::string>* out_languages) {
  DCHECK(out_languages);
  *out_languages = ads_client_->GetUserModelLanguages();
  return true;
}

void AdsClientMojoBridge::GetUserModelLanguages(
    GetUserModelLanguagesCallback callback) {
  std::move(callback).Run(ads_client_->GetUserModelLanguages());
}

void AdsClientMojoBridge::SetIdleThreshold(
    const int32_t threshold) {
  ads_client_->SetIdleThreshold(threshold);
}

void AdsClientMojoBridge::KillTimer(
    const uint32_t timer_id) {
  ads_client_->KillTimer(timer_id);
}

bool AdsClientMojoBridge::GetClientInfo(
    const std::string& client_info,
    std::string* out_client_info) {
  DCHECK(out_client_info);
  ads::ClientInfo info;
  info.FromJson(client_info);
  ads_client_->GetClientInfo(&info);
  *out_client_info = info.ToJson();
  return true;
}

void AdsClientMojoBridge::GetClientInfo(
    const std::string& client_info,
    GetClientInfoCallback callback) {
  ads::ClientInfo info;
  info.FromJson(client_info);
  ads_client_->GetClientInfo(&info);
  std::move(callback).Run(info.ToJson());
}

void AdsClientMojoBridge::EventLog(
    const std::string& json) {
  ads_client_->EventLog(json);
}

// static
void AdsClientMojoBridge::OnLoad(
    CallbackHolder<LoadCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result), std::move(value));
  }

  delete holder;
}

void AdsClientMojoBridge::Load(
    const std::string& name,
    LoadCallback callback) {
  // this gets deleted in OnLoad
  auto* holder =
      new CallbackHolder<LoadCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->Load(
      name, std::bind(AdsClientMojoBridge::OnLoad, holder, _1, _2));
}

// static
void AdsClientMojoBridge::OnSave(
    CallbackHolder<SaveCallback>* holder,
    const ads::Result result) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void AdsClientMojoBridge::Save(
    const std::string& name,
    const std::string& value,
    SaveCallback callback) {
  // this gets deleted in OnSave
  auto* holder =
      new CallbackHolder<SaveCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->Save(name, value,
      std::bind(AdsClientMojoBridge::OnSave, holder, _1));
}

// static
void AdsClientMojoBridge::OnReset(
    CallbackHolder<ResetCallback>* holder,
    const ads::Result result) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void AdsClientMojoBridge::Reset(
    const std::string& name,
    ResetCallback callback) {
  // this gets deleted in OnReset
  auto* holder =
      new CallbackHolder<ResetCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->Reset(name, std::bind(AdsClientMojoBridge::OnReset, holder, _1));
}

// static
void AdsClientMojoBridge::OnLoadUserModelForLanguage(
    CallbackHolder<LoadUserModelForLanguageCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result), std::move(value));
  }

  delete holder;
}

void AdsClientMojoBridge::LoadUserModelForLanguage(
    const std::string& language,
    LoadUserModelForLanguageCallback callback) {
  // this gets deleted in OnLoadUserModelForLanguage
  auto* holder = new CallbackHolder<LoadUserModelForLanguageCallback>(
      AsWeakPtr(), std::move(callback));
  ads_client_->LoadUserModelForLanguage(language,
      std::bind(AdsClientMojoBridge::OnLoadUserModelForLanguage,
          holder, _1, _2));
}

// static
void AdsClientMojoBridge::OnURLRequest(
    CallbackHolder<URLRequestCallback>* holder,
    const int response_status_code,
    const std::string& content,
    const std::map<std::string, std::string>& headers) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(response_status_code, content,
        ToFlatMap(headers));
  }

  delete holder;
}

void AdsClientMojoBridge::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const int32_t method,
    URLRequestCallback callback) {
  // this gets deleted in OnURLRequest
  auto* holder =
      new CallbackHolder<URLRequestCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->URLRequest(url, headers, content, content_type,
      ToAdsURLRequestMethod(method),
          std::bind(AdsClientMojoBridge::OnURLRequest,
              holder, _1, _2, _3));
}

// static
void AdsClientMojoBridge::OnLoadSampleBundle(
    CallbackHolder<LoadSampleBundleCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result), std::move(value));
  }

  delete holder;
}

void AdsClientMojoBridge::LoadSampleBundle(
    LoadSampleBundleCallback callback) {
  // this gets deleted in OnLoadSampleBundle
  auto* holder = new CallbackHolder<LoadSampleBundleCallback>(
      AsWeakPtr(), std::move(callback));
  ads_client_->LoadSampleBundle(
      std::bind(AdsClientMojoBridge::OnLoadSampleBundle, holder, _1, _2));
}

void AdsClientMojoBridge::ShowNotification(
    const std::string& notification_info) {
  auto info = std::make_unique<ads::AdNotificationInfo>();
  if (info->FromJson(notification_info) != ads::Result::SUCCESS) {
    return;
  }

  ads_client_->ShowNotification(std::move(info));
}

void AdsClientMojoBridge::CloseNotification(
    const std::string& uuid) {
  ads_client_->CloseNotification(uuid);
}

void AdsClientMojoBridge::SetCatalogIssuers(
    const std::string& issuers_info) {
  auto info = std::make_unique<ads::IssuersInfo>();
  if (info->FromJson(issuers_info) != ads::Result::SUCCESS) {
    return;
  }

  ads_client_->SetCatalogIssuers(std::move(info));
}

void AdsClientMojoBridge::ConfirmAd(
    const std::string& json,
    const std::string& confirmation_type) {
  ads::AdInfo info;
  if (info.FromJson(json) != ads::Result::SUCCESS) {
    return;
  }

  ads_client_->ConfirmAd(info, ads::ConfirmationType(confirmation_type));
}

void AdsClientMojoBridge::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const std::string& confirmation_type) {
  ads_client_->ConfirmAction(creative_instance_id, creative_set_id,
      ads::ConfirmationType(confirmation_type));
}

// static
void AdsClientMojoBridge::OnSaveBundleState(
    CallbackHolder<SaveBundleStateCallback>* holder,
    const ads::Result result) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void AdsClientMojoBridge::SaveBundleState(
    const std::string& bundle_state_json,
    SaveBundleStateCallback callback) {
  // this gets deleted in OnSaveBundleState
  auto* holder = new CallbackHolder<SaveBundleStateCallback>(
      AsWeakPtr(), std::move(callback));

  auto bundle_state = std::make_unique<ads::BundleState>();

  auto schema = ads_client_->LoadJsonSchema(ads::_bundle_schema_resource_name);

  if (bundle_state->FromJson(bundle_state_json, schema) !=
      ads::Result::SUCCESS) {
    std::move(holder->get()).Run(ToMojomResult(ads::Result::FAILED));
    return;
  }

  ads_client_->SaveBundleState(std::move(bundle_state),
      std::bind(AdsClientMojoBridge::OnSaveBundleState, holder, _1));
}

// static
void AdsClientMojoBridge::OnGetCreativeAdNotifications(
    CallbackHolder<GetCreativeAdNotificationsCallback>* holder,
    const ads::Result result,
    const std::vector<std::string>& categories,
    const ads::CreativeAdNotificationList& ads) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::vector<std::string> json_list;

    for (const auto& ad : ads) {
      json_list.push_back(ad.ToJson());
    }

    std::move(holder->get()).Run(ToMojomResult(result), categories, json_list);
  }

  delete holder;
}

void AdsClientMojoBridge::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    GetCreativeAdNotificationsCallback callback) {
  // this gets deleted in OnGetCreativeAdNotifications
  auto* holder = new CallbackHolder<GetCreativeAdNotificationsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_client_->GetCreativeAdNotifications(categories,
      std::bind(AdsClientMojoBridge::OnGetCreativeAdNotifications,
          holder, _1, _2, _3));
}

// static
void AdsClientMojoBridge::OnGetAdConversions(
    CallbackHolder<GetAdConversionsCallback>* holder,
    const ads::Result result,
    const std::string& url,
    const ads::AdConversionList& ad_conversions) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::vector<std::string> json_list;

    for (const auto& ad_conversion : ad_conversions) {
      json_list.push_back(ad_conversion.ToJson());
    }

    std::move(holder->get()).Run(ToMojomResult(result), url, json_list);
  }

  delete holder;
}

void AdsClientMojoBridge::GetAdConversions(
    const std::string& url,
    GetAdConversionsCallback callback) {
  // this gets deleted in OnGetAdConversions
  auto* holder = new CallbackHolder<GetAdConversionsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_client_->GetAdConversions(url,
      std::bind(AdsClientMojoBridge::OnGetAdConversions, holder, _1, _2, _3));
}

}  // namespace bat_ads
