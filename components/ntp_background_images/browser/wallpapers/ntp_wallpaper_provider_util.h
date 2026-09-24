/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_UTIL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_UTIL_H_

#include <string_view>

namespace ntp_background_images {

// The color pref stores CSS gradients under the same key as plain colors, so
// gradient and solid color providers both need to tell the two apart.
bool IsGradientColor(std::string_view color);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_UTIL_H_
