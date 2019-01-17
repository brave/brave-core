/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"

namespace bat_ads {

namespace {

int32_t ToMojomURLRequestMethod(
    ads::URLRequestMethod method) {
  return (int32_t)method;
}

std::map<std::string, std::string> ToStdMap(
    const base::flat_map<std::string, std::string>& map) {
  std::map<std::string, std::string> std_map;
  for (const auto it : map) {
    std_map[it.first] = it.second;
  }
  return std_map;
}

ads::Result ToAdsResult(int32_t result) {
  return (ads::Result)result;
}

class LogStreamImpl : public ads::LogStream {
 public:
  LogStreamImpl(const char* file,
                int line,
                const ads::LogLevel log_level) {
    switch(log_level) {
      case ads::LogLevel::LOG_INFO:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_INFO);
        break;
      case ads::LogLevel::LOG_WARNING:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_WARNING);
        break;
      default:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_ERROR);
    }
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;

  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

}

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojom::BatAdsClientAssociatedPtrInfo client_info){
  bat_ads_client_.Bind(std::move(client_info));
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() {}

bool BatAdsClientMojoBridge::IsAdsEnabled() const {
  if (!connected())
    return false;

  bool is_enabled;
  bat_ads_client_->IsAdsEnabled(&is_enabled);
  return is_enabled;
}

bool BatAdsClientMojoBridge::IsForeground() const {
  if (!connected())
    return false;

  bool is_foreground;
  bat_ads_client_->IsForeground(&is_foreground);
  return is_foreground;
}

const std::string BatAdsClientMojoBridge::GetAdsLocale() const {
  if (!connected())
    return "en-US";

  std::string locale;
  bat_ads_client_->GetAdsLocale(&locale);
  return locale;
}

uint64_t BatAdsClientMojoBridge::GetAdsPerHour() const {
  if (!connected())
    return 0;

  uint64_t ads_per_hour;
  bat_ads_client_->GetAdsPerHour(&ads_per_hour);
  return ads_per_hour;
}

uint64_t BatAdsClientMojoBridge::GetAdsPerDay() const {
  if (!connected())
    return 0;

  uint64_t ads_per_day;
  bat_ads_client_->GetAdsPerDay(&ads_per_day);
  return ads_per_day;
}

void BatAdsClientMojoBridge::GetClientInfo(ads::ClientInfo* info) const {
  if (!connected())
    return;

  std::string out_info_json;
  bat_ads_client_->GetClientInfo(info->ToJson(), &out_info_json);
  info->FromJson(out_info_json);
}

const std::vector<std::string> BatAdsClientMojoBridge::GetLocales() const {
  if (!connected())
    return std::vector<std::string>();

  std::vector<std::string> locales;
  bat_ads_client_->GetLocales(&locales);
  return locales;
}

const std::string BatAdsClientMojoBridge::GenerateUUID() const {
  if (!connected())
    return "";

  std::string uuid;
  bat_ads_client_->GenerateUUID(&uuid);
  return uuid;
}

void BatAdsClientMojoBridge::ShowNotification(
    std::unique_ptr<ads::NotificationInfo> info) {
  if (!connected())
    return;

  bat_ads_client_->ShowNotification(info->ToJson());
}

void BatAdsClientMojoBridge::SetCatalogIssuers(
    std::unique_ptr<ads::IssuersInfo> info) {
  if (!connected())
    return;

  bat_ads_client_->SetCatalogIssuers(info->ToJson());
}

bool BatAdsClientMojoBridge::IsConfirmationsReadyToShowAds() {
  if (!connected())
    return false;

  bool can_show;
  bat_ads_client_->IsConfirmationsReadyToShowAds(&can_show);
  return can_show;
}

void BatAdsClientMojoBridge::AdSustained(
    std::unique_ptr<ads::NotificationInfo> info) {
  if (!connected())
    return;

  bat_ads_client_->AdSustained(info->ToJson());
}

uint32_t BatAdsClientMojoBridge::SetTimer(const uint64_t time_offset) {
  if (!connected())
    return 0;

  uint32_t timer_id;
  bat_ads_client_->SetTimer(time_offset, &timer_id);
  return timer_id;
}

void BatAdsClientMojoBridge::KillTimer(uint32_t timer_id) {
  if (!connected())
    return;

  bat_ads_client_->KillTimer(timer_id);
}

void OnURLRequest(const ads::URLRequestCallback& callback,
                  int32_t status_code,
                  const std::string& content,
                  const base::flat_map<std::string, std::string>& headers) {
  callback(status_code, content, ToStdMap(headers));
}

void BatAdsClientMojoBridge::URLRequest(const std::string& url,
                const std::vector<std::string>& headers,
                const std::string& content,
                const std::string& content_type,
                ads::URLRequestMethod method,
                ads::URLRequestCallback callback) {
  if (!connected()) {
    callback(418, "", std::map<std::string, std::string>());
    return;
  }

  bat_ads_client_->URLRequest(url,
                              headers,
                              content,
                              content_type,
                              ToMojomURLRequestMethod(method),
                              base::BindOnce(&OnURLRequest,
                                  std::move(callback)));
}

void OnSave(const ads::OnSaveCallback& callback,
            int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Save(const std::string& name,
                                 const std::string& value,
                                 ads::OnSaveCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Save(name, value,
      base::BindOnce(&OnSave, std::move(callback)));
}

void OnLoad(const ads::OnLoadCallback& callback,
            int32_t result,
            const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::Load(const std::string& name,
                                 ads::OnLoadCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->Load(name, base::BindOnce(&OnLoad, std::move(callback)));
}

void OnSaveBundleState(const ads::OnSaveCallback& callback,
                       int32_t result) {
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

const std::string BatAdsClientMojoBridge::LoadJsonSchema(
    const std::string& name) {
  if (!connected())
    return "{}";

  std::string json;
  bat_ads_client_->LoadJsonSchema(name, &json);
  return json;
}

void OnReset(const ads::OnResetCallback& callback,
            int32_t result) {
  callback(ToAdsResult(result));
}

void BatAdsClientMojoBridge::Reset(const std::string& name,
                                  ads::OnResetCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED);
    return;
  }

  bat_ads_client_->Reset(name, base::BindOnce(&OnReset, std::move(callback)));
}

void OnGetAds(const ads::OnGetAdsCallback& callback,
              int32_t result,
              const std::string& region,
              const std::string& category,
              const std::vector<std::string>& ad_info_json_list) {
  std::vector<ads::AdInfo> ad_info_list;

  for (const auto it : ad_info_json_list) {
    ads::AdInfo ad_info;
    if (ad_info.FromJson(it) == ads::Result::SUCCESS) {
      ad_info_list.push_back(ad_info);
    } else {
      callback(
          ads::Result::FAILED, region, category, std::vector<ads::AdInfo>());
      return;
    }
  }

  callback(ToAdsResult(result), region, category, ad_info_list);
}

void BatAdsClientMojoBridge::GetAds(
    const std::string& region,
    const std::string& category,
    ads::OnGetAdsCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, region, category, std::vector<ads::AdInfo>());
    return;
  }

  bat_ads_client_->GetAds(region, category,
      base::BindOnce(&OnGetAds, std::move(callback)));
}

void OnLoadSampleBundle(const ads::OnLoadSampleBundleCallback& callback,
                        int32_t result,
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

bool BatAdsClientMojoBridge::GetUrlComponents(
    const std::string& url,
    ads::UrlComponents* components) const {
  if (!connected())
    return false;

  bool out_result;
  std::string out_components_json;
  bat_ads_client_->GetUrlComponents(url, &out_result, &out_components_json);
  components->FromJson(out_components_json);

  return out_result;
}

void BatAdsClientMojoBridge::EventLog(const std::string& json) {
  if (!connected())
    return;

  bat_ads_client_->EventLog(json);
}

std::unique_ptr<ads::LogStream> BatAdsClientMojoBridge::Log(
    const char* file,
    int line,
    const ads::LogLevel log_level) const {
  // There's no need to proxy this
  return std::make_unique<LogStreamImpl>(file, line, log_level);
}

void BatAdsClientMojoBridge::SetIdleThreshold(const int threshold) {
  if (!connected())
    return;

  bat_ads_client_->SetIdleThreshold(threshold);
}

bool BatAdsClientMojoBridge::IsNotificationsAvailable() const {
  if (!connected())
    return false;

  bool available;
  bat_ads_client_->IsNotificationsAvailable(&available);
  return available;
}

void OnLoadUserModelForLocale(const ads::OnLoadCallback& callback,
            int32_t result,
            const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::LoadUserModelForLocale(
    const std::string& locale,
    ads::OnLoadCallback callback) const {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->LoadUserModelForLocale(locale,
      base::BindOnce(&OnLoadUserModelForLocale, std::move(callback)));
}

bool BatAdsClientMojoBridge::IsNetworkConnectionAvailable() {
  if (!connected())
    return false;

  bool available;
  bat_ads_client_->IsNetworkConnectionAvailable(&available);
  return available;
}

bool BatAdsClientMojoBridge::connected() const {
  return bat_ads_client_.is_bound();
}

}  // namespace bat_ads
