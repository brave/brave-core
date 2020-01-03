/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_DATA_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_DATA_H_

#include <string>
#include <vector>

#include "base/component_export.h"

struct COMPONENT_EXPORT(BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES)
NTPSponsoredImagesData {
  NTPSponsoredImagesData();
  NTPSponsoredImagesData(const NTPSponsoredImagesData& data);
  ~NTPSponsoredImagesData();

  void Print() const;

  std::string logo_image_url;
  std::string logo_alt_text;
  std::string logo_destination_url;
  std::string logo_company_name;

  std::vector<std::string> wallpaper_image_urls;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_DATA_H_
