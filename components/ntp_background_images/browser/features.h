/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace ntp_background_images {
namespace features {

BASE_DECLARE_FEATURE(kBraveNTPBrandedWallpaperDemo);

BASE_DECLARE_FEATURE(kBraveNTPSuperReferralWallpaper);

BASE_DECLARE_FEATURE(kBraveNTPBrandedWallpaper);

// Show initial branded wallpaper after nth new tab page for fresh opens.
inline constexpr base::FeatureParam<int> kInitialCountToBrandedWallpaper{
    &kBraveNTPBrandedWallpaper, "initial_count_to_branded_wallpaper", 2};

// Show branded wallpaper every nth new tab page.
inline constexpr base::FeatureParam<int> kCountToBrandedWallpaper{
    &kBraveNTPBrandedWallpaper, "count_to_branded_wallpaper", 3};

// Reset counter when a specific amount of time has elapsed in SI mode.
inline constexpr base::FeatureParam<base::TimeDelta> kResetCounterAfter{
    &kBraveNTPBrandedWallpaper, "reset_counter_after", base::Days(1)};

}  // namespace features
}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
