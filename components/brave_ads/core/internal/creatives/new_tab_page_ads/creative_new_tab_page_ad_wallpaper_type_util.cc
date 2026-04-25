/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_util.h"

#include <ostream>  // IWYU pragma: keep
#include <utility>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_constants.h"

namespace brave_ads {

CreativeNewTabPageAdWallpaperType ToCreativeNewTabPageAdWallpaperType(
    std::string_view wallpaper_type) {
  if (wallpaper_type == kCreativeNewTabPageAdImageWallpaperType) {
    return CreativeNewTabPageAdWallpaperType::kImage;
  }

  if (wallpaper_type == kCreativeNewTabPageAdRichMediaWallpaperType) {
    return CreativeNewTabPageAdWallpaperType::kRichMedia;
  }

  NOTREACHED() << "Unexpected value for wallpaper_type: " << wallpaper_type;
}

std::string ToString(CreativeNewTabPageAdWallpaperType wallpaper_type) {
  switch (wallpaper_type) {
    case CreativeNewTabPageAdWallpaperType::kImage: {
      return kCreativeNewTabPageAdImageWallpaperType;
    }

    case CreativeNewTabPageAdWallpaperType::kRichMedia: {
      return kCreativeNewTabPageAdRichMediaWallpaperType;
    }

    default: {
      break;
    }
  }

  NOTREACHED() << "Unexpected value for CreativeNewTabPageAdWallpaperType: "
               << std::to_underlying(wallpaper_type);
}

}  // namespace brave_ads
