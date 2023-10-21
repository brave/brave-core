/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace bat_ads {

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info,
    mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
    : notifier_impl_(std::move(client_notifier)) {
  bat_ads_client_.Bind(std::move(client_info));
  bat_ads_client_.reset_on_disconnect();
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() = default;

void BatAdsClientMojoBridge::AddObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  notifier_impl_.AddObserver(observer);
}

void BatAdsClientMojoBridge::RemoveObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  notifier_impl_.RemoveObserver(observer);
}

void BatAdsClientMojoBridge::NotifyPendingObservers() {
  notifier_impl_.BindReceiver();
}

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
    const brave_ads::NotificationAdInfo& ad) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ShowNotificationAd(brave_ads::NotificationAdToValue(ad));
  }
}

void BatAdsClientMojoBridge::ShowReminder(
    const brave_ads::mojom::ReminderType type) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ShowReminder(type);
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

void BatAdsClientMojoBridge::CacheAdEventForInstanceId(
    const std::string& id,
    const std::string& ad_type,
    const std::string& confirmation_type,
    const base::Time time) const {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->CacheAdEventForInstanceId(id, ad_type, confirmation_type,
                                               time);
  }
}

std::vector<base::Time> BatAdsClientMojoBridge::GetCachedAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  if (!bat_ads_client_.is_bound()) {
    return {};
  }

  std::vector<base::Time> ad_event_cache;
  bat_ads_client_->GetCachedAdEvents(ad_type, confirmation_type,
                                     &ad_event_cache);
  return ad_event_cache;
}

void BatAdsClientMojoBridge::ResetAdEventCacheForInstanceId(
    const std::string& id) const {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ResetAdEventCacheForInstanceId(id);
  }
}

void OnUrlRequest(brave_ads::UrlRequestCallback callback,
                  const brave_ads::mojom::UrlResponseInfoPtr url_response_ptr) {
  brave_ads::mojom::UrlResponseInfo url_response;

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
    brave_ads::mojom::UrlRequestInfoPtr url_request,
    brave_ads::UrlRequestCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    brave_ads::mojom::UrlResponseInfo response;
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
                                  brave_ads::SaveCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(/*success*/ false);
    return;
  }

  bat_ads_client_->Save(name, value, std::move(callback));
}

void BatAdsClientMojoBridge::LoadFileResource(
    const std::string& id,
    const int version,
    brave_ads::LoadFileCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(base::File());
    return;
  }

  bat_ads_client_->LoadFileResource(id, version, std::move(callback));
}

void BatAdsClientMojoBridge::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    brave_ads::GetBrowsingHistoryCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run({});
    return;
  }

  bat_ads_client_->GetBrowsingHistory(max_count, days_ago, std::move(callback));
}

void BatAdsClientMojoBridge::RecordP2AEvents(
    const std::vector<std::string>& events) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->RecordP2AEvents(events);
  }
}

void BatAdsClientMojoBridge::AddFederatedLearningPredictorTrainingSample(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->AddFederatedLearningPredictorTrainingSample(
        std::move(training_sample));
  }
}

void BatAdsClientMojoBridge::Load(const std::string& name,
                                  brave_ads::LoadCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(/*value*/ absl::nullopt);
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
    brave_ads::mojom::DBTransactionInfoPtr transaction,
    brave_ads::RunDBTransactionCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    std::move(callback).Run(brave_ads::mojom::DBCommandResponseInfoPtr());
    return;
  }

  bat_ads_client_->RunDBTransaction(std::move(transaction),
                                    std::move(callback));
}

void BatAdsClientMojoBridge::GetScheduledCaptcha(
    const std::string& payment_id,
    brave_ads::GetScheduledCaptchaCallback callback) {
  if (!bat_ads_client_.is_bound()) {
    return std::move(callback).Run({});
  }

  bat_ads_client_->GetScheduledCaptcha(payment_id, std::move(callback));
}

void BatAdsClientMojoBridge::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ShowScheduledCaptchaNotification(payment_id, captcha_id);
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

absl::optional<base::Value> BatAdsClientMojoBridge::GetProfilePref(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return absl::nullopt;
  }

  absl::optional<base::Value> value;
  bat_ads_client_->GetProfilePref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetProfilePref(const std::string& path,
                                            base::Value value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetProfilePref(path, std::move(value));
  }
}

void BatAdsClientMojoBridge::ClearProfilePref(const std::string& path) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ClearProfilePref(path);
  }
}

bool BatAdsClientMojoBridge::HasProfilePrefPath(const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool value = false;
  bat_ads_client_->HasProfilePrefPath(path, &value);
  return value;
}

absl::optional<base::Value> BatAdsClientMojoBridge::GetLocalStatePref(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return absl::nullopt;
  }

  absl::optional<base::Value> value;
  bat_ads_client_->GetLocalStatePref(path, &value);
  return value;
}

void BatAdsClientMojoBridge::SetLocalStatePref(const std::string& path,
                                               base::Value value) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->SetLocalStatePref(path, std::move(value));
  }
}

void BatAdsClientMojoBridge::ClearLocalStatePref(const std::string& path) {
  if (bat_ads_client_.is_bound()) {
    bat_ads_client_->ClearLocalStatePref(path);
  }
}

bool BatAdsClientMojoBridge::HasLocalStatePrefPath(
    const std::string& path) const {
  if (!bat_ads_client_.is_bound()) {
    return false;
  }

  bool value = false;
  bat_ads_client_->HasLocalStatePrefPath(path, &value);
  return value;
}

}  // namespace bat_ads
