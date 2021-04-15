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

bool NativeAdsClient::IsNetworkConnectionAvailable() const {
  return [bridge_ isNetworkConnectionAvailable];
}

bool NativeAdsClient::IsForeground() const {
  return [bridge_ isForeground];
}

bool NativeAdsClient::IsFullScreen() const {
  return [bridge_ isFullScreen];
}

bool NativeAdsClient::CanShowBackgroundNotifications() const {
    return [bridge_ canShowBackgroundNotifications];
}

void NativeAdsClient::ShowNotification(const ads::AdNotificationInfo & info) {
  [bridge_ showNotification:info];
}

bool NativeAdsClient::ShouldShowNotifications() {
  return [bridge_ shouldShowNotifications];
}

void NativeAdsClient::CloseNotification(const std::string & uuid) {
  [bridge_ closeNotification:uuid];
}

void NativeAdsClient::RecordAdEvent(const std::string& ad_type,
                                    const std::string& confirmation_type,
                                    const uint64_t timestamp) const {
  [bridge_ recordAdEvent:ad_type
        confirmationType:confirmation_type
               timestamp:timestamp];
}

std::vector<uint64_t> NativeAdsClient::GetAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  return [bridge_ getAdEvents:ad_type confirmationType:confirmation_type];
}

void NativeAdsClient::UrlRequest(ads::UrlRequestPtr url_request, ads::UrlRequestCallback callback) {
  [bridge_ UrlRequest:std::move(url_request) callback:callback];
}

void NativeAdsClient::Save(const std::string & name, const std::string & value, ads::ResultCallback callback) {
  [bridge_ save:name value:value callback:callback];
}

void NativeAdsClient::LoadAdsResource(const std::string& id,
                                      const int version,
                                      ads::LoadCallback callback) {
  [bridge_ loadAdsResource:id version:version callback:callback];
}

void NativeAdsClient::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    ads::GetBrowsingHistoryCallback callback) {
  [bridge_ getBrowsingHistory:max_count forDays:days_ago callback:callback];
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

void NativeAdsClient::RunDBTransaction(ads::DBTransactionPtr transaction, ads::RunDBTransactionCallback callback) {
  [bridge_ runDBTransaction:std::move(transaction) callback:callback];
}

void NativeAdsClient::OnAdRewardsChanged() {
  [bridge_ onAdRewardsChanged];
}

void NativeAdsClient::SetBooleanPref(const std::string & path, const bool value) {
  [bridge_ setBooleanPref:path value:value];
}

bool NativeAdsClient::GetBooleanPref(const std::string & path) const {
  return [bridge_ getBooleanPref:path];
}

void NativeAdsClient::SetIntegerPref(const std::string & path, const int value) {
  [bridge_ setIntegerPref:path value:value];
}

int NativeAdsClient::GetIntegerPref(const std::string & path) const {
  return [bridge_ getIntegerPref:path];
}

void NativeAdsClient::SetDoublePref(const std::string & path, const double value) {
  [bridge_ setDoublePref:path value:value];
}

double NativeAdsClient::GetDoublePref(const std::string & path) const {
  return [bridge_ getDoublePref:path];
}

void NativeAdsClient::SetStringPref(const std::string & path, const std::string & value) {
  [bridge_ setStringPref:path value:value];
}

std::string NativeAdsClient::GetStringPref(const std::string& path) const {
  return [bridge_ getStringPref:path];
}

void NativeAdsClient::SetInt64Pref(const std::string& path, const int64_t value) {
  [bridge_ setInt64Pref:path value:value];
}

int64_t NativeAdsClient::GetInt64Pref(const std::string& path) const {
  return [bridge_ getInt64Pref:path];
}

void NativeAdsClient::SetUint64Pref(const std::string& path, const uint64_t value) {
  [bridge_ setUint64Pref:path value:value];
}

uint64_t NativeAdsClient::GetUint64Pref(const std::string& path) const {
  return [bridge_ getUint64Pref:path];
}

void NativeAdsClient::ClearPref(const std::string & path) {
  [bridge_ clearPref:path];
}

void NativeAdsClient::RecordP2AEvent(const std::string& name, const ads::P2AEventType type, const std::string& value) {
  [bridge_ recordP2AEvent:name type:type value:value];
}
