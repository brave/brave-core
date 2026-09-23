/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/test/fake_ads_client.h"

#include <utility>

namespace brave_ads::test {

FakeAdsClient::FakeAdsClient() = default;

FakeAdsClient::~FakeAdsClient() = default;

bool FakeAdsClient::IsNetworkConnectionAvailable() const {
  return false;
}

bool FakeAdsClient::IsBrowserActive() const {
  return false;
}

bool FakeAdsClient::IsBrowserInFullScreenMode() const {
  return false;
}

bool FakeAdsClient::CanShowNotificationAds() const {
  return false;
}

bool FakeAdsClient::CanShowNotificationAdsWhileBrowserIsBackgrounded() const {
  return false;
}

void FakeAdsClient::GetSiteHistory(int /*max_count*/,
                                   int /*recent_day_range*/,
                                   GetSiteHistoryCallback callback) {
  std::move(callback).Run(/*site_history=*/{});
}

void FakeAdsClient::Save(const std::string& /*name*/,
                         const std::string& /*value*/,
                         ResultCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeAdsClient::Remove(const std::string& /*name*/,
                           ResultCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeAdsClient::Load(const std::string& /*name*/, LoadCallback callback) {
  std::move(callback).Run(/*value=*/std::nullopt);
}

void FakeAdsClient::LoadResourceComponent(
    const std::string& /*id*/,
    int /*version*/,
    LoadResourceComponentCallback callback) {
  std::move(callback).Run(/*file=*/{}, /*exists=*/false);
}

bool FakeAdsClient::FindProfilePref(const std::string& /*path*/) const {
  return false;
}

std::optional<base::Value> FakeAdsClient::GetProfilePref(
    const std::string& /*path*/) {
  return std::nullopt;
}

bool FakeAdsClient::HasProfilePrefPath(const std::string& /*path*/) const {
  return false;
}

bool FakeAdsClient::FindLocalStatePref(const std::string& /*path*/) const {
  return false;
}

std::optional<base::Value> FakeAdsClient::GetLocalStatePref(
    const std::string& /*path*/) {
  return std::nullopt;
}

bool FakeAdsClient::HasLocalStatePrefPath(const std::string& /*path*/) const {
  return false;
}

base::DictValue FakeAdsClient::GetVirtualPrefs() const {
  return {};
}

}  // namespace brave_ads::test
