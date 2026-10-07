/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_brave_background_wallpaper_provider.h"

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_custom_background_delegate.h"

namespace ntp_background_images {

NTPBraveBackgroundWallpaperProvider::NTPBraveBackgroundWallpaperProvider(
    NTPCustomBackgroundDelegate& custom_background_delegate,
    NTPBackgroundImagesService& background_images_service,
    ViewCounterModel& view_counter_model)
    : custom_background_delegate_(custom_background_delegate),
      background_images_service_(background_images_service),
      view_counter_model_(view_counter_model) {}

NTPBraveBackgroundWallpaperProvider::
    ~NTPBraveBackgroundWallpaperProvider() = default;

bool NTPBraveBackgroundWallpaperProvider::IsEligible() const {
  return true;
}

void NTPBraveBackgroundWallpaperProvider::MaybeGetWallpaper(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback) {
  if (custom_background_delegate_->HasPreferredBraveBackground()) {
    base::DictValue brave_background_dict =
        custom_background_delegate_->GetPreferredBraveBackground();
    if (!brave_background_dict.empty()) {
      brave_background_dict.Set(kWallpaperRandomKey, false);
      return std::move(callback).Run(std::move(brave_background_dict));
    }
    // The pinned photo's data is missing. Fall back to a random one below.
  }

  const NTPBackgroundImagesData* const background_images_data =
      background_images_service_->GetBackgroundImagesData();
  if (!background_images_data) {
    return std::move(callback).Run(std::nullopt);
  }

  const int index = view_counter_model_->current_wallpaper_image_index();
  if (index < 0 || static_cast<size_t>(index) >=
                       background_images_data->backgrounds.size()) {
    // A component update can shrink `backgrounds` after `index` was chosen
    // against the previous count.
    return std::move(callback).Run(std::nullopt);
  }

  base::DictValue background_dict =
      background_images_data->GetBackgroundAt(index);
  background_dict.Set(kWallpaperRandomKey, true);
  std::move(callback).Run(std::move(background_dict));
}

}  // namespace ntp_background_images
