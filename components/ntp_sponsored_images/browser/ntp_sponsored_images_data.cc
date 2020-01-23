/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_internal_data.h"
#include "brave/components/ntp_sponsored_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

NTPSponsoredImagesData::NTPSponsoredImagesData() = default;

NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesInternalData& internal_data) {
  logo_alt_text = internal_data.logo_alt_text;
  logo_destination_url = internal_data.logo_destination_url;
  logo_company_name = internal_data.logo_company_name;
  const int wallpaper_image_count = internal_data.wallpaper_image_files.size();
  const std::string url_prefix = base::StringPrintf("%s://%s/",
      content::kChromeUIScheme, kBrandedWallpaperHost);
  logo_image_url = url_prefix + kLogoPath;
  for (int i = 0; i < wallpaper_image_count; i++) {
    const std::string wallpaper_image_url = url_prefix + base::StringPrintf(
        "%s%d.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) {
  logo_image_url = data.logo_image_url;
  logo_alt_text = data.logo_alt_text;
  logo_destination_url = data.logo_destination_url;
  logo_company_name = data.logo_company_name;
  wallpaper_image_urls = data.wallpaper_image_urls;
  return *this;
}

NTPSponsoredImagesData::NTPSponsoredImagesData(
    NTPSponsoredImagesData&& data) = default;

NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

bool NTPSponsoredImagesData::IsValid() const {
  LOG(ERROR) << wallpaper_image_urls.size();
  return wallpaper_image_urls.size() > 0;
}
