/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"
#endif  // BUILDFLAG(IS_ANDROID)

namespace brave_ads {

namespace {

#if BUILDFLAG(IS_ANDROID)
constexpr int kMaximumNotificationAds = 3;
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace

NotificationAdManager::NotificationAdManager() {
  Initialize();
}

NotificationAdManager::~NotificationAdManager() = default;

// static
NotificationAdManager& NotificationAdManager::GetInstance() {
  return GlobalState::GetInstance()->GetNotificationAdManager();
}

std::optional<NotificationAdInfo> NotificationAdManager::MaybeGetForPlacementId(
    const std::string& placement_id) const {
  CHECK(!placement_id.empty());

  const auto iter =
      base::ranges::find(ads_, placement_id, &NotificationAdInfo::placement_id);
  if (iter == ads_.cend()) {
    return std::nullopt;
  }

  return *iter;
}

void NotificationAdManager::Add(const NotificationAdInfo& ad) {
  CHECK(ad.IsValid());

  ads_.push_back(ad);

  GetAdsClient()->ShowNotificationAd(ad);

#if BUILDFLAG(IS_ANDROID)
  if (ads_.size() > kMaximumNotificationAds) {
    GetAdsClient()->CloseNotificationAd(ads_.front().placement_id);
    ads_.pop_front();
  }
#endif  // BUILDFLAG(IS_ANDROID)

  SetProfileListPref(prefs::kNotificationAds, NotificationAdsToValue(ads_));
}

void NotificationAdManager::Remove(const std::string& placement_id,
                                   const bool should_close) {
  CHECK(!placement_id.empty());

  if (should_close) {
    GetAdsClient()->CloseNotificationAd(placement_id);
  }

  const auto iter =
      base::ranges::find(ads_, placement_id, &NotificationAdInfo::placement_id);
  if (iter == ads_.cend()) {
    return;
  }

  ads_.erase(iter);

  SetProfileListPref(prefs::kNotificationAds, NotificationAdsToValue(ads_));
}

void NotificationAdManager::RemoveAll(bool should_close) {
  if (should_close) {
    for (const auto& ad : ads_) {
      GetAdsClient()->CloseNotificationAd(ad.placement_id);
    }
  }

  ads_.clear();
  ads_.shrink_to_fit();

  SetProfileListPref(prefs::kNotificationAds, NotificationAdsToValue(ads_));
}

bool NotificationAdManager::Exists(const std::string& placement_id) const {
  CHECK(!placement_id.empty());

  return base::ranges::find(ads_, placement_id,
                            &NotificationAdInfo::placement_id) != ads_.cend();
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdManager::Initialize() {
  const std::optional<base::Value::List> list =
      GetProfileListPref(prefs::kNotificationAds);
  if (!list) {
    return;
  }
  ads_ = NotificationAdsFromValue(*list);

  MaybeRemoveAll();
}

void NotificationAdManager::MaybeRemoveAll() {
#if BUILDFLAG(IS_ANDROID)
  if (WasBrowserUpgraded()) {
    // Android deletes notifications after upgrading an app, so we should remove
    // orphaned notification ads after a browser upgrade.
    RemoveAll(/*should_close=*/false);
  }
#endif  // BUILDFLAG(IS_ANDROID)
}

}  // namespace brave_ads
