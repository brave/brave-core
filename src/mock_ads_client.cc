/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/mock_ads_client.h"
#include "../include/ads_impl.h"
#include "../include/ads.h"

namespace bat_ads {

MockAdsClient::MockAdsClient() : ads_(ads::Ads::CreateInstance(this)) {
  // TODO(Terry Mancey): Mock AdsClient (#3)
}

MockAdsClient::~MockAdsClient() = default;

void MockAdsClient::Initialize() {
}

void MockAdsClient::AppFocused(bool focused) {
}

void MockAdsClient::TabUpdate() {
}

void MockAdsClient::RecordUnIdle() {
}

void MockAdsClient::RemoveAllHistory() {
}

void MockAdsClient::SaveCachedInfo() {
}

void MockAdsClient::ConfirmAdUUIDIfAdEnabled(bool enabled) {
}

void MockAdsClient::TestShoppingData(const std::string& url) {
}

void MockAdsClient::TestSearchState(const std::string& url) {
}

void MockAdsClient::RecordMediaPlaying(bool active, uint64_t tabId) {
}

void MockAdsClient::ClassifyPage(uint64_t windowId) {
}

void MockAdsClient::ChangeLocale(const std::string& locale) {
}

void MockAdsClient::CollectActivity() {
}

void MockAdsClient::InitializeCatalog() {
}

void MockAdsClient::RetrieveSSID(uint64_t error, const std::string& ssid) {
}

void MockAdsClient::CheckReadyAdServe(uint64_t windowId, bool forceP) {
}

void MockAdsClient::ServeSampleAd(uint64_t windowId) {
}

}  // namespace bat_ads
