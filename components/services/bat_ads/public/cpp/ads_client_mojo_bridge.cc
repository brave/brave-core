/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/notification_ad_value_util.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep

namespace bat_ads {

AdsClientMojoBridge::AdsClientMojoBridge(ads::AdsClient* ads_client)
    : ads_client_(ads_client) {
  DCHECK(ads_client_);
}

AdsClientMojoBridge::~AdsClientMojoBridge() = default;

// static
void AdsClientMojoBridge::OnURLRequest(
    UrlRequestCallback callback,
    const ads::mojom::UrlResponseInfo& url_response) {
  std::move(callback).Run(ads::mojom::UrlResponseInfo::New(url_response));
}

bool AdsClientMojoBridge::IsNetworkConnectionAvailable(bool* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->IsNetworkConnectionAvailable();
  return true;
}

void AdsClientMojoBridge::IsNetworkConnectionAvailable(
    IsNetworkConnectionAvailableCallback callback) {
  std::move(callback).Run(ads_client_->IsNetworkConnectionAvailable());
}

bool AdsClientMojoBridge::IsBrowserActive(bool* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->IsBrowserActive();
  return true;
}

void AdsClientMojoBridge::IsBrowserActive(IsBrowserActiveCallback callback) {
  std::move(callback).Run(ads_client_->IsBrowserActive());
}

bool AdsClientMojoBridge::IsBrowserInFullScreenMode(bool* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->IsBrowserInFullScreenMode();
  return true;
}

void AdsClientMojoBridge::IsBrowserInFullScreenMode(
    IsBrowserInFullScreenModeCallback callback) {
  std::move(callback).Run(ads_client_->IsBrowserInFullScreenMode());
}

bool AdsClientMojoBridge::CanShowNotificationAds(bool* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->CanShowNotificationAds();
  return true;
}

void AdsClientMojoBridge::CanShowNotificationAds(
    CanShowNotificationAdsCallback callback) {
  std::move(callback).Run(ads_client_->CanShowNotificationAds());
}

bool AdsClientMojoBridge::CanShowNotificationAdsWhileBrowserIsBackgrounded(
    bool* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->CanShowNotificationAdsWhileBrowserIsBackgrounded();
  return true;
}

void AdsClientMojoBridge::CanShowNotificationAdsWhileBrowserIsBackgrounded(
    CanShowNotificationAdsWhileBrowserIsBackgroundedCallback callback) {
  std::move(callback).Run(
      ads_client_->CanShowNotificationAdsWhileBrowserIsBackgrounded());
}

void AdsClientMojoBridge::ShowNotificationAd(base::Value::Dict dict) {
  ads_client_->ShowNotificationAd(ads::NotificationAdFromValue(dict));
}

void AdsClientMojoBridge::CloseNotificationAd(const std::string& placement_id) {
  ads_client_->CloseNotificationAd(placement_id);
}

void AdsClientMojoBridge::UpdateAdRewards() {
  ads_client_->UpdateAdRewards();
}

void AdsClientMojoBridge::RecordAdEventForId(
    const std::string& id,
    const std::string& ad_type,
    const std::string& confirmation_type,
    const base::Time time) {
  ads_client_->RecordAdEventForId(id, ad_type, confirmation_type, time);
}

bool AdsClientMojoBridge::GetAdEventHistory(
    const std::string& ad_type,
    const std::string& confirmation_type,
    std::vector<base::Time>* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->GetAdEventHistory(ad_type, confirmation_type);
  return true;
}

void AdsClientMojoBridge::GetAdEventHistory(
    const std::string& ad_type,
    const std::string& confirmation_type,
    GetAdEventHistoryCallback callback) {
  std::move(callback).Run(
      ads_client_->GetAdEventHistory(ad_type, confirmation_type));
}

void AdsClientMojoBridge::ResetAdEventHistoryForId(const std::string& id) {
  ads_client_->ResetAdEventHistoryForId(id);
}

void AdsClientMojoBridge::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    GetBrowsingHistoryCallback callback) {
  ads_client_->GetBrowsingHistory(max_count, days_ago, std::move(callback));
}

void AdsClientMojoBridge::UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                                     UrlRequestCallback callback) {
  ads_client_->UrlRequest(
      std::move(url_request),
      base::BindOnce(&AdsClientMojoBridge::OnURLRequest, std::move(callback)));
}

void AdsClientMojoBridge::Save(const std::string& name,
                               const std::string& value,
                               SaveCallback callback) {
  ads_client_->Save(name, value, std::move(callback));
}

void AdsClientMojoBridge::Load(const std::string& name, LoadCallback callback) {
  ads_client_->Load(name, std::move(callback));
}

void AdsClientMojoBridge::LoadFileResource(const std::string& id,
                                           const int version,
                                           LoadFileResourceCallback callback) {
  ads_client_->LoadFileResource(id, version, std::move(callback));
}

bool AdsClientMojoBridge::LoadDataResource(const std::string& name,
                                           std::string* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->LoadDataResource(name);
  return true;
}

void AdsClientMojoBridge::LoadDataResource(const std::string& name,
                                           LoadDataResourceCallback callback) {
  std::move(callback).Run(ads_client_->LoadDataResource(name));
}

void AdsClientMojoBridge::GetScheduledCaptcha(
    const std::string& payment_id,
    GetScheduledCaptchaCallback callback) {
  ads_client_->GetScheduledCaptcha(payment_id, std::move(callback));
}

void AdsClientMojoBridge::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id,
    const bool should_show_tooltip_notification) {
  ads_client_->ShowScheduledCaptchaNotification(
      payment_id, captcha_id, should_show_tooltip_notification);
}

void AdsClientMojoBridge::ClearScheduledCaptcha() {
  ads_client_->ClearScheduledCaptcha();
}

void AdsClientMojoBridge::RunDBTransaction(
    ads::mojom::DBTransactionInfoPtr transaction,
    RunDBTransactionCallback callback) {
  ads_client_->RunDBTransaction(std::move(transaction), std::move(callback));
}

void AdsClientMojoBridge::RecordP2AEvent(const std::string& name,
                                         base::Value::List value) {
  ads_client_->RecordP2AEvent(name, std::move(value));
}

void AdsClientMojoBridge::LogTrainingInstance(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance) {
  ads_client_->LogTrainingInstance(std::move(training_instance));
}

void AdsClientMojoBridge::GetBooleanPref(const std::string& path,
                                         GetBooleanPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetBooleanPref(path));
}

void AdsClientMojoBridge::SetBooleanPref(const std::string& path,
                                         const bool value) {
  ads_client_->SetBooleanPref(path, value);
}

void AdsClientMojoBridge::GetIntegerPref(const std::string& path,
                                         GetIntegerPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetIntegerPref(path));
}

void AdsClientMojoBridge::SetIntegerPref(const std::string& path,
                                         const int value) {
  ads_client_->SetIntegerPref(path, value);
}

void AdsClientMojoBridge::GetDoublePref(const std::string& path,
                                        GetDoublePrefCallback callback) {
  std::move(callback).Run(ads_client_->GetDoublePref(path));
}

void AdsClientMojoBridge::SetDoublePref(const std::string& path,
                                        const double value) {
  ads_client_->SetDoublePref(path, value);
}

void AdsClientMojoBridge::GetStringPref(const std::string& path,
                                        GetStringPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetStringPref(path));
}

void AdsClientMojoBridge::SetStringPref(const std::string& path,
                                        const std::string& value) {
  ads_client_->SetStringPref(path, value);
}

void AdsClientMojoBridge::GetInt64Pref(const std::string& path,
                                       GetInt64PrefCallback callback) {
  std::move(callback).Run(ads_client_->GetInt64Pref(path));
}

void AdsClientMojoBridge::SetInt64Pref(const std::string& path,
                                       const int64_t value) {
  ads_client_->SetInt64Pref(path, value);
}

void AdsClientMojoBridge::GetUint64Pref(const std::string& path,
                                        GetUint64PrefCallback callback) {
  std::move(callback).Run(ads_client_->GetUint64Pref(path));
}

void AdsClientMojoBridge::SetUint64Pref(const std::string& path,
                                        const uint64_t value) {
  ads_client_->SetUint64Pref(path, value);
}

void AdsClientMojoBridge::GetTimePref(const std::string& path,
                                      GetTimePrefCallback callback) {
  std::move(callback).Run(ads_client_->GetTimePref(path));
}

void AdsClientMojoBridge::SetTimePref(const std::string& path,
                                      const base::Time value) {
  ads_client_->SetTimePref(path, value);
}

void AdsClientMojoBridge::GetDictPref(const std::string& path,
                                      GetDictPrefCallback callback) {
  absl::optional<base::Value::Dict> value = ads_client_->GetDictPref(path);
  std::move(callback).Run(std::move(value));
}

void AdsClientMojoBridge::SetDictPref(const std::string& path,
                                      base::Value::Dict value) {
  ads_client_->SetDictPref(path, std::move(value));
}

void AdsClientMojoBridge::GetListPref(const std::string& path,
                                      GetListPrefCallback callback) {
  absl::optional<base::Value::List> value = ads_client_->GetListPref(path);
  std::move(callback).Run(std::move(value));
}

void AdsClientMojoBridge::SetListPref(const std::string& path,
                                      base::Value::List value) {
  ads_client_->SetListPref(path, std::move(value));
}

void AdsClientMojoBridge::ClearPref(const std::string& path) {
  ads_client_->ClearPref(path);
}

void AdsClientMojoBridge::HasPrefPath(const std::string& path,
                                      HasPrefPathCallback callback) {
  std::move(callback).Run(ads_client_->HasPrefPath(path));
}

void AdsClientMojoBridge::Log(const std::string& file,
                              const int32_t line,
                              const int32_t verbose_level,
                              const std::string& message) {
  ads_client_->Log(file.c_str(), line, verbose_level, message);
}

}  // namespace bat_ads
