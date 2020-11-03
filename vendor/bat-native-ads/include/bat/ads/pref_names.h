/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_PREF_NAMES_H_
#define BAT_ADS_PREF_NAMES_H_

namespace ads {

namespace prefs {

extern const char kEnabled[];

extern const char kShouldAllowConversionTracking[];

extern const char kAdsPerHour[];
extern const char kAdsPerDay[];

extern const char kIdleThreshold[];

extern const char kShouldAllowAdsSubdivisionTargeting[];
extern const char kAdsSubdivisionTargetingCode[];
extern const char kAutoDetectedAdsSubdivisionTargetingCode[];

}  // namespace prefs

}  // namespace ads

#endif  // BAT_ADS_PREF_NAMES_H_
