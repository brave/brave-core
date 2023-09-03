/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_

#include <cstddef>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class AdType;
class ConfirmationType;
struct AdEventInfo;
struct CreativeAdInfo;

AdEventInfo BuildAdEventForTesting(const CreativeAdInfo& creative_ad,
                                   const AdType& ad_type,
                                   const ConfirmationType& confirmation_type,
                                   base::Time created_at);

void RecordAdEventForTesting(const AdType& type,
                             const ConfirmationType& confirmation_type);
void RecordAdEventsForTesting(const AdType& type,
                              const ConfirmationType& confirmation_type,
                              int count);

void FireAdEventForTesting(const AdEventInfo& ad_event);
void FireAdEventsForTesting(const AdEventInfo& ad_event, size_t count);

size_t GetAdEventCountForTesting(const AdType& ad_type,
                                 const ConfirmationType& confirmation_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
