/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include <utility>

#include "base/time/time.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/notification_ad_value_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace bat_ads {

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info) {
  bat_ads_client_.Bind(std::move(client_info));
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() = default;

bool BatAdsClientMojoBridge::CanShowNotificationAdsWhileBrowserIsBackgrounded()
    const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool can_show = false;
  bat_ads_client_->CanShowNotificationAdsWhileBrowserIsBackgrounded(&can_show);
  return can_show;
}

bool BatAdsClientMojoBridge::IsNetworkConnectionAvailable() const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool is_available = false;
  bat_ads_client_->IsNetworkConnectionAvailable(&is_available);
  return is_available;
}

bool BatAdsClientMojoBridge::IsBrowserActive() const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool is_browser_active = false;
  bat_ads_client_->IsBrowserActive(&is_browser_active);
  return is_browser_active;
}

bool BatAdsClientMojoBridge::IsBrowserInFullScreenMode() const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool is_browser_in_full_screen_mode = false;
  bat_ads_client_->IsBrowserInFullScreenMode(&is_browser_in_full_screen_mode);
  return is_browser_in_full_screen_mode;
}

void BatAdsClientMojoBridge::ShowNotificationAd(
    const ads::NotificationAdInfo& ad) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ShowNotificationAd(ads::NotificationAdToValue(ad));
  }
}

bool BatAdsClientMojoBridge::CanShowNotificationAds() {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool can_show = false;
  bat_ads_client_->CanShowNotificationAds(&can_show);
  return can_show;
}

void BatAdsClientMojoBridge::CloseNotificationAd(
    const std::string& placement_id) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->CloseNotificationAd(placement_id);
  }
}

void BatAdsClientMojoBridge::RecordAdEventForId(
    const std::string& id,
    const std::string& ad_type,
    const std::string& confirmation_type,
    const base::Time time) const {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->RecordAdEventForId(id, ad_type, confirmation_type, time);
  }
}

std::vector<base::Time> BatAdsClientMojoBridge::GetAdEventHistory(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  if (!bat_ads_client_.is_bound()) {
    return {};
  }

  std::vector<base::Time> ad_event_history;
  bat_ads_client_->GetAdEventHistory(ad_type, confirmation_type,
                                     &ad_event_history);
  return ad_event_history;
}

void BatAdsClientMojoBridge::ResetAdEventHistoryForId(
    const std::string& id) const {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ResetAdEventHistoryForId(id);
  }
}

void OnUrlRequest(ads::UrlRequestCallback callback,
                  const ads::mojom::UrlResponseInfoPtr url_response_ptr) {
  ads::mojom::UrlResponseInfo url_response;

  if (!url_response_ptr) {
    url_response.status_code = -1;
    std::move(callback).Run(url_response);
    return;
  }

  url_response.url = url_response_ptr->url;
  url_response.status_code = url_response_ptr->status_code;
  url_response.body = url_response_ptr->body;
  url_response.headers = url_response_ptr->headers;
  std::move(callback).Run(url_response);
}

void BatAdsClientMojoBridge::UrlRequest(
    ads::mojom::UrlRequestInfoPtr url_request,
    ads::UrlRequestCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    ads::mojom::UrlResponseInfo response;
    response.url = url_request->url;
    response.status_code = -1;
    std::move(callback).Run(response);
    return;
  }

  bat_ads_client_->UrlRequest(
      std::move(url_request),
      base::BindOnce(&OnUrlRequest, std::move(callback)));
}

void BatAdsClientMojoBridge::Save(const std::string& name,
                                  const std::string& value,
                                  ads::SaveCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(/*success*/ false);
    return;
  }

  bat_ads_client_->Save(name, value, std::move(callback));
}

void BatAdsClientMojoBridge::LoadFileResource(const std::string& id,
                                              const int version,
                                              ads::LoadFileCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(base::File());
    return;
  }

  bat_ads_client_->LoadFileResource(id, version, std::move(callback));
}

void BatAdsClientMojoBridge::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    ads::GetBrowsingHistoryCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run({});
    return;
  }

  bat_ads_client_->GetBrowsingHistory(max_count, days_ago, std::move(callback));
}

void BatAdsClientMojoBridge::RecordP2AEvent(const std::string& name,
                                            base::Value::List value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->RecordP2AEvent(name, std::move(value));
  }
}

void BatAdsClientMojoBridge::LogTrainingInstance(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->LogTrainingInstance(std::move(training_instance));
  }
}

void BatAdsClientMojoBridge::Load(const std::string& name,
                                  ads::LoadCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(/*success*/ false, /*value*/ {});
    return;
  }

  bat_ads_client_->Load(name, std::move(callback));
}

std::string BatAdsClientMojoBridge::LoadDataResource(const std::string& name) {
  if (!bat_ads_client_.is_bound()) {
    return {};
  }

  std::string value;
  bat_ads_client_->LoadDataResource(name, &value);
  return value;
}

void BatAdsClientMojoBridge::RunDBTransaction(
    ads::mojom::DBTransactionInfoPtr transaction,
    ads::RunDBTransactionCallback callback) {
  bat_ads_client_->RunDBTransaction(std::move(transaction),
                                    std::move(callback));
}

void BatAdsClientMojoBridge::ClearScheduledCaptcha() {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ClearScheduledCaptcha();
  }
}

void BatAdsClientMojoBridge::GetScheduledCaptcha(
    const std::string& payment_id,
    ads::GetScheduledCaptchaCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    return std::move(callback).Run({});
  }

  bat_ads_client_->GetScheduledCaptcha(payment_id, std::move(callback));
}

void BatAdsClientMojoBridge::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id,
    const bool should_show_tooltip_notification) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ShowScheduledCaptchaNotification(
        payment_id, captcha_id, should_show_tooltip_notification);
  }
}

void BatAdsClientMojoBridge::UpdateAdRewards() {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->UpdateAdRewards();
  }
}

void BatAdsClientMojoBridge::Log(const char* file,
                                 const int line,
                                 const int verbose_level,
                                 const std::string& message) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->Log(file, line, verbose_level, message);
  }
}

bool BatAdsClientMojoBridge::GetBooleanPref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool value = false;
  bat_ads_client_->GetBooleanPref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetBooleanPref(const std::string& path,
                                            const bool value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetBooleanPref(path, value);
  }
}

int BatAdsClientMojoBridge::GetIntegerPref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return 0;
  }

  int value = 0;
  bat_ads_client_->GetIntegerPref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetIntegerPref(const std::string& path,
                                            const int value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetIntegerPref(path, value);
  }
}

double BatAdsClientMojoBridge::GetDoublePref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return 0.0;
  }

  double value = 0.0;
  bat_ads_client_->GetDoublePref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetDoublePref(const std::string& path,
                                           const double value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetDoublePref(path, value);
  }
}

std::string BatAdsClientMojoBridge::GetStringPref(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return {};
  }

  std::string value;
  bat_ads_client_->GetStringPref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetStringPref(const std::string& path,
                                           const std::string& value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetStringPref(path, value);
  }
}

int64_t BatAdsClientMojoBridge::GetInt64Pref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return 0;
  }

  int64_t value = 0;
  bat_ads_client_->GetInt64Pref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetInt64Pref(const std::string& path,
                                          const int64_t value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetInt64Pref(path, value);
  }
}

uint64_t BatAdsClientMojoBridge::GetUint64Pref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return 0;
  }

  uint64_t value = 0;
  bat_ads_client_->GetUint64Pref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetUint64Pref(const std::string& path,
                                           const uint64_t value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetUint64Pref(path, value);
  }
}

base::Time BatAdsClientMojoBridge::GetTimePref(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return {};
  }

  base::Time value;
  bat_ads_client_->GetTimePref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetTimePref(const std::string& path,
                                         const base::Time value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetTimePref(path, value);
  }
}

absl::optional<base::Value::Dict> BatAdsClientMojoBridge::GetDictPref(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return absl::nullopt;
  }

  absl::optional<base::Value::Dict> value;
  bat_ads_client_->GetDictPref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetDictPref(const std::string& path,
                                         base::Value::Dict value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetDictPref(path, std::move(value));
  }
}

absl::optional<base::Value::List> BatAdsClientMojoBridge::GetListPref(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return absl::nullopt;
  }

  absl::optional<base::Value::List> value;
  bat_ads_client_->GetListPref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetListPref(const std::string& path,
                                         base::Value::List value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetListPref(path, std::move(value));
  }
}

void BatAdsClientMojoBridge::ClearPref(const std::string& path) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ClearPref(path);
  }
}

bool BatAdsClientMojoBridge::HasPrefPath(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool value = false;
  bat_ads_client_->HasPrefPath(path, &value);
  return value;
}

}  // namespace bat_ads
