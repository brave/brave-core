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

namespace bat_ads {

AdsClientMojoBridge::AdsClientMojoBridge(
    ads::AdsClient* ads_client)
    : ads_client_(ads_client) {
  DCHECK(ads_client_);
}

AdsClientMojoBridge::~AdsClientMojoBridge() = default;

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

bool AdsClientMojoBridge::IsFullScreen(bool* out_is_full_screen) {
  DCHECK(out_is_full_screen);
  *out_is_full_screen = ads_client_->IsFullScreen();
  return true;
}

void AdsClientMojoBridge::IsFullScreen(IsFullScreenCallback callback) {
  std::move(callback).Run(ads_client_->IsFullScreen());
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

bool AdsClientMojoBridge::GetAdEvents(const std::string& ad_type,
                                      const std::string& confirmation_type,
                                      std::vector<uint64_t>* out_ad_events) {
  DCHECK(out_ad_events);
  *out_ad_events = ads_client_->GetAdEvents(ad_type, confirmation_type);
  return true;
}

void AdsClientMojoBridge::GetAdEvents(const std::string& ad_type,
                                      const std::string& confirmation_type,
                                      GetAdEventsCallback callback) {
  std::move(callback).Run(ads_client_->GetAdEvents(ad_type, confirmation_type));
}

bool AdsClientMojoBridge::LoadResourceForId(
    const std::string& id,
    std::string* out_value) {
  DCHECK(out_value);
  *out_value = ads_client_->LoadResourceForId(id);
  return true;
}

void AdsClientMojoBridge::LoadResourceForId(
    const std::string& id,
    LoadResourceForIdCallback callback) {
  std::move(callback).Run(ads_client_->LoadResourceForId(id));
}

void AdsClientMojoBridge::Log(
    const std::string& file,
    const int32_t line,
    const int32_t verbose_level,
    const std::string& message) {
  ads_client_->Log(file.c_str(), line, verbose_level, message);
}

// static
void AdsClientMojoBridge::OnLoadAdsResource(
    CallbackHolder<LoadCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run((int32_t)result, std::move(value));
  }

  delete holder;
}

void AdsClientMojoBridge::LoadAdsResource(const std::string& id,
                                          const int version,
                                          LoadCallback callback) {
  // this gets deleted in OnLoad
  auto* holder =
      new CallbackHolder<LoadCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->LoadAdsResource(
      id, version,
      std::bind(AdsClientMojoBridge::OnLoadAdsResource, holder, _1, _2));
}

// static
void AdsClientMojoBridge::OnGetBrowsingHistory(
    CallbackHolder<GetBrowsingHistoryCallback>* holder,
    const std::vector<std::string>& history) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(history));
  }

  delete holder;
}

void AdsClientMojoBridge::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    GetBrowsingHistoryCallback callback) {
  // this gets deleted in OnGetBrowsingHistory
  auto* holder = new CallbackHolder<GetBrowsingHistoryCallback>(
      AsWeakPtr(), std::move(callback));
  ads_client_->GetBrowsingHistory(
      max_count, days_ago,
      std::bind(AdsClientMojoBridge::OnGetBrowsingHistory, holder, _1));
}

void AdsClientMojoBridge::RecordP2AEvent(
    const std::string& name,
    const ads::P2AEventType type,
    const std::string& out_value) {
  ads_client_->RecordP2AEvent(name, type, out_value);
}

// static
void AdsClientMojoBridge::OnLoad(
    CallbackHolder<LoadCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run((int32_t)result, std::move(value));
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
    std::move(holder->get()).Run((int32_t)result);
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
void AdsClientMojoBridge::OnURLRequest(
    CallbackHolder<UrlRequestCallback>* holder,
    const ads::UrlResponse& url_response) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ads::UrlResponse::New(url_response));
  }

  delete holder;
}

void AdsClientMojoBridge::UrlRequest(
    ads::UrlRequestPtr url_request,
    UrlRequestCallback callback) {
  // this gets deleted in OnURLRequest
  auto* holder =
      new CallbackHolder<UrlRequestCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->UrlRequest(std::move(url_request),
      std::bind(AdsClientMojoBridge::OnURLRequest, holder, _1));
}

// static
void AdsClientMojoBridge::ShowNotification(
    const std::string& json) {
  ads::AdNotificationInfo ad_notification;
  if (ad_notification.FromJson(json) != ads::Result::SUCCESS) {
    return;
  }

  ads_client_->ShowNotification(ad_notification);
}

void AdsClientMojoBridge::CloseNotification(
    const std::string& uuid) {
  ads_client_->CloseNotification(uuid);
}

void AdsClientMojoBridge::RecordAdEvent(const std::string& ad_type,
                                        const std::string& confirmation_type,
                                        const uint64_t timestamp) {
  ads_client_->RecordAdEvent(ad_type, confirmation_type, timestamp);
}

// static
void AdsClientMojoBridge::OnRunDBTransaction(
    CallbackHolder<RunDBTransactionCallback>* holder,
    ads::DBCommandResponsePtr response) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(response));
  }
  delete holder;
}

void AdsClientMojoBridge::RunDBTransaction(
    ads::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  auto* holder = new CallbackHolder<RunDBTransactionCallback>(AsWeakPtr(),
      std::move(callback));
  ads_client_->RunDBTransaction(std::move(transaction),
      std::bind(AdsClientMojoBridge::OnRunDBTransaction, holder, _1));
}

void AdsClientMojoBridge::OnAdRewardsChanged() {
  ads_client_->OnAdRewardsChanged();
}

void AdsClientMojoBridge::GetBooleanPref(
    const std::string& path,
    GetBooleanPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetBooleanPref(path));
}

void AdsClientMojoBridge::SetBooleanPref(
    const std::string& path,
    const bool value) {
  ads_client_->SetBooleanPref(path, value);
}

void AdsClientMojoBridge::GetIntegerPref(
    const std::string& path,
    GetIntegerPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetIntegerPref(path));
}

void AdsClientMojoBridge::SetIntegerPref(
    const std::string& path,
    const int value) {
  ads_client_->SetIntegerPref(path, value);
}

void AdsClientMojoBridge::GetDoublePref(
    const std::string& path,
    GetDoublePrefCallback callback) {
  std::move(callback).Run(ads_client_->GetDoublePref(path));
}

void AdsClientMojoBridge::SetDoublePref(
    const std::string& path,
    const double value) {
  ads_client_->SetDoublePref(path, value);
}

void AdsClientMojoBridge::GetStringPref(
    const std::string& path,
    GetStringPrefCallback callback) {
  std::move(callback).Run(ads_client_->GetStringPref(path));
}

void AdsClientMojoBridge::SetStringPref(
    const std::string& path,
    const std::string& value) {
  ads_client_->SetStringPref(path, value);
}

void AdsClientMojoBridge::GetInt64Pref(
    const std::string& path,
    GetInt64PrefCallback callback) {
  std::move(callback).Run(ads_client_->GetInt64Pref(path));
}

void AdsClientMojoBridge::SetInt64Pref(
    const std::string& path,
    const int64_t value) {
  ads_client_->SetInt64Pref(path, value);
}

void AdsClientMojoBridge::GetUint64Pref(
    const std::string& path,
    GetUint64PrefCallback callback) {
  std::move(callback).Run(ads_client_->GetUint64Pref(path));
}

void AdsClientMojoBridge::SetUint64Pref(
    const std::string& path,
    const uint64_t value) {
  ads_client_->SetUint64Pref(path, value);
}

void AdsClientMojoBridge::ClearPref(
    const std::string& path) {
  ads_client_->ClearPref(path);
}

}  // namespace bat_ads
