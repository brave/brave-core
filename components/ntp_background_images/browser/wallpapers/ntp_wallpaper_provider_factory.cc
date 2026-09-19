/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_factory.h"

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_brave_background_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_gradient_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_solid_color_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_use_your_own_wallpaper_provider.h"

namespace ntp_background_images {

NTPWallpaperProviderFactory::NTPWallpaperProviderFactory(
    NTPUseYourOwnWallpaperProvider& use_your_own_wallpaper_provider,
    NTPGradientWallpaperProvider& gradient_wallpaper_provider,
    NTPSolidColorWallpaperProvider& solid_color_wallpaper_provider,
    NTPBraveBackgroundWallpaperProvider& brave_background_wallpaper_provider)
    : use_your_own_wallpaper_provider_(use_your_own_wallpaper_provider),
      gradient_wallpaper_provider_(gradient_wallpaper_provider),
      solid_color_wallpaper_provider_(solid_color_wallpaper_provider),
      brave_background_wallpaper_provider_(
          brave_background_wallpaper_provider) {}

NTPWallpaperProviderFactory::~NTPWallpaperProviderFactory() = default;

NTPWallpaperProvider&
NTPWallpaperProviderFactory::GetWallpaperProvider() const {
  if (use_your_own_wallpaper_provider_->IsEligible()) {
    return *use_your_own_wallpaper_provider_;
  }

  if (gradient_wallpaper_provider_->IsEligible()) {
    return *gradient_wallpaper_provider_;
  }

  if (solid_color_wallpaper_provider_->IsEligible()) {
    return *solid_color_wallpaper_provider_;
  }

  return *brave_background_wallpaper_provider_;
}

}  // namespace ntp_background_images
