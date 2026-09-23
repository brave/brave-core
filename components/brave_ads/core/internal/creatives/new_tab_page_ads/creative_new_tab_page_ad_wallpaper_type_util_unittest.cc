/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_util.h"

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeNewTabPageAdWallpaperTypeUtilTest,
     StringToCreativeNewTabPageAdImageWallpaperType) {
  // Act & Assert
  EXPECT_EQ(CreativeNewTabPageAdWallpaperType::kImage,
            ToCreativeNewTabPageAdWallpaperType("image"));
}

TEST(BraveAdsCreativeNewTabPageAdWallpaperTypeUtilTest,
     StringToCreativeNewTabPageAdWallpaperType) {
  // Act & Assert
  EXPECT_EQ(CreativeNewTabPageAdWallpaperType::kRichMedia,
            ToCreativeNewTabPageAdWallpaperType("richMedia"));
}

TEST(BraveAdsCreativeNewTabPageAdWallpaperTypeUtilTest,
     CreativeNewTabPageAdImageWallpaperTypeToString) {
  // Act & Assert
  EXPECT_EQ("image", ToString(CreativeNewTabPageAdWallpaperType::kImage));
}

TEST(BraveAdsCreativeNewTabPageAdWallpaperTypeUtilTest,
     CreativeNewTabPageAdRichMediaWallpaperTypeToString) {
  // Act & Assert
  EXPECT_EQ("richMedia",
            ToString(CreativeNewTabPageAdWallpaperType::kRichMedia));
}

}  // namespace brave_ads
