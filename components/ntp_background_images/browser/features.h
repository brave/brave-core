/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace ntp_background_images {
namespace features {

BASE_DECLARE_FEATURE(kBraveNTPBrandedWallpaperDemo);

BASE_DECLARE_FEATURE(kBraveNTPSuperReferralWallpaper);

BASE_DECLARE_FEATURE(kBraveNTPBrandedWallpaper);

// Show initial branded wallpaper after nth new tab page for fresh opens.
constexpr base::FeatureParam<int> kInitialCountToBrandedWallpaper{
    &kBraveNTPBrandedWallpaper, "initial_count_to_branded_wallpaper", 1};

// Show branded wallpaper every nth new tab page.
constexpr base::FeatureParam<int> kCountToBrandedWallpaper{
    &kBraveNTPBrandedWallpaper, "count_to_branded_wallpaper", 2};

}  // namespace features
}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
