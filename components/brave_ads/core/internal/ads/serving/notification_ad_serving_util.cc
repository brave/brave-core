/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kServeFirstAdAfter = base::Minutes(2);
constexpr base::TimeDelta kMinimumDelayBeforeServingAnAd = base::Minutes(1);

bool HasPreviouslyServedAnAd() {
  return AdsClientHelper::GetInstance()->HasPrefPath(prefs::kServeAdAt);
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
  AdsClientHelper::GetInstance()->SetTimePref(prefs::kServeAdAt, serve_ad_at);
}

base::Time ServeAdAt() {
  return AdsClientHelper::GetInstance()->GetTimePref(prefs::kServeAdAt);
}

base::TimeDelta CalculateDelayBeforeServingAnAd() {
  if (!HasPreviouslyServedAnAd()) {
    return kServeFirstAdAfter;
  }

  if (ShouldHaveServedAdInThePast() || ShouldServeAd()) {
    return kMinimumDelayBeforeServingAnAd;
  }

  const base::TimeDelta delay = DelayBeforeServingAnAd();
  if (delay < kMinimumDelayBeforeServingAnAd) {
    return kMinimumDelayBeforeServingAnAd;
  }

  return delay;
}

}  // namespace brave_ads
