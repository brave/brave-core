/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/notification_ad_serving_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::notification_ads {

namespace {

constexpr base::TimeDelta kServeFirstAdAfterDelay = base::Minutes(2);
constexpr base::TimeDelta kMinimumDelayBeforeServingAnAd = base::Minutes(1);

bool HasPreviouslyServedAnAd() {
  return AdsClientHelper::GetInstance()->HasPrefPath(prefs::kServeAdAt);
}

bool ShouldServeAd() {
  return base::Time::Now() >= ServeAdAt();
}

}  // namespace

bool ShouldServeAdsAtRegularIntervals() {
  return PlatformHelper::GetInstance()->IsMobile();
}

base::Time ServeAdAt() {
  return AdsClientHelper::GetInstance()->GetTimePref(prefs::kServeAdAt);
}

void SetServeAdAt(const base::Time serve_ad_at) {
  AdsClientHelper::GetInstance()->SetTimePref(prefs::kServeAdAt, serve_ad_at);
}

base::TimeDelta CalculateDelayBeforeServingAnAd() {
  if (!HasPreviouslyServedAnAd()) {
    return kServeFirstAdAfterDelay;
  }

  if (ShouldServeAd()) {
    return kMinimumDelayBeforeServingAnAd;
  }

  base::TimeDelta delay = ServeAdAt() - base::Time::Now();
  if (delay.is_negative()) {
    delay = base::TimeDelta();
  }

  return delay;
}

}  // namespace ads::notification_ads
