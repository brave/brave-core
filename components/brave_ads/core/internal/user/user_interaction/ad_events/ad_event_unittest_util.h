/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class AdType;
class ConfirmationType;
struct AdEventInfo;
struct CreativeAdInfo;

namespace test {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         base::Time created_at,
                         bool should_use_random_uuids);

void RecordAdEvent(const AdType& type,
                   const ConfirmationType& confirmation_type);
void RecordAdEvents(const AdType& type,
                    const ConfirmationType& confirmation_type,
                    int count);

void RecordAdEvent(const AdEventInfo& ad_event);
void RecordAdEvents(const AdEventInfo& ad_event, int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
