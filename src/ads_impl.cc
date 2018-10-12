/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/ads_impl.h"
#include "../include/ads_client.h"
#include "../include/bat_client.h"

namespace bat_ads {

AdsImpl::AdsImpl(ads::AdsClient* ads_client) :
    ads_client_(ads_client),
    bat_client_(new ads_bat_client::BatClient(this)),
    focused_(false) {
}

AdsImpl::~AdsImpl() = default;

void AdsImpl::Initialize() {
  // TODO(Terry Mancey): Implement Initialize (#42)
}

void AdsImpl::AppFocused(bool focused) {
  focused_ = focused;
}

void AdsImpl::TabUpdate() {
  // TODO(Terry Mancey): Implement TabUpdate (#14)
}

void AdsImpl::RecordUnIdle() {
  // TODO(Terry Mancey): Implement RecordUnIdle (#15)
}

void AdsImpl::RemoveAllHistory() {
  // TODO(Terry Mancey): Implement RemoveAllHistory (#16)
}

void AdsImpl::SaveCachedInfo() {
  // TODO(Terry Mancey): Implement SaveCachedInfo (#17)
}

void AdsImpl::ConfirmAdUUIDIfAdEnabled(bool enabled) {
  // TODO(Terry Mancey): Implement ConfirmAdUUIDIfAdEnabled (#18)
}

void AdsImpl::TestShoppingData(const std::string& url) {
  // TODO(Terry Mancey): Implement TestShoppingData (#19)
}

void AdsImpl::TestSearchState(const std::string& url) {
  // TODO(Terry Mancey): Implement TestSearchState (#20)
}

void AdsImpl::RecordMediaPlaying(bool active, uint64_t tabId) {
  // TODO(Terry Mancey): Implement RecordMediaPlaying (#21)
}

void AdsImpl::ClassifyPage(uint64_t windowId) {
  // TODO(Terry Mancey): Implement ClassifyPage (#22)
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  // TODO(Terry Mancey): Implement ChangeLocale (#23)
}

void AdsImpl::CollectActivity() {
  // TODO(Terry Mancey): Implement CollectActivity (#24)
}

void AdsImpl::InitializeCatalog() {
  // TODO(Terry Mancey): Implement InitializeCatalog (#25)
}

void AdsImpl::RetrieveSSID(uint64_t error, const std::string& ssid) {
  // TODO(Terry Mancey): Implement RetrieveSSID (#38)
}

void AdsImpl::CheckReadyAdServe(uint64_t windowId, bool forceP) {
  // TODO(Terry Mancey): Implement CheckReadyAdServe (#26)
}

void AdsImpl::ServeSampleAd(uint64_t windowId) {
  // TODO(Terry Mancey): Implement ServeSampleAd (#27)
}

void AdsImpl::OnTimer(uint32_t timer_id) {
  // TODO(Terry Mancey): Download catalog
}

void AdsImpl::SaveState(const std::string& json) {
  ads_client_->SaveState(json, this);
}

void AdsImpl::SetCampaignInfo(std::unique_ptr<catalog::CampaignInfo> info,
    ads::CampaignInfoCallback callback) {
  ads_client_->SaveCampaignInfo(std::move(info),
    std::bind(&AdsImpl::OnSetCampaignInfo, this, callback,
    std::placeholders::_1, std::placeholders::_2));
}

void AdsImpl::OnSetCampaignInfo(ads::CampaignInfoCallback callback,
    ads::Result result, std::unique_ptr<catalog::CampaignInfo> info) {
  callback(result, std::move(info));
}

void AdsImpl::GetCampaignInfo(const catalog::CampaignInfoFilter& filter,
    ads::CampaignInfoCallback callback) {
  ads_client_->LoadCampaignInfo(filter, callback);
}

void AdsImpl::SetCreativeSetInfo(std::unique_ptr<catalog::CreativeSetInfo> info,
    ads::CreativeSetInfoCallback callback) {
  ads_client_->SaveCreativeSetInfo(std::move(info),
    std::bind(&AdsImpl::OnSetCreativeSetInfo, this, callback,
    std::placeholders::_1, std::placeholders::_2));
}

void AdsImpl::OnSetCreativeSetInfo(ads::CreativeSetInfoCallback callback,
    ads::Result result, std::unique_ptr<catalog::CreativeSetInfo> info) {
  callback(result, std::move(info));
}

void AdsImpl::GetCreativeSetInfo(const catalog::CreativeSetInfoFilter& filter,
    ads::CreativeSetInfoCallback callback) {
  ads_client_->LoadCreativeSetInfo(filter, callback);
}

std::string AdsImpl::URIEncode(const std::string& value) {
  return ads_client_->URIEncode(value);
}

std::unique_ptr<ads::AdsURLLoader> AdsImpl::LoadURL(
    const std::string& url, const std::vector<std::string>& headers,
    const std::string& content, const std::string& contentType,
    const ads::URL_METHOD& method, ads::CallbackHandler* handler) {
  return ads_client_->LoadURL(url, headers, content,
    contentType, method, handler);
}

}  // namespace bat_ads
