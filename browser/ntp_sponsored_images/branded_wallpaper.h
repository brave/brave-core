// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BRANDED_WALLPAPER_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BRANDED_WALLPAPER_H_

#include <memory>
#include <string>
#include <vector>

class BrandedWallpaperLogo {
 public:
  BrandedWallpaperLogo();
  ~BrandedWallpaperLogo();

  std::string imageUrl;
  std::string altText;
  std::string companyName;
  std::string destinationUrl;
};

class BrandedWallpaper {
 public:
  BrandedWallpaper();
  ~BrandedWallpaper();

  std::vector<std::string> wallpaperImageUrls;
  std::unique_ptr<BrandedWallpaperLogo> logo;
};

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BRANDED_WALLPAPER_H_

