/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_SELECTOR_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_SELECTOR_H_

#include "base/memory/raw_ref.h"

namespace ntp_background_images {

class NTPBraveBackgroundWallpaperProvider;
class NTPGradientWallpaperProvider;
class NTPSolidColorWallpaperProvider;
class NTPUseYourOwnWallpaperProvider;
class NTPWallpaperProvider;

// Use your own, Gradients, Solid colors, and Brave backgrounds are mutually
// exclusive choices the user configures, so only one is ever active. This
// selects which one that is, so callers do not each need to check all four
// themselves.
class NTPWallpaperProviderSelector {
 public:
  NTPWallpaperProviderSelector(
      NTPUseYourOwnWallpaperProvider& use_your_own_wallpaper_provider,
      NTPGradientWallpaperProvider& gradient_wallpaper_provider,
      NTPSolidColorWallpaperProvider& solid_color_wallpaper_provider,
      NTPBraveBackgroundWallpaperProvider& brave_background_wallpaper_provider);

  NTPWallpaperProviderSelector(const NTPWallpaperProviderSelector&) = delete;
  NTPWallpaperProviderSelector& operator=(
      const NTPWallpaperProviderSelector&) = delete;

  ~NTPWallpaperProviderSelector();

  // Brave backgrounds is the fallback and is always eligible, so this always
  // returns a provider, never nothing.
  NTPWallpaperProvider& GetWallpaperProvider() const;

 private:
  const raw_ref<NTPUseYourOwnWallpaperProvider>
      use_your_own_wallpaper_provider_;
  const raw_ref<NTPGradientWallpaperProvider> gradient_wallpaper_provider_;
  const raw_ref<NTPSolidColorWallpaperProvider> solid_color_wallpaper_provider_;
  const raw_ref<NTPBraveBackgroundWallpaperProvider>
      brave_background_wallpaper_provider_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_SELECTOR_H_
