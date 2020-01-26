/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"

#include "base/strings/stringprintf.h"
#include "brave/components/ntp_sponsored_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

namespace ntp_sponsored_images {

namespace {
const std::string default_url_prefix =  // NOLINT
    base::StringPrintf("%s://%s/",
                       content::kChromeUIScheme,
                       kBrandedWallpaperHost);
}  // namespace

NTPSponsoredImagesData::NTPSponsoredImagesData()
    : url_prefix(default_url_prefix) {}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    NTPSponsoredImagesData&& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

bool NTPSponsoredImagesData::IsValid() const {
  return wallpaper_image_files.size() > 0;
}

std::string NTPSponsoredImagesData::logo_image_url() const {
  return url_prefix + kLogoPath;
}

std::vector<std::string> NTPSponsoredImagesData::wallpaper_image_urls() const {
  std::vector<std::string> wallpaper_image_urls;
  for (size_t i = 0; i < wallpaper_image_files.size(); i++) {
    const std::string wallpaper_image_url = url_prefix + base::StringPrintf(
        "%s%zu.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
  return wallpaper_image_urls;
}

}  // namespace ntp_sponsored_images
