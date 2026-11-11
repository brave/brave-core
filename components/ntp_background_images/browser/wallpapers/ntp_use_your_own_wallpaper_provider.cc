/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_use_your_own_wallpaper_provider.h"

#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_constants.h"
#include "url/gurl.h"

namespace ntp_background_images {

// The uploaded photo and its opt-in state live in
// `NTPCustomBackgroundDelegate`; this class only decides when that option
// applies and shapes its payload.

NTPUseYourOwnWallpaperProvider::NTPUseYourOwnWallpaperProvider(
    NTPCustomBackgroundDelegate& custom_background_delegate)
    : custom_background_delegate_(custom_background_delegate) {}

NTPUseYourOwnWallpaperProvider::~NTPUseYourOwnWallpaperProvider() = default;

bool NTPUseYourOwnWallpaperProvider::IsEligible() const {
  return custom_background_delegate_->IsCustomImageBackgroundEnabled();
}

void NTPUseYourOwnWallpaperProvider::MaybeGetWallpaper(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback) {
  const GURL url = custom_background_delegate_->GetCustomBackgroundImageURL();
  std::move(callback).Run(
      base::DictValue()
          .Set(kIsBackgroundKey, true)
          .Set(kWallpaperURLKey, url.spec())
          .Set(kWallpaperTypeKey, kWallpaperTypeImage)
          .Set(kWallpaperRandomKey,
               custom_background_delegate_->ShouldUseRandomValue()));
}

}  // namespace ntp_background_images
