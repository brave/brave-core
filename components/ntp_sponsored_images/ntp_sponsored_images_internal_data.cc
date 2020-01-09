/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_internal_data.h"

#include "base/logging.h"

NTPSponsoredImagesInternalData::NTPSponsoredImagesInternalData() = default;
NTPSponsoredImagesInternalData::NTPSponsoredImagesInternalData(
    const NTPSponsoredImagesInternalData& data) = default;
NTPSponsoredImagesInternalData::~NTPSponsoredImagesInternalData() = default;

void NTPSponsoredImagesInternalData::Print() const {
  LOG(ERROR) << "Logo url: " << logo_image_file.value();
  LOG(ERROR) << "Logo alt text: " << logo_alt_text;
  LOG(ERROR) << "Logo destination url: " << logo_destination_url;
  LOG(ERROR) << "Logo company name: " << logo_company_name;

  for (const auto& file : wallpaper_image_files)
    LOG(ERROR) << "Wallpaper image urls: " << file.value();
}
