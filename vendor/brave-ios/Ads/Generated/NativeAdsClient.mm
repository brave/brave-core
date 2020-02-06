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

bool NativeAdsClient::ShouldShowPublisherAdsOnPariticipatingSites() const {
  return [bridge_ shouldShowPublisherAdsOnParticipatingSites];
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

void NativeAdsClient::GetClientInfo(ads::ClientInfo * info) const {
  [bridge_ getClientInfo:info];
}

const std::string NativeAdsClient::GetLocale() const {
  return [bridge_ getLocale];
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

const std::vector<std::string> NativeAdsClient::GetUserModelLanguages() const {
  return [bridge_ getUserModelLanguages];
}

void NativeAdsClient::LoadUserModelForLanguage(const std::string & language, ads::OnLoadCallback callback) const {
  [bridge_ loadUserModelForLanguage:language callback:callback];
}

void NativeAdsClient::ShowNotification(std::unique_ptr<ads::AdNotificationInfo> info) {
  [bridge_ showNotification:std::move(info)];
}

bool NativeAdsClient::ShouldShowNotifications() {
  return [bridge_ shouldShowNotifications];
}

void NativeAdsClient::CloseNotification(const std::string& id) {
  [bridge_ closeNotification:id];
}

void NativeAdsClient::SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) {
  [bridge_ setCatalogIssuers:std::move(info)];
}

void NativeAdsClient::ConfirmAdNotification(const ads::AdNotificationInfo & info) {
  [bridge_ confirmAdNotification:info];
}

void NativeAdsClient::ConfirmPublisherAd(const ads::PublisherAdInfo & info) {
  [bridge_ confirmPublisherAd:info];
}

void NativeAdsClient::ConfirmAction(const std::string & uuid, const std::string & creative_set_id, const ads::ConfirmationType & type) {
  [bridge_ confirmAction:uuid creativeSetId:creative_set_id confirmationType:type];
}

uint32_t NativeAdsClient::SetTimer(const uint64_t time_offset) {
  return [bridge_ setTimer:time_offset];
}

void NativeAdsClient::KillTimer(uint32_t timer_id) {
  [bridge_ killTimer:timer_id];
}

void NativeAdsClient::URLRequest(const std::string & url, const std::vector<std::string> & headers, const std::string & content, const std::string & content_type, const ads::URLRequestMethod method, ads::URLRequestCallback callback) {
  [bridge_ URLRequest:url headers:headers content:content contentType:content_type method:method callback:callback];
}

void NativeAdsClient::Save(const std::string & name, const std::string & value, ads::OnSaveCallback callback) {
  [bridge_ save:name value:value callback:callback];
}

void NativeAdsClient::Load(const std::string & name, ads::OnLoadCallback callback) {
  [bridge_ load:name callback:callback];
}

void NativeAdsClient::Reset(const std::string & name, ads::OnResetCallback callback) {
  [bridge_ reset:name callback:callback];
}

const std::string NativeAdsClient::LoadJsonSchema(const std::string & name) {
  return [bridge_ loadJsonSchema:name];
}

void NativeAdsClient::LoadSampleBundle(ads::OnLoadSampleBundleCallback callback) {
  [bridge_ loadSampleBundle:callback];
}

void NativeAdsClient::SaveBundleState(std::unique_ptr<ads::BundleState> state, ads::OnSaveCallback callback) {
  [bridge_ saveBundleState:std::move(state) callback:callback];
}

void NativeAdsClient::GetCreativeAdNotifications(const std::vector<std::string> & categories, ads::OnGetCreativeAdNotificationsCallback callback) {
  [bridge_ getCreativeAdNotifications:categories callback:callback];
}

void NativeAdsClient::GetCreativePublisherAds(const std::string & url, const std::vector<std::string> & categories, const std::vector<std::string> & sizes, ads::OnGetCreativePublisherAdsCallback callback) {
  [bridge_ getCreativePublisherAds:url categories:categories callback:callback];
}

void NativeAdsClient::GetAdConversions(const std::string & url, ads::OnGetAdConversionsCallback callback) {
  [bridge_ getAdConversions:url callback:callback];
}

void NativeAdsClient::EventLog(const std::string & json) const {
  [bridge_ eventLog:json];
}

std::unique_ptr<ads::LogStream> NativeAdsClient::Log(const char * file, const int line, const ads::LogLevel log_level) const {
  return [bridge_ log:file line:line logLevel:log_level];
}
