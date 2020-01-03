/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_data.h"

#include "base/logging.h"

NTPSponsoredImagesData::NTPSponsoredImagesData() = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

void NTPSponsoredImagesData::Print() const {
  LOG(ERROR) << "Logo url: " << logo_image_url;
  LOG(ERROR) << "Logo alt text: " << logo_alt_text;
  LOG(ERROR) << "Logo destination url: " << logo_destination_url;
  LOG(ERROR) << "Logo company name: " << logo_company_name;

  for (const auto& url : wallpaper_image_urls)
    LOG(ERROR) << "Wallpaper image urls: " << url;
}
