/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_

namespace base {
struct Feature;
}  // namespace base

namespace ntp_background_images {
namespace features {
extern const base::Feature kBraveNTPBrandedWallpaper;
extern const base::Feature kBraveNTPBrandedWallpaperDemo;
extern const base::Feature kBraveNTPSuperReferrerWallpaper;
}  // namespace features
}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_FEATURES_H_
