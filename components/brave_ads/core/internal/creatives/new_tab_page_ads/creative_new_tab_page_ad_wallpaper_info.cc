/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"

namespace brave_ads {

CreativeNewTabPageAdWallpaperInfo::CreativeNewTabPageAdWallpaperInfo() =
    default;

CreativeNewTabPageAdWallpaperInfo::CreativeNewTabPageAdWallpaperInfo(
    const CreativeNewTabPageAdWallpaperInfo& other) = default;

CreativeNewTabPageAdWallpaperInfo& CreativeNewTabPageAdWallpaperInfo::operator=(
    const CreativeNewTabPageAdWallpaperInfo& other) = default;

CreativeNewTabPageAdWallpaperInfo::CreativeNewTabPageAdWallpaperInfo(
    CreativeNewTabPageAdWallpaperInfo&& other) noexcept = default;

CreativeNewTabPageAdWallpaperInfo& CreativeNewTabPageAdWallpaperInfo::operator=(
    CreativeNewTabPageAdWallpaperInfo&& other) noexcept = default;

CreativeNewTabPageAdWallpaperInfo::~CreativeNewTabPageAdWallpaperInfo() =
    default;

}  // namespace brave_ads
