/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/notification_ad_value_util.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_ads/core/internal/browser/browser_util.h"
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

absl::optional<NotificationAdInfo>
NotificationAdManager::MaybeGetForPlacementId(
    const std::string& placement_id) const {
  CHECK(!placement_id.empty());

  const auto iter =
      base::ranges::find(ads_, placement_id, &NotificationAdInfo::placement_id);
  if (iter == ads_.cend()) {
    return absl::nullopt;
  }

  NotificationAdInfo ad = *iter;
  ad.type = AdType::kNotificationAd;
  return ad;
}

void NotificationAdManager::Add(const NotificationAdInfo& ad) {
  CHECK(ad.IsValid());

  ads_.push_back(ad);

#if BUILDFLAG(IS_ANDROID)
  if (ads_.size() > kMaximumNotificationAds) {
    AdsClientHelper::GetInstance()->CloseNotificationAd(
        ads_.front().placement_id);

    ads_.pop_front();
  }
#endif  // BUILDFLAG(IS_ANDROID)

  AdsClientHelper::GetInstance()->SetListPref(prefs::kNotificationAds,
                                              NotificationAdsToValue(ads_));
}

bool NotificationAdManager::Remove(const std::string& placement_id) {
  CHECK(!placement_id.empty());

  const auto iter =
      base::ranges::find(ads_, placement_id, &NotificationAdInfo::placement_id);
  if (iter == ads_.cend()) {
    return false;
  }

  ads_.erase(iter);

  AdsClientHelper::GetInstance()->SetListPref(prefs::kNotificationAds,
                                              NotificationAdsToValue(ads_));

  return true;
}

void NotificationAdManager::RemoveAll() {
  ads_.clear();

  AdsClientHelper::GetInstance()->SetListPref(prefs::kNotificationAds,
                                              NotificationAdsToValue(ads_));
}

void NotificationAdManager::CloseAll() {
  for (const auto& ad : ads_) {
    AdsClientHelper::GetInstance()->CloseNotificationAd(ad.placement_id);
  }

  RemoveAll();
}

bool NotificationAdManager::Exists(const std::string& placement_id) const {
  CHECK(!placement_id.empty());

  return base::ranges::find(ads_, placement_id,
                            &NotificationAdInfo::placement_id) != ads_.cend();
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdManager::Initialize() {
  const absl::optional<base::Value::List> list =
      AdsClientHelper::GetInstance()->GetListPref(prefs::kNotificationAds);
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
    RemoveAll();
  }
#endif  // BUILDFLAG(IS_ANDROID)
}

}  // namespace brave_ads
