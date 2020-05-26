/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include <map>
#include <memory>
#include <utility>

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"
#include "base/containers/flat_map.h"
#include "base/logging.h"

namespace bat_ads {

namespace {

int32_t ToMojomURLRequestMethod(
    const ads::URLRequestMethod method) {
  return (int32_t)method;
}

std::map<std::string, std::string> ToStdMap(
    const base::flat_map<std::string, std::string>& map) {
  std::map<std::string, std::string> std_map;

  for (const auto& it : map) {
    std_map[it.first] = it.second;
  }

  return std_map;
}

ads::Result ToAdsResult(
    const int32_t result) {
  return (ads::Result)result;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info) {
  bat_ads_client_.Bind(std::move(client_info));
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() = default;

bool BatAdsClientMojoBridge::IsEnabled() const {
  if (!connected()) {
    return false;
  }

  bool is_enabled;
  bat_ads_client_->IsEnabled(&is_enabled);
  return is_enabled;
}

bool BatAdsClientMojoBridge::ShouldAllowAdConversionTracking() const {
  if (!connected()) {
    return false;
  }

  bool should_allow;
  bat_ads_client_->ShouldAllowAdConversionTracking(&should_allow);
  return should_allow;
}

bool BatAdsClientMojoBridge::CanShowBackgroundNotifications() const {
  if (!connected())
    return false;

  bool can_show;
  bat_ads_client_->CanShowBackgroundNotifications(&can_show);
  return can_show;
}

uint64_t BatAdsClientMojoBridge::GetAdsPerHour() const {
  if (!connected()) {
    return 0;
  }

  uint64_t ads_per_hour;
  bat_ads_client_->GetAdsPerHour(&ads_per_hour);
  return ads_per_hour;
}

uint64_t BatAdsClientMojoBridge::GetAdsPerDay() const {
  if (!connected()) {
    return 0;
  }

  uint64_t ads_per_day;
  bat_ads_client_->GetAdsPerDay(&ads_per_day);
  return ads_per_day;
}

void BatAdsClientMojoBridge::SetIdleThreshold(
    const int threshold) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetIdleThreshold(threshold);
}

bool BatAdsClientMojoBridge::IsNetworkConnectionAvailable() const {
  if (!connected()) {
    return false;
  }

  bool is_available;
  bat_ads_client_->IsNetworkConnectionAvailable(&is_available);
  return is_available;
}

void BatAdsClientMojoBridge::GetClientInfo(
    ads::ClientInfo* info) const {
  if (!connected()) {
    return;
  }

  std::string client_info;
  bat_ads_client_->GetClientInfo(info->ToJson(), &client_info);
  info->FromJson(client_info);
}

std::vector<std::string> BatAdsClientMojoBridge::GetUserModelLanguages() const {
  std::vector<std::string> languages;

  if (!connected()) {
    return languages;
  }

  bat_ads_client_->GetUserModelLanguages(&languages);
  return languages;
}

void OnLoadUserModelForLanguage(
    const ads::LoadCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::LoadUserModelForLanguage(
    const std::string& language,
    ads::LoadCallback callback) const {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->LoadUserModelForLanguage(language,
      base::BindOnce(&OnLoadUserModelForLanguage, std::move(callback)));
}

bool BatAdsClientMojoBridge::IsForeground() const {
  if (!connected()) {
    return false;
  }

  bool is_foreground;
  bat_ads_client_->IsForeground(&is_foreground);
  return is_foreground;
}

void BatAdsClientMojoBridge::ShowNotification(
    std::unique_ptr<ads::AdNotificationInfo> info) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ShowNotification(info->ToJson());
}

bool BatAdsClientMojoBridge::ShouldShowNotifications() {
  if (!connected()) {
    return false;
  }

  bool should_show;
  bat_ads_client_->ShouldShowNotifications(&should_show);
  return should_show;
}

void BatAdsClientMojoBridge::CloseNotification(
    const std::string& uuid) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->CloseNotification(uuid);
}

void BatAdsClientMojoBridge::SetCatalogIssuers(
    std::unique_ptr<ads::IssuersInfo> info) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetCatalogIssuers(info->ToJson());
}

void BatAdsClientMojoBridge::ConfirmAd(
    const ads::AdInfo& info,
    const ads::ConfirmationType confirmation_type) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ConfirmAd(info.ToJson(), confirmation_type);
}

void BatAdsClientMojoBridge::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ads::ConfirmationType confirmation_type) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ConfirmAction(creative_instance_id, creative_set_id,
      confirmation_type);
}

void OnURLRequest(
    const ads::URLRequestCallback& callback,
    const int32_t response_status_code,
    const std::string& content,
    const base::flat_map<std::string, std::string>& headers) {
  callback(response_status_code, content, ToStdMap(headers));
}

void BatAdsClientMojoBridge::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const ads::URLRequestMethod method,
    ads::URLRequestCallback callback) {
  if (!connected()) {
    callback(418, "", {});
    return;
  }

  bat_ads_client_->URLRequest(url, headers, content, content_type,
      ToMojomURLRequestMethod(method), base::BindOnce(&OnURLRequest,
          std::move(callback)));
}

void OnSave(
    const ads::ResultCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Save(
    const std::string& name,
    const std::string& value,
    ads::ResultCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Save(name, value, base::BindOnce(&OnSave,
      std::move(callback)));
}

void OnLoad(
    const ads::LoadCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::Load(
    const std::string& name,
    ads::LoadCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->Load(name, base::BindOnce(&OnLoad, std::move(callback)));
}

void OnReset(
    const ads::ResultCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Reset(
    const std::string& name,
    ads::ResultCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Reset(name, base::BindOnce(&OnReset, std::move(callback)));
}

std::string BatAdsClientMojoBridge::LoadJsonSchema(
    const std::string& name) {
  std::string json;

  if (!connected()) {
    return json;
  }

  bat_ads_client_->LoadJsonSchema(name, &json);
  return json;
}

void OnSaveBundleState(
    const ads::ResultCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::SaveBundleState(
    std::unique_ptr<ads::BundleState> bundle_state,
    ads::ResultCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->SaveBundleState(bundle_state->ToJson(),
      base::BindOnce(&OnSaveBundleState, std::move(callback)));
}

void OnGetCreativeAdNotifications(
    const ads::GetCreativeAdNotificationsCallback& callback,
    const int32_t result,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& json_list) {
  ads::CreativeAdNotificationList ads;

  for (const auto& json : json_list) {
    ads::CreativeAdNotificationInfo ad;
    if (ad.FromJson(json) != ads::Result::SUCCESS) {
      callback(ads::Result::FAILED, categories, {});
      return;
    }

    ads.push_back(ad);
  }

  callback(ToAdsResult(result), categories, ads);
}

void BatAdsClientMojoBridge::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::GetCreativeAdNotificationsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, categories, {});
    return;
  }

  bat_ads_client_->GetCreativeAdNotifications(categories,
      base::BindOnce(&OnGetCreativeAdNotifications, std::move(callback)));
}

void OnGetAdConversions(
    const ads::GetAdConversionsCallback& callback,
    const int32_t result,
    const std::vector<std::string>& json_list) {
  ads::AdConversionList ad_conversions;

  for (const auto& json : json_list) {
    ads::AdConversionInfo ad_conversion;
    if (ad_conversion.FromJson(json) != ads::Result::SUCCESS) {
      callback(ads::Result::FAILED, {});
      return;
    }

    ad_conversions.push_back(ad_conversion);
  }

  callback(ToAdsResult(result), ad_conversions);
}

void BatAdsClientMojoBridge::GetAdConversions(
    ads::GetAdConversionsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, {});
    return;
  }

  bat_ads_client_->GetAdConversions(
      base::BindOnce(&OnGetAdConversions, std::move(callback)));
}

void BatAdsClientMojoBridge::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) const {
  if (!connected()) {
    return;
  }

  bat_ads_client_->Log(file, line, verbose_level, message);
}

///////////////////////////////////////////////////////////////////////////////

bool BatAdsClientMojoBridge::connected() const {
  return bat_ads_client_.is_bound();
}

}  // namespace bat_ads
