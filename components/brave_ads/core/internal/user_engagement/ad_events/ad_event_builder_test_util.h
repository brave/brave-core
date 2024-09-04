/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_TEST_UTIL_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct CreativeAdInfo;
struct AdEventInfo;

namespace test {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         mojom::AdType mojom_ad_type,
                         mojom::ConfirmationType mojom_confirmation_type,
                         base::Time created_at,
                         bool should_generate_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_TEST_UTIL_H_
