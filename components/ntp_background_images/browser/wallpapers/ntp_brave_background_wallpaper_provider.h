/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_BRAVE_BACKGROUND_WALLPAPER_PROVIDER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_BRAVE_BACKGROUND_WALLPAPER_PROVIDER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider.h"

namespace ntp_background_images {

class NTPBackgroundImagesService;
class NTPCustomBackgroundDelegate;
class ViewCounterModel;

// Brave-provided background photos. Always eligible: this is both the
// destination when the user picks a specific photo with random rotation off,
// and the final fallback when nothing else applies.
class NTPBraveBackgroundWallpaperProvider final : public NTPWallpaperProvider {
 public:
  NTPBraveBackgroundWallpaperProvider(
      NTPCustomBackgroundDelegate& custom_background_delegate,
      NTPBackgroundImagesService& background_images_service,
      ViewCounterModel& view_counter_model);

  NTPBraveBackgroundWallpaperProvider(
      const NTPBraveBackgroundWallpaperProvider&) = delete;
  NTPBraveBackgroundWallpaperProvider& operator=(
      const NTPBraveBackgroundWallpaperProvider&) = delete;

  ~NTPBraveBackgroundWallpaperProvider() override;

  // NTPWallpaperProvider:
  bool IsEligible() const override;
  void MaybeGetWallpaper(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback)
      override;

 private:
  const raw_ref<NTPCustomBackgroundDelegate> custom_background_delegate_;
  const raw_ref<NTPBackgroundImagesService> background_images_service_;
  const raw_ref<ViewCounterModel> view_counter_model_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_BRAVE_BACKGROUND_WALLPAPER_PROVIDER_H_
