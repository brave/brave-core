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

bool AdsClientMojoBridge::ShouldAllowAdsSubdivisionTargeting(
    bool* out_should_allow) {
  DCHECK(out_should_allow);
  *out_should_allow = ads_client_->ShouldAllowAdsSubdivisionTargeting();
  return true;
}

void AdsClientMojoBridge::ShouldAllowAdsSubdivisionTargeting(
    ShouldAllowAdsSubdivisionTargetingCallback callback) {
  std::move(callback).Run(ads_client_->ShouldAllowAdsSubdivisionTargeting());
}

void AdsClientMojoBridge::SetAllowAdsSubdivisionTargeting(
    const bool should_allow) {
  ads_client_->SetAllowAdsSubdivisionTargeting(should_allow);
}

bool AdsClientMojoBridge::GetAdsSubdivisionTargetingCode(
    std::string* out_subdivision_targeting_code) {
  DCHECK(out_subdivision_targeting_code);
  *out_subdivision_targeting_code =
      ads_client_->GetAdsSubdivisionTargetingCode();
  return true;
}

void AdsClientMojoBridge::GetAdsSubdivisionTargetingCode(
    GetAdsSubdivisionTargetingCodeCallback callback) {
  std::move(callback).Run(ads_client_->GetAdsSubdivisionTargetingCode());
}

void AdsClientMojoBridge::SetAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  ads_client_->SetAdsSubdivisionTargetingCode(subdivision_targeting_code);
}

bool AdsClientMojoBridge::GetAutomaticallyDetectedAdsSubdivisionTargetingCode(
    std::string* out_subdivision_targeting_code) {
  DCHECK(out_subdivision_targeting_code);
  *out_subdivision_targeting_code =
      ads_client_->GetAutomaticallyDetectedAdsSubdivisionTargetingCode();
  return true;
}

void AdsClientMojoBridge::GetAutomaticallyDetectedAdsSubdivisionTargetingCode(
    GetAutomaticallyDetectedAdsSubdivisionTargetingCodeCallback callback) {
  std::move(callback).Run(
      ads_client_->GetAutomaticallyDetectedAdsSubdivisionTargetingCode());
}

void AdsClientMojoBridge::SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  ads_client_->SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      subdivision_targeting_code);
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

void AdsClientMojoBridge::SetIdleThreshold(
    const int32_t threshold) {
  ads_client_->SetIdleThreshold(threshold);
}

void AdsClientMojoBridge::Log(
    const std::string& file,
    const int32_t line,
    const int32_t verbose_level,
    const std::string& message) {
  ads_client_->Log(file.c_str(), line, verbose_level, message);
}

// static
void AdsClientMojoBridge::OnLoadUserModelForId(
    CallbackHolder<LoadCallback>* holder,
    const ads::Result result,
    const std::string& value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run((int32_t)result, std::move(value));
  }

  delete holder;
}

void AdsClientMojoBridge::LoadUserModelForId(
    const std::string& id,
    LoadCallback callback) {
  // this gets deleted in OnLoad
  auto* holder =
      new CallbackHolder<LoadCallback>(AsWeakPtr(), std::move(callback));
  ads_client_->LoadUserModelForId(
      id, std::bind(AdsClientMojoBridge::OnLoadUserModelForId, holder, _1, _2));
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
    const ads::UrlResponse& response) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(ads::UrlResponse::New(response));
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

}  // namespace bat_ads
