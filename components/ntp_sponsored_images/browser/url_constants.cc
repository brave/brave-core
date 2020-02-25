/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/url_constants.h"

namespace ntp_sponsored_images {

// For sponsored wallpaper,
//   * chrome://branded-wallpaper/wallpaper-N.jpg
//   * chrome://branded-wallpaper/logo.png
// For referral wallpaper,
//   * chrome://referral-wallpaper/wallpaper-N.jpg
//   * chrome://referral-wallpaper/logo.png
//   * chrome://referral-wallpaper/icon-file-name.png
const char kSponsoredWallpaperHost[] = "branded-wallpaper";
const char kReferralWallpaperHost[] = "referral-wallpaper";
const char kLogoPath[] = "logo.png";
const char kWallpaperPathPrefix[] = "wallpaper-";

}  // namespace ntp_sponsored_images
