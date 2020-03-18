/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/url_constants.h"

namespace ntp_background_images {

const char kBrandedWallpaperHost[] = "branded-wallpaper";
const char kLogoPath[] = "logo.png";
const char kWallpaperPathPrefix[] = "wallpaper-";
// TODO(simonhong): Use prod url before merging.
const char kSuperReferrerMappingTableURL[] =
    "https://brave-ntp-crx-input-dev.s3-us-west-2.amazonaws.com/superreferrer/map-table.json";  // NOLINT

}  // namespace ntp_background_images
