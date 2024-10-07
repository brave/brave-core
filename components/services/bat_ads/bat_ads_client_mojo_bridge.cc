/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace bat_ads {

BatAdsClientMojoBridge::BatAdsClientMojoBridge(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient>
        bat_ads_client_pending_associated_remote,
    mojo::PendingReceiver<mojom::BatAdsClientNotifier>
        bat_ads_client_notifier_pending_receiver)
    : bat_ads_client_notifier_impl_(
          std::move(bat_ads_client_notifier_pending_receiver)) {
  bat_ads_client_associated_remote_.Bind(
      std::move(bat_ads_client_pending_associated_remote));
  bat_ads_client_associated_remote_.reset_on_disconnect();
}

BatAdsClientMojoBridge::~BatAdsClientMojoBridge() = default;

void BatAdsClientMojoBridge::AddObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  bat_ads_client_notifier_impl_.AddObserver(observer);
}

void BatAdsClientMojoBridge::RemoveObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  bat_ads_client_notifier_impl_.RemoveObserver(observer);
}

void BatAdsClientMojoBridge::NotifyPendingObservers() {
  bat_ads_client_notifier_impl_.BindReceiver();
}

bool BatAdsClientMojoBridge::CanShowNotificationAdsWhileBrowserIsBackgrounded()
    const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return false;
  }

  bool can_show = false;
  bat_ads_client_associated_remote_
      ->CanShowNotificationAdsWhileBrowserIsBackgrounded(&can_show);
  return can_show;
}

bool BatAdsClientMojoBridge::IsNetworkConnectionAvailable() const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return false;
  }

  bool is_available = false;
  bat_ads_client_associated_remote_->IsNetworkConnectionAvailable(
      &is_available);
  return is_available;
}

bool BatAdsClientMojoBridge::IsBrowserActive() const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return false;
  }

  bool is_browser_active = false;
  bat_ads_client_associated_remote_->IsBrowserActive(&is_browser_active);
  return is_browser_active;
}

bool BatAdsClientMojoBridge::IsBrowserInFullScreenMode() const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return false;
  }

  bool is_browser_in_full_screen_mode = false;
  bat_ads_client_associated_remote_->IsBrowserInFullScreenMode(
      &is_browser_in_full_screen_mode);
  return is_browser_in_full_screen_mode;
}

void BatAdsClientMojoBridge::ShowNotificationAd(
    const brave_ads::NotificationAdInfo& ad) {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->ShowNotificationAd(
        brave_ads::NotificationAdToValue(ad));
  }
}

bool BatAdsClientMojoBridge::CanShowNotificationAds() const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return false;
  }

  bool can_show = false;
  bat_ads_client_associated_remote_->CanShowNotificationAds(&can_show);
  return can_show;
}

void BatAdsClientMojoBridge::CloseNotificationAd(
    const std::string& placement_id) {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->CloseNotificationAd(placement_id);
  }
}

void BatAdsClientMojoBridge::CacheAdEventForInstanceId(
    const std::string& id,
    const brave_ads::mojom::AdType mojom_ad_type,
    const brave_ads::mojom::ConfirmationType mojom_confirmation_type,
    const base::Time time) const {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->CacheAdEventForInstanceId(
        id, mojom_ad_type, mojom_confirmation_type, time);
  }
}

std::vector<base::Time> BatAdsClientMojoBridge::GetCachedAdEvents(
    const brave_ads::mojom::AdType mojom_ad_type,
    const brave_ads::mojom::ConfirmationType mojom_confirmation_type) const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return {};
  }

  std::vector<base::Time> ad_event_cache;
  bat_ads_client_associated_remote_->GetCachedAdEvents(
      mojom_ad_type, mojom_confirmation_type, &ad_event_cache);
  return ad_event_cache;
}

void BatAdsClientMojoBridge::ResetAdEventCacheForInstanceId(
    const std::string& id) const {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->ResetAdEventCacheForInstanceId(id);
  }
}

void OnUrlRequest(
    brave_ads::UrlRequestCallback callback,
    const brave_ads::mojom::UrlResponseInfoPtr mojom_url_response_ptr) {
  brave_ads::mojom::UrlResponseInfo mojom_url_response;

  if (!mojom_url_response_ptr) {
    mojom_url_response.status_code = -1;
    std::move(callback).Run(mojom_url_response);
    return;
  }

  mojom_url_response.url = mojom_url_response_ptr->url;
  mojom_url_response.status_code = mojom_url_response_ptr->status_code;
  mojom_url_response.body = mojom_url_response_ptr->body;
  mojom_url_response.headers = mojom_url_response_ptr->headers;
  std::move(callback).Run(mojom_url_response);
}

void BatAdsClientMojoBridge::UrlRequest(
    brave_ads::mojom::UrlRequestInfoPtr mojom_url_request,
    brave_ads::UrlRequestCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    brave_ads::mojom::UrlResponseInfo mojom_url_response;
    mojom_url_response.url = mojom_url_request->url;
    mojom_url_response.status_code = -1;
    std::move(callback).Run(mojom_url_response);
    return;
  }

  bat_ads_client_associated_remote_->UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&OnUrlRequest, std::move(callback)));
}

void BatAdsClientMojoBridge::Save(const std::string& name,
                                  const std::string& value,
                                  brave_ads::SaveCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    std::move(callback).Run(/*success*/ false);
    return;
  }

  bat_ads_client_associated_remote_->Save(name, value, std::move(callback));
}

void BatAdsClientMojoBridge::LoadResourceComponent(
    const std::string& id,
    const int version,
    brave_ads::LoadFileCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    std::move(callback).Run(base::File());
    return;
  }

  bat_ads_client_associated_remote_->LoadResourceComponent(id, version,
                                                           std::move(callback));
}

void BatAdsClientMojoBridge::GetSiteHistory(
    const int max_count,
    const int days_ago,
    brave_ads::GetSiteHistoryCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    std::move(callback).Run({});
    return;
  }

  bat_ads_client_associated_remote_->GetSiteHistory(max_count, days_ago,
                                                    std::move(callback));
}

void BatAdsClientMojoBridge::RecordP2AEvents(
    const std::vector<std::string>& events) {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->RecordP2AEvents(events);
  }
}

void BatAdsClientMojoBridge::Load(const std::string& name,
                                  brave_ads::LoadCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    std::move(callback).Run(/*value*/ std::nullopt);
    return;
  }

  bat_ads_client_associated_remote_->Load(name, std::move(callback));
}

std::string BatAdsClientMojoBridge::LoadDataResource(const std::string& name) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return {};
  }

  std::string value;
  bat_ads_client_associated_remote_->LoadDataResource(name, &value);
  return value;
}

void BatAdsClientMojoBridge::RunDBTransaction(
    brave_ads::mojom::DBTransactionInfoPtr mojom_db_transaction,
    brave_ads::RunDBTransactionCallback callback) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    std::move(callback).Run(brave_ads::mojom::DBTransactionResultInfoPtr());
    return;
  }

  bat_ads_client_associated_remote_->RunDBTransaction(
      std::move(mojom_db_transaction), std::move(callback));
}

void BatAdsClientMojoBridge::ShowScheduledCaptcha(
    const std::string& payment_id,
    const std::string& captcha_id) {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->ShowScheduledCaptcha(payment_id,
                                                            captcha_id);
  }
}

void BatAdsClientMojoBridge::Log(const char* file,
                                 const int line,
                                 const int verbose_level,
                                 const std::string& message) {
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->Log(file, line, verbose_level, message);
  }
}

bool BatAdsClientMojoBridge::FindProfilePref(const std::string& path) const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return cached_profile_prefs_.contains(path);
  }

  bool value;
  if (!bat_ads_client_associated_remote_->FindProfilePref(path, &value)) {
    return cached_profile_prefs_.contains(path);
  }

  return value;
}

std::optional<base::Value> BatAdsClientMojoBridge::GetProfilePref(
    const std::string& path) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return CachedProfilePrefValue(path);
  }

  std::optional<base::Value> value;
  if (!bat_ads_client_associated_remote_->GetProfilePref(path, &value)) {
    return CachedProfilePrefValue(path);
  }

  if (value) {
    cached_profile_prefs_[path] = value->Clone();
  } else {
    cached_profile_prefs_.erase(path);
  }

  return value;
}

void BatAdsClientMojoBridge::SetProfilePref(const std::string& path,
                                            base::Value value) {
  cached_profile_prefs_[path] = value.Clone();
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->SetProfilePref(path, std::move(value));
  }
}

void BatAdsClientMojoBridge::ClearProfilePref(const std::string& path) {
  cached_profile_prefs_.erase(path);
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->ClearProfilePref(path);
  }
}

bool BatAdsClientMojoBridge::HasProfilePrefPath(const std::string& path) const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return cached_profile_prefs_.contains(path);
  }

  bool value = false;
  if (!bat_ads_client_associated_remote_->HasProfilePrefPath(path, &value)) {
    return cached_profile_prefs_.contains(path);
  }

  return value;
}

bool BatAdsClientMojoBridge::FindLocalStatePref(const std::string& path) const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return cached_local_state_prefs_.contains(path);
  }

  bool value;
  if (!bat_ads_client_associated_remote_->FindLocalStatePref(path, &value)) {
    return cached_local_state_prefs_.contains(path);
  }

  return value;
}

std::optional<base::Value> BatAdsClientMojoBridge::GetLocalStatePref(
    const std::string& path) {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return CachedLocalStatePrefValue(path);
  }

  std::optional<base::Value> value;
  if (!bat_ads_client_associated_remote_->GetLocalStatePref(path, &value)) {
    return CachedLocalStatePrefValue(path);
  }

  if (value) {
    cached_local_state_prefs_[path] = value->Clone();
  } else {
    cached_local_state_prefs_.erase(path);
  }

  return value;
}

void BatAdsClientMojoBridge::SetLocalStatePref(const std::string& path,
                                               base::Value value) {
  cached_local_state_prefs_[path] = value.Clone();
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->SetLocalStatePref(path,
                                                         std::move(value));
  }
}

void BatAdsClientMojoBridge::ClearLocalStatePref(const std::string& path) {
  cached_local_state_prefs_.erase(path);
  if (bat_ads_client_associated_remote_.is_bound()) {
    bat_ads_client_associated_remote_->ClearLocalStatePref(path);
  }
}

bool BatAdsClientMojoBridge::HasLocalStatePrefPath(
    const std::string& path) const {
  if (!bat_ads_client_associated_remote_.is_bound()) {
    return cached_local_state_prefs_.contains(path);
  }

  bool value = false;
  if (!bat_ads_client_associated_remote_->HasLocalStatePrefPath(path, &value)) {
    return cached_local_state_prefs_.contains(path);
  }

  return value;
}

std::optional<base::Value> BatAdsClientMojoBridge::CachedProfilePrefValue(
    const std::string& path) const {
  if (!cached_profile_prefs_.contains(path)) {
    return std::nullopt;
  }

  return cached_profile_prefs_.at(path).Clone();
}

std::optional<base::Value> BatAdsClientMojoBridge::CachedLocalStatePrefValue(
    const std::string& path) const {
  if (!cached_local_state_prefs_.contains(path)) {
    return std::nullopt;
  }

  return cached_local_state_prefs_.at(path).Clone();
}

}  // namespace bat_ads
