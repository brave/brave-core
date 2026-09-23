/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_SOLID_COLOR_WALLPAPER_PROVIDER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_SOLID_COLOR_WALLPAPER_PROVIDER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider.h"

namespace ntp_background_images {

class NTPCustomBackgroundDelegate;

// The color pref stores CSS gradients under the same key as plain colors, so
// this is eligible only when the stored value does not carry a gradient
// prefix.
class NTPSolidColorWallpaperProvider final : public NTPWallpaperProvider {
 public:
  explicit NTPSolidColorWallpaperProvider(
      NTPCustomBackgroundDelegate& custom_background_delegate);

  NTPSolidColorWallpaperProvider(const NTPSolidColorWallpaperProvider&) =
      delete;
  NTPSolidColorWallpaperProvider& operator=(
      const NTPSolidColorWallpaperProvider&) = delete;

  ~NTPSolidColorWallpaperProvider() override;

  // NTPWallpaperProvider:
  bool IsEligible() const override;
  void MaybeGetWallpaper(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback)
      override;

 private:
  const raw_ref<NTPCustomBackgroundDelegate> custom_background_delegate_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_SOLID_COLOR_WALLPAPER_PROVIDER_H_
