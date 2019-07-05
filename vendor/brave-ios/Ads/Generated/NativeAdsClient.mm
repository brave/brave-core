/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "NativeAdsClient.h"
#import "NativeAdsClientBridge.h"

// Constructor & Destructor
NativeAdsClient::NativeAdsClient(id<NativeAdsClientBridge> bridge) : bridge_(bridge) { }
NativeAdsClient::~NativeAdsClient() {
  bridge_ = nil;
}

void NativeAdsClient::ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) {
  [bridge_ confirmAd:std::move(info)];
}
void NativeAdsClient::EventLog(const std::string & json) {
  [bridge_ eventLog:json];
}
void NativeAdsClient::GetAds(const std::string & category, ads::OnGetAdsCallback callback) {
  [bridge_ getAds:category callback:callback];
}
const std::string NativeAdsClient::GetAdsLocale() const {
  return [bridge_ getAdsLocale];
}
uint64_t NativeAdsClient::GetAdsPerDay() const {
  return [bridge_ getAdsPerDay];
}
uint64_t NativeAdsClient::GetAdsPerHour() const {
  return [bridge_ getAdsPerHour];
}
void NativeAdsClient::GetClientInfo(ads::ClientInfo * info) const {
  [bridge_ getClientInfo:info];
}
const std::vector<std::string> NativeAdsClient::GetLocales() const {
  return [bridge_ getLocales];
}
bool NativeAdsClient::IsAdsEnabled() const {
  return [bridge_ isAdsEnabled];
}
bool NativeAdsClient::IsForeground() const {
  return [bridge_ isForeground];
}
bool NativeAdsClient::IsNetworkConnectionAvailable() {
  return [bridge_ isNetworkConnectionAvailable];
}
bool NativeAdsClient::IsNotificationsAvailable() const {
  return [bridge_ isNotificationsAvailable];
}
void NativeAdsClient::KillTimer(uint32_t timer_id) {
  [bridge_ killTimer:timer_id];
}
void NativeAdsClient::Load(const std::string & name, ads::OnLoadCallback callback) {
  [bridge_ load:name callback:callback];
}
const std::string NativeAdsClient::LoadJsonSchema(const std::string & name) {
  return [bridge_ loadJsonSchema:name];
}
void NativeAdsClient::LoadSampleBundle(ads::OnLoadSampleBundleCallback callback) {
  [bridge_ loadSampleBundle:callback];
}
void NativeAdsClient::LoadUserModelForLocale(const std::string & locale, ads::OnLoadCallback callback) const {
  [bridge_ loadUserModelForLocale:locale callback:callback];
}
std::unique_ptr<ads::LogStream> NativeAdsClient::Log(const char * file, const int line, const ads::LogLevel log_level) const {
  return [bridge_ log:file line:line logLevel:log_level];
}
void NativeAdsClient::Reset(const std::string & name, ads::OnResetCallback callback) {
  [bridge_ reset:name callback:callback];
}
void NativeAdsClient::Save(const std::string & name, const std::string & value, ads::OnSaveCallback callback) {
  [bridge_ save:name value:value callback:callback];
}
void NativeAdsClient::SaveBundleState(std::unique_ptr<ads::BundleState> state, ads::OnSaveCallback callback) {
  [bridge_ saveBundleState:std::move(state) callback:callback];
}
void NativeAdsClient::SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) {
  [bridge_ setCatalogIssuers:std::move(info)];
}
void NativeAdsClient::SetIdleThreshold(const int threshold) {
  [bridge_ setIdleThreshold:threshold];
}
uint32_t NativeAdsClient::SetTimer(const uint64_t time_offset) {
  return [bridge_ setTimer:time_offset];
}
void NativeAdsClient::ShowNotification(std::unique_ptr<ads::NotificationInfo> info) {
  [bridge_ showNotification:std::move(info)];
}
void NativeAdsClient::URLRequest(const std::string & url, const std::vector<std::string> & headers, const std::string & content, const std::string & content_type, const ads::URLRequestMethod method, ads::URLRequestCallback callback) {
  [bridge_ URLRequest:url headers:headers content:content contentType:content_type method:method callback:callback];
}
