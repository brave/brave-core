/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct AdEventInfo;
struct CreativeAdInfo;

namespace test {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         AdType ad_type,
                         ConfirmationType confirmation_type,
                         base::Time created_at,
                         bool should_use_random_uuids);

void RecordAdEvent(AdType ad_type, ConfirmationType confirmation_type);
void RecordAdEvents(AdType ad_type,
                    ConfirmationType confirmation_type,
                    int count);

void RecordAdEvent(const AdEventInfo& ad_event);
void RecordAdEvents(const AdEventInfo& ad_event, int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
