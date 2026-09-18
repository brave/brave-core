/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_solid_color_wallpaper_provider.h"

#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_constants.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_util.h"

namespace ntp_background_images {

NTPSolidColorWallpaperProvider::NTPSolidColorWallpaperProvider(
    NTPCustomBackgroundDelegate& custom_background_delegate)
    : custom_background_delegate_(custom_background_delegate) {}

NTPSolidColorWallpaperProvider::~NTPSolidColorWallpaperProvider() = default;

bool NTPSolidColorWallpaperProvider::IsEligible() const {
  return custom_background_delegate_->IsColorBackgroundEnabled() &&
         !IsGradientColor(custom_background_delegate_->GetColor());
}

void NTPSolidColorWallpaperProvider::MaybeGetWallpaper(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback) {
  std::move(callback).Run(
      base::DictValue()
          .Set(kIsBackgroundKey, true)
          .Set(kWallpaperColorKey, custom_background_delegate_->GetColor())
          .Set(kWallpaperTypeKey, kWallpaperTypeColor)
          .Set(kWallpaperRandomKey,
               custom_background_delegate_->ShouldUseRandomValue()));
}

}  // namespace ntp_background_images
