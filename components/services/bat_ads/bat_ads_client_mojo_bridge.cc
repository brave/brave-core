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
#include "base/logging.h"

namespace bat_ads {

namespace {

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

bool BatAdsClientMojoBridge::ShouldAllowAdsSubdivisionTargeting() const {
  if (!connected()) {
    return false;
  }

  bool should_allow;
  bat_ads_client_->ShouldAllowAdsSubdivisionTargeting(&should_allow);
  return should_allow;
}

void BatAdsClientMojoBridge::SetAllowAdsSubdivisionTargeting(
    const bool should_allow) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetAllowAdsSubdivisionTargeting(should_allow);
}

std::string BatAdsClientMojoBridge::GetAdsSubdivisionTargetingCode() const {
  std::string subdivision_targeting_code;

  if (!connected()) {
    return subdivision_targeting_code;
  }

  bat_ads_client_->GetAdsSubdivisionTargetingCode(&subdivision_targeting_code);
  return subdivision_targeting_code;
}

void BatAdsClientMojoBridge::SetAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetAdsSubdivisionTargetingCode(subdivision_targeting_code);
}

std::string BatAdsClientMojoBridge::
GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const {
  std::string subdivision_targeting_code;

  if (!connected()) {
    return subdivision_targeting_code;
  }

  bat_ads_client_->GetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      &subdivision_targeting_code);
  return subdivision_targeting_code;
}

void BatAdsClientMojoBridge::
SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  if (!connected()) {
    return;
  }

  bat_ads_client_->SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      subdivision_targeting_code);
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

void OnUrlRequest(
    const ads::UrlRequestCallback& callback,
    const ads::UrlResponsePtr response_ptr) {
  ads::UrlResponse response;

  if (!response_ptr) {
    response.status_code = 418;  // I'm a teapot
    callback(response);
    return;
  }

  response.url = response_ptr->url;
  response.status_code = response_ptr->status_code;
  response.body = response_ptr->body;
  response.headers = response_ptr->headers;
  callback(response);
}

void BatAdsClientMojoBridge::UrlRequest(
    ads::UrlRequestPtr url_request,
    ads::UrlRequestCallback callback) {
  if (!connected()) {
    ads::UrlResponse response;
    response.url = url_request->url;
    response.status_code = 418;  // I'm a teapot
    callback(response);
    return;
  }

  bat_ads_client_->UrlRequest(std::move(url_request),
      base::BindOnce(&OnUrlRequest, std::move(callback)));
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

void OnLoadUserModelForId(
    const ads::LoadCallback& callback,
    const int32_t result,
    const std::string& value) {
  callback(ToAdsResult(result), value);
}

void BatAdsClientMojoBridge::LoadUserModelForId(
    const std::string& id,
    ads::LoadCallback callback) {
  if (!connected()) {
    callback(ads::Result::FAILED, "");
    return;
  }

  bat_ads_client_->LoadUserModelForId(id,
      base::BindOnce(&OnLoadUserModelForId, std::move(callback)));
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

std::string BatAdsClientMojoBridge::LoadResourceForId(
    const std::string& id) {
  std::string value;

  if (!connected()) {
    return value;
  }

  bat_ads_client_->LoadResourceForId(id, &value);
  return value;
}

void OnRunDBTransaction(
    const ads::RunDBTransactionCallback& callback,
    ads::DBCommandResponsePtr response) {
  callback(std::move(response));
}

void BatAdsClientMojoBridge::RunDBTransaction(
    ads::DBTransactionPtr transaction,
    ads::RunDBTransactionCallback callback) {
  bat_ads_client_->RunDBTransaction(std::move(transaction),
      base::BindOnce(&OnRunDBTransaction, std::move(callback)));
}

void BatAdsClientMojoBridge::OnAdRewardsChanged() {
  if (!connected()) {
    return;
  }

  bat_ads_client_->OnAdRewardsChanged();
}

void BatAdsClientMojoBridge::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
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
