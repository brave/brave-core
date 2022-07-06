/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_wallpaper_focal_point_info.h"

#include <tuple>

namespace ads {

NewTabPageAdWallpaperFocalPointInfo::NewTabPageAdWallpaperFocalPointInfo() =
    default;

NewTabPageAdWallpaperFocalPointInfo::NewTabPageAdWallpaperFocalPointInfo(
    const NewTabPageAdWallpaperFocalPointInfo& info) = default;

NewTabPageAdWallpaperFocalPointInfo&
NewTabPageAdWallpaperFocalPointInfo::operator=(
    const NewTabPageAdWallpaperFocalPointInfo& info) = default;

NewTabPageAdWallpaperFocalPointInfo::~NewTabPageAdWallpaperFocalPointInfo() =
    default;

bool NewTabPageAdWallpaperFocalPointInfo::operator==(
    const NewTabPageAdWallpaperFocalPointInfo& rhs) const {
  auto tie = [](const NewTabPageAdWallpaperFocalPointInfo& info) {
    return std::tie(info.x, info.y);
  };

  return tie(*this) == tie(rhs);
}

}  // namespace ads
