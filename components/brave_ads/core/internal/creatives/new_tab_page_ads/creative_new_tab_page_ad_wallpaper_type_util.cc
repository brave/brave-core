/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_util.h"

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/types/cxx23_to_underlying.h"
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

  SCOPED_CRASH_KEY_STRING64("Issue45288", "wallpaper_type", wallpaper_type);
  SCOPED_CRASH_KEY_STRING64("Issue45288", "failure_reason",
                            "Invalid new tab page ad wallpaper type");
  base::debug::DumpWithoutCrashing();

  // Defaulting to image as a fallback for unrecognized wallpaper types.
  // This is a temporary measure while investigating the root cause of
  // https://github.com/brave/brave-browser/issues/45288.
  return CreativeNewTabPageAdWallpaperType::kImage;
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
      SCOPED_CRASH_KEY_NUMBER("Issue45288", "wallpaper_type",
                              base::to_underlying(wallpaper_type));
      SCOPED_CRASH_KEY_STRING64("Issue45288", "failure_reason",
                                "Invalid new tab page ad wallpaper type");
      base::debug::DumpWithoutCrashing();

      // Defaulting to image as a fallback for unrecognized wallpaper types.
      // This is a temporary measure while investigating the root cause of
      // https://github.com/brave/brave-browser/issues/45288.
      return kCreativeNewTabPageAdImageWallpaperType;
    }
  }
}

}  // namespace brave_ads
