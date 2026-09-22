/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_USE_YOUR_OWN_WALLPAPER_PROVIDER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_USE_YOUR_OWN_WALLPAPER_PROVIDER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider.h"

namespace ntp_background_images {

class NTPCustomBackgroundDelegate;

// Eligible when the user has set their own uploaded photo as the NTP
// background.
class NTPUseYourOwnWallpaperProvider final : public NTPWallpaperProvider {
 public:
  explicit NTPUseYourOwnWallpaperProvider(
      NTPCustomBackgroundDelegate& custom_background_delegate);

  NTPUseYourOwnWallpaperProvider(const NTPUseYourOwnWallpaperProvider&) =
      delete;
  NTPUseYourOwnWallpaperProvider& operator=(
      const NTPUseYourOwnWallpaperProvider&) = delete;

  ~NTPUseYourOwnWallpaperProvider() override;

  // NTPWallpaperProvider:
  bool IsEligible() const override;
  void MaybeGetWallpaper(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback)
      override;

 private:
  const raw_ref<NTPCustomBackgroundDelegate> custom_background_delegate_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_USE_YOUR_OWN_WALLPAPER_PROVIDER_H_
