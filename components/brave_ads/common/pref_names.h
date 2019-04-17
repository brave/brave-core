/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_

namespace brave_ads {

namespace prefs {

extern const char kBraveAdsEnabled[];

extern const char kBraveAdsPerHour[];
extern const char kBraveAdsPerDay[];
extern const char kBraveAdsIdleThreshold[];

extern const int kBraveAdsPrefsDefaultVersion;
extern const int kBraveAdsPrefsCurrentVersion;
extern const char kBraveAdsPrefsVersion[];
extern const char kBraveAdsPrefsMigratedFrom62[];

}  // namespace prefs

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_
