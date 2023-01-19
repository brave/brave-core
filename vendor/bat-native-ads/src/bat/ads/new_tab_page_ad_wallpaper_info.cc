/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_wallpaper_info.h"

namespace ads {

bool operator==(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs) {
  return lhs.image_url == rhs.image_url && lhs.focal_point == rhs.focal_point;
}

bool operator!=(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
