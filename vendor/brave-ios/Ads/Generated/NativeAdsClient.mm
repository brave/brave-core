/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "NativeAdsClient.h"
#import "NativeAdsClientBridge.h"

// Constructor & Destructor
NativeAdsClient::NativeAdsClient(id<NativeAdsClientBridge> bridge) : bridge_(bridge) {
}

NativeAdsClient::~NativeAdsClient() {
  bridge_ = nil;
}

bool NativeAdsClient::IsEnabled() const {
  return [bridge_ isAdsEnabled];
}

bool NativeAdsClient::ShouldAllowAdConversionTracking() const {
  return [bridge_ shouldAllowAdConversionTracking];
}

uint64_t NativeAdsClient::GetAdsPerDay() const {
  return [bridge_ getAdsPerDay];
}

uint64_t NativeAdsClient::GetAdsPerHour() const {
  return [bridge_ getAdsPerHour];
}

bool NativeAdsClient::IsNetworkConnectionAvailable() const {
  return [bridge_ isNetworkConnectionAvailable];
}

void NativeAdsClient::SetIdleThreshold(const int threshold) {
  [bridge_ setIdleThreshold:threshold];
}

bool NativeAdsClient::IsForeground() const {
  return [bridge_ isForeground];
}

bool NativeAdsClient::CanShowBackgroundNotifications() const {
    return [bridge_ canShowBackgroundNotifications];
}

void NativeAdsClient::ShowNotification(std::unique_ptr<ads::AdNotificationInfo> info) {
  [bridge_ showNotification:std::move(info)];
}

bool NativeAdsClient::ShouldShowNotifications() {
  return [bridge_ shouldShowNotifications];
}

void NativeAdsClient::CloseNotification(const std::string & uuid) {
  [bridge_ closeNotification:uuid];
}

void NativeAdsClient::UrlRequest(ads::UrlRequestPtr url_request, ads::UrlRequestCallback callback) {
  [bridge_ UrlRequest:std::move(url_request) callback:callback];
}

void NativeAdsClient::Save(const std::string & name, const std::string & value, ads::ResultCallback callback) {
  [bridge_ save:name value:value callback:callback];
}

void NativeAdsClient::LoadUserModelForId(const std::string & id, ads::LoadCallback callback) {
  [bridge_ loadUserModelForId:id callback:callback];
}

void NativeAdsClient::Load(const std::string & name, ads::LoadCallback callback) {
  [bridge_ load:name callback:callback];
}

std::string NativeAdsClient::LoadResourceForId(const std::string & id) {
  return [bridge_ loadResourceForId:id];
}

void NativeAdsClient::Log(const char * file, const int line, const int verbose_level, const std::string & message) {
  [bridge_ log:file line:line verboseLevel:verbose_level message:message];
}

bool NativeAdsClient::ShouldAllowAdsSubdivisionTargeting() const {
  return [bridge_ shouldAllowAdsSubdivisionTargeting];
}

void NativeAdsClient::SetAllowAdsSubdivisionTargeting(const bool should_allow) {
  [bridge_ setAllowAdsSubdivisionTargeting:should_allow];
}

std::string NativeAdsClient::GetAdsSubdivisionTargetingCode() const {
  return [bridge_ adsSubdivisionTargetingCode];
}

void NativeAdsClient::SetAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) {
  [bridge_ setAdsSubdivisionTargetingCode:subdivision_targeting_code];
}

std::string NativeAdsClient::GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const {
  return [bridge_ automaticallyDetectedAdsSubdivisionTargetingCode];
}

void NativeAdsClient::SetAutomaticallyDetectedAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) {
  [bridge_ setAutomaticallyDetectedAdsSubdivisionTargetingCode:subdivision_targeting_code];
}

void NativeAdsClient::RunDBTransaction(ads::DBTransactionPtr transaction, ads::RunDBTransactionCallback callback) {
  [bridge_ runDBTransaction:std::move(transaction) callback:callback];
}

void NativeAdsClient::OnAdRewardsChanged() {
  [bridge_ onAdRewardsChanged];
}
