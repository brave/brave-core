/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include <map>
#include <memory>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"

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

class LogStreamImpl : public ads::LogStream {
 public:
  LogStreamImpl(
      const char* file,
      const int line,
      const ads::LogLevel log_level) {
    switch (log_level) {
      case ads::LogLevel::LOG_INFO: {
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_INFO);
        break;
      }

      case ads::LogLevel::LOG_WARNING: {
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_WARNING);
        break;
      }

      default: {
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_ERROR);
        break;
      }
    }
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;

  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info) {
  bat_ads_client_.Bind(std::move(client_info));
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() {
}

bool BatAdsClientMojoBridge::IsEnabled() const {
  if (!connected()) {
    return false;
  }

  bool is_enabled;
  bat_ads_client_->IsEnabled(&is_enabled);
  return is_enabled;
}

bool
BatAdsClientMojoBridge::ShouldShowPublisherAdsOnPariticipatingSites() const {
  if (!connected()) {
    return false;
  }

  bool should_show;
  bat_ads_client_->ShouldShowPublisherAdsOnPariticipatingSites(&should_show);
  return should_show;
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

const std::string BatAdsClientMojoBridge::GetLocale() const {
  if (!connected()) {
    return "en-US";
  }

  std::string locale;
  bat_ads_client_->GetLocale(&locale);
  return locale;
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

  bool available;
  bat_ads_client_->IsNetworkConnectionAvailable(&available);
  return available;
}

void BatAdsClientMojoBridge::GetClientInfo(
    ads::ClientInfo* info) const {
  if (!connected()) {
    return;
  }

  std::string out_info;
  bat_ads_client_->GetClientInfo(info->ToJson(), &out_info);
  info->FromJson(out_info);
}

const std::vector<std::string>
BatAdsClientMojoBridge::GetUserModelLanguages() const {
  if (!connected()) {
    return {};
  }

  std::vector<std::string> languages;
  bat_ads_client_->GetUserModelLanguages(&languages);
  return languages;
}

void OnLoadUserModelForLanguage(
    const ads::OnLoadCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::LoadUserModelForLanguage(
    const std::string& language,
    ads::OnLoadCallback callback) const {
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
    const std::string& id) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->CloseNotification(id);
}

void BatAdsClientMojoBridge::SetCatalogIssuers(
    std::unique_ptr<ads::IssuersInfo> info) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetCatalogIssuers(info->ToJson());
}

void BatAdsClientMojoBridge::ConfirmAdNotification(
    std::unique_ptr<ads::AdNotificationInfo> info) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ConfirmAdNotification(info->ToJson());
}

void BatAdsClientMojoBridge::ConfirmPublisherAd(
    const ads::PublisherAdInfo& info) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ConfirmPublisherAd(info.ToJson());
}

void BatAdsClientMojoBridge::ConfirmAction(
    const std::string& uuid,
    const std::string& creative_set_id,
    const ads::ConfirmationType& type) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->ConfirmAction(uuid, creative_set_id, type);
}

uint32_t BatAdsClientMojoBridge::SetTimer(
    const uint64_t time_offset) {
  if (!connected()) {
    return 0;
  }

  uint32_t timer_id;
  bat_ads_client_->SetTimer(time_offset, &timer_id);
  return timer_id;
}

void BatAdsClientMojoBridge::KillTimer(
    const uint32_t timer_id) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->KillTimer(timer_id);
}

void OnURLRequest(
    const ads::URLRequestCallback& callback,
    const int32_t status_code,
    const std::string& content,
    const base::flat_map<std::string, std::string>& headers) {
  callback(status_code, content, ToStdMap(headers));
}

void BatAdsClientMojoBridge::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const ads::URLRequestMethod method,
    ads::URLRequestCallback callback) {
  if (!connected()) {
    callback(418, "", std::map<std::string, std::string>());
    return;
  }

  bat_ads_client_->URLRequest(url, headers, content, content_type,
      ToMojomURLRequestMethod(method), base::BindOnce(&OnURLRequest,
          std::move(callback)));
}

void OnSave(
    const ads::OnSaveCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Save(
    const std::string& name,
    const std::string& value,
    ads::OnSaveCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Save(name, value, base::BindOnce(&OnSave,
      std::move(callback)));
}

void OnLoad(
    const ads::OnLoadCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::Load(
    const std::string& name,
    ads::OnLoadCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->Load(name, base::BindOnce(&OnLoad, std::move(callback)));
}

void OnReset(
    const ads::OnResetCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Reset(
    const std::string& name,
    ads::OnResetCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Reset(name, base::BindOnce(&OnReset, std::move(callback)));
}

const std::string BatAdsClientMojoBridge::LoadJsonSchema(
    const std::string& name) {
  if (!connected()) {
    return "";
  }

  std::string json;
  bat_ads_client_->LoadJsonSchema(name, &json);
  return json;
}

void OnLoadSampleBundle(
    const ads::OnLoadSampleBundleCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::LoadSampleBundle(
    ads::OnLoadSampleBundleCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->LoadSampleBundle(
      base::BindOnce(&OnLoadSampleBundle, std::move(callback)));
}

void OnSaveBundleState(
    const ads::OnSaveCallback& callback,
    const int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::SaveBundleState(
    std::unique_ptr<ads::BundleState> bundle_state,
    ads::OnSaveCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->SaveBundleState(bundle_state->ToJson(),
      base::BindOnce(&OnSaveBundleState, std::move(callback)));
}

void OnGetCreativeAdNotifications(
    const ads::OnGetCreativeAdNotificationsCallback& callback,
    const int32_t result,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& ads_json) {
  ads::CreativeAdNotifications ads;

  for (const auto& ad_json : ads_json) {
    ads::CreativeAdNotificationInfo ad;
    if (ad.FromJson(ad_json) != ads::Result::SUCCESS) {
      callback(ads::Result::FAILED, categories, {});
      return;
    }

    ads.push_back(ad);
  }

  callback(ToAdsResult(result), categories, ads);
}

void BatAdsClientMojoBridge::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::OnGetCreativeAdNotificationsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, categories, {});
    return;
  }

  bat_ads_client_->GetCreativeAdNotifications(categories,
      base::BindOnce(&OnGetCreativeAdNotifications, std::move(callback)));
}

void OnGetCreativePublisherAds(
    const ads::OnGetCreativePublisherAdsCallback& callback,
    const int32_t result,
    const std::string& url,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& sizes,
    const std::vector<std::string>& ads_json) {
  ads::CreativePublisherAds ads;

  for (const auto& json : ads_json) {
    ads::CreativePublisherAdInfo ad;
    if (ad.FromJson(json) != ads::Result::SUCCESS) {
      callback(ads::Result::FAILED, url, categories, sizes, {});
      return;
    }

    ads.push_back(ad);
  }

  callback(ToAdsResult(result), url, categories, sizes, ads);
}

void BatAdsClientMojoBridge::GetCreativePublisherAds(
    const std::string& url,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& sizes,
    ads::OnGetCreativePublisherAdsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, url, categories, sizes, {});
    return;
  }

  bat_ads_client_->GetCreativePublisherAds(url, categories, sizes,
      base::BindOnce(&OnGetCreativePublisherAds, std::move(callback)));
}

void OnGetAdConversions(
    const ads::OnGetAdConversionsCallback& callback,
    const int32_t result,
    const std::string& url,
    const std::vector<std::string>& ad_conversion_json_list) {
  ads::AdConversions ad_conversions;

  for (const auto& json : ad_conversion_json_list) {
    ads::AdConversionTrackingInfo ad_conversion;
    if (ad_conversion.FromJson(json) != ads::Result::SUCCESS) {
      callback(ads::Result::FAILED, url, {});
      return;
    }

    ad_conversions.push_back(ad_conversion);
  }

  callback(ToAdsResult(result), url, ad_conversions);
}

void BatAdsClientMojoBridge::GetAdConversions(
    const std::string& url,
    ads::OnGetAdConversionsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, url, {});
    return;
  }

  bat_ads_client_->GetAdConversions(url, base::BindOnce(&OnGetAdConversions,
      std::move(callback)));
}

void BatAdsClientMojoBridge::EventLog(
    const std::string& json) const {
  if (!connected()) {
    return;
  }

  bat_ads_client_->EventLog(json);
}

std::unique_ptr<ads::LogStream> BatAdsClientMojoBridge::Log(
    const char* file,
    const int line,
    const ads::LogLevel log_level) const {
  return std::make_unique<LogStreamImpl>(file, line, log_level);
}

///////////////////////////////////////////////////////////////////////////////

bool BatAdsClientMojoBridge::connected() const {
  return bat_ads_client_.is_bound();
}

}  // namespace bat_ads
