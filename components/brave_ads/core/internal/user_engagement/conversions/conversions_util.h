/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UTIL_H_

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct AdEventInfo;

bool IsAllowedToConvertAdEvent(const AdEventInfo& ad_event);

bool DidAdEventOccurWithinObservationWindow(const AdEventInfo& ad_event,
                                            base::TimeDelta observation_window);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UTIL_H_
