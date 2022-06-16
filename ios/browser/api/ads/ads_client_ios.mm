/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ads_client_ios.h"
#import "ads_client_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Constructor & Destructor
AdsClientIOS::AdsClientIOS(id<AdsClientBridge> bridge) : bridge_(bridge) {}

AdsClientIOS::~AdsClientIOS() {
  bridge_ = nil;
}

bool AdsClientIOS::IsNetworkConnectionAvailable() const {
  return [bridge_ isNetworkConnectionAvailable];
}

bool AdsClientIOS::IsBrowserActive() const {
  return [bridge_ isBrowserActive];
}

bool AdsClientIOS::IsBrowserInFullScreenMode() const {
  return [bridge_ isBrowserInFullScreenMode];
}

bool AdsClientIOS::CanShowBackgroundNotifications() const {
  return [bridge_ canShowBackgroundNotifications];
}

void AdsClientIOS::ShowNotification(const ads::NotificationAdInfo& info) {
  [bridge_ showNotification:info];
}

bool AdsClientIOS::ShouldShowNotifications() {
  return [bridge_ shouldShowNotifications];
}

void AdsClientIOS::CloseNotification(const std::string& uuid) {
  [bridge_ closeNotification:uuid];
}

void AdsClientIOS::RecordAdEventForId(const std::string& id,
                                      const std::string& ad_type,
                                      const std::string& confirmation_type,
                                      const base::Time time) const {
  [bridge_ recordAdEventForId:id
                       adType:ad_type
             confirmationType:confirmation_type
                         time:time];
}

std::vector<base::Time> AdsClientIOS::GetAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  return [bridge_ getAdEvents:ad_type confirmationType:confirmation_type];
}

void AdsClientIOS::ResetAdEventsForId(const std::string& id) const {
  [bridge_ resetAdEventsForId:id];
}

void AdsClientIOS::UrlRequest(ads::mojom::UrlRequestPtr url_request,
                              ads::UrlRequestCallback callback) {
  [bridge_ UrlRequest:std::move(url_request) callback:callback];
}

void AdsClientIOS::Save(const std::string& name,
                        const std::string& value,
                        ads::ResultCallback callback) {
  [bridge_ save:name value:value callback:callback];
}

void AdsClientIOS::LoadFileResource(const std::string& id,
                                    const int version,
                                    ads::LoadFileCallback callback) {
  [bridge_ loadFileResource:id version:version callback:std::move(callback)];
}

void AdsClientIOS::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    ads::GetBrowsingHistoryCallback callback) {
  [bridge_ getBrowsingHistory:max_count forDays:days_ago callback:callback];
}

void AdsClientIOS::Load(const std::string& name, ads::LoadCallback callback) {
  [bridge_ load:name callback:callback];
}

std::string AdsClientIOS::LoadDataResource(const std::string& name) {
  return [bridge_ loadDataResource:name];
}

void AdsClientIOS::ClearScheduledCaptcha() {
  [bridge_ clearScheduledCaptcha];
}

void AdsClientIOS::GetScheduledCaptcha(
    const std::string& payment_id,
    ads::GetScheduledCaptchaCallback callback) {
  [bridge_ getScheduledCaptcha:payment_id callback:std::move(callback)];
}

void AdsClientIOS::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id) {
  [bridge_ showScheduledCaptchaNotification:payment_id captchaId:captcha_id];
}

void AdsClientIOS::Log(const char* file,
                       const int line,
                       const int verbose_level,
                       const std::string& message) {
  [bridge_ log:file line:line verboseLevel:verbose_level message:message];
}

void AdsClientIOS::RunDBTransaction(ads::mojom::DBTransactionPtr transaction,
                                    ads::RunDBTransactionCallback callback) {
  [bridge_ runDBTransaction:std::move(transaction) callback:callback];
}

void AdsClientIOS::OnAdRewardsChanged() {
  [bridge_ onAdRewardsChanged];
}

void AdsClientIOS::SetBooleanPref(const std::string& path, const bool value) {
  [bridge_ setBooleanPref:path value:value];
}

bool AdsClientIOS::GetBooleanPref(const std::string& path) const {
  return [bridge_ getBooleanPref:path];
}

void AdsClientIOS::SetIntegerPref(const std::string& path, const int value) {
  [bridge_ setIntegerPref:path value:value];
}

int AdsClientIOS::GetIntegerPref(const std::string& path) const {
  return [bridge_ getIntegerPref:path];
}

void AdsClientIOS::SetDoublePref(const std::string& path, const double value) {
  [bridge_ setDoublePref:path value:value];
}

double AdsClientIOS::GetDoublePref(const std::string& path) const {
  return [bridge_ getDoublePref:path];
}

void AdsClientIOS::SetStringPref(const std::string& path,
                                 const std::string& value) {
  [bridge_ setStringPref:path value:value];
}

std::string AdsClientIOS::GetStringPref(const std::string& path) const {
  return [bridge_ getStringPref:path];
}

void AdsClientIOS::SetInt64Pref(const std::string& path, const int64_t value) {
  [bridge_ setInt64Pref:path value:value];
}

int64_t AdsClientIOS::GetInt64Pref(const std::string& path) const {
  return [bridge_ getInt64Pref:path];
}

void AdsClientIOS::SetUint64Pref(const std::string& path,
                                 const uint64_t value) {
  [bridge_ setUint64Pref:path value:value];
}

uint64_t AdsClientIOS::GetUint64Pref(const std::string& path) const {
  return [bridge_ getUint64Pref:path];
}

void AdsClientIOS::SetTimePref(const std::string& path,
                               const base::Time value) {
  [bridge_ setTimePref:path value:value];
}

base::Time AdsClientIOS::GetTimePref(const std::string& path) const {
  return [bridge_ getTimePref:path];
}

void AdsClientIOS::ClearPref(const std::string& path) {
  [bridge_ clearPref:path];
}

bool AdsClientIOS::HasPrefPath(const std::string& path) const {
  return [bridge_ hasPrefPath:path];
}

void AdsClientIOS::RecordP2AEvent(const std::string& name,
                                  const ads::mojom::P2AEventType type,
                                  const std::string& value) {
  [bridge_ recordP2AEvent:name type:type value:value];
}

void AdsClientIOS::LogTrainingInstance(
    brave_federated::mojom::TrainingInstancePtr training_instance) {
  [bridge_ logTrainingInstance:std::move(training_instance)];
}
