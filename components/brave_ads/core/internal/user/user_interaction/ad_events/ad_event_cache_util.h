/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_CACHE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_CACHE_UTIL_H_

#include <vector>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class AdType;
class ConfirmationType;
struct AdEventInfo;

void RebuildAdEventCache();

void CacheAdEvent(const AdEventInfo& ad_event);

std::vector<base::Time> GetCachedAdEvents(
    const AdType& ad_type,
    const ConfirmationType& confirmation_type);

void ResetAdEventCache();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_USER_INTERACTION_AD_EVENTS_AD_EVENT_CACHE_UTIL_H_
