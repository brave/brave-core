/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

bool HasPreviouslyServedAnAd() {
  return HasProfilePrefPath(prefs::kServeAdAt);
}

base::TimeDelta DelayBeforeServingAnAd() {
  return ServeAdAt() - base::Time::Now();
}

bool ShouldHaveServedAdInThePast() {
  return DelayBeforeServingAnAd().is_negative();
}

bool ShouldServeAd() {
  return base::Time::Now() >= ServeAdAt();
}

}  // namespace

bool ShouldServeAdsAtRegularIntervals() {
  return PlatformHelper::GetInstance().IsMobile();
}

void SetServeAdAt(const base::Time serve_ad_at) {
  SetProfileTimePref(prefs::kServeAdAt, serve_ad_at);
}

base::Time ServeAdAt() {
  return GetProfileTimePref(prefs::kServeAdAt);
}

base::TimeDelta CalculateDelayBeforeServingAnAd() {
  if (!HasPreviouslyServedAnAd()) {
    return kServeFirstNotificationAdAfter.Get();
  }

  if (ShouldHaveServedAdInThePast() || ShouldServeAd()) {
    return kMinimumDelayBeforeServingNotificationAd.Get();
  }

  const base::TimeDelta delay = DelayBeforeServingAnAd();
  if (delay < kMinimumDelayBeforeServingNotificationAd.Get()) {
    return kMinimumDelayBeforeServingNotificationAd.Get();
  }

  return delay;
}

}  // namespace brave_ads
