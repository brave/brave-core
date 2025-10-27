/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/features.h"

#include "base/feature_list.h"

namespace ntp_background_images::features {

BASE_FEATURE(kBraveNTPBrandedWallpaperDemo,
             "BraveNTPBrandedWallpaperDemoName",
             base::FEATURE_DISABLED_BY_DEFAULT);

// TODO(https://github.com/brave/brave-browser/issues/44403): Remove super
// referrals.
BASE_FEATURE(kBraveNTPSuperReferralWallpaper,
             "BraveNTPSuperReferralWallpaperName",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveNTPBrandedWallpaperSurveyPanelist,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveNTPBrandedWallpaper,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace ntp_background_images::features
