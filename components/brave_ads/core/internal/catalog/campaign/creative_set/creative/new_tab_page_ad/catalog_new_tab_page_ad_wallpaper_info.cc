/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"

namespace brave_ads {

CatalogNewTabPageAdWallpaperInfo::CatalogNewTabPageAdWallpaperInfo() = default;

CatalogNewTabPageAdWallpaperInfo::CatalogNewTabPageAdWallpaperInfo(
    const CatalogNewTabPageAdWallpaperInfo& other) = default;

CatalogNewTabPageAdWallpaperInfo& CatalogNewTabPageAdWallpaperInfo::operator=(
    const CatalogNewTabPageAdWallpaperInfo& other) = default;

CatalogNewTabPageAdWallpaperInfo::CatalogNewTabPageAdWallpaperInfo(
    CatalogNewTabPageAdWallpaperInfo&& other) noexcept = default;

CatalogNewTabPageAdWallpaperInfo& CatalogNewTabPageAdWallpaperInfo::operator=(
    CatalogNewTabPageAdWallpaperInfo&& other) noexcept = default;

CatalogNewTabPageAdWallpaperInfo::~CatalogNewTabPageAdWallpaperInfo() = default;

}  // namespace brave_ads
