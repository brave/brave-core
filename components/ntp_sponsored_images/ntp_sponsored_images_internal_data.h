/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_INTERNAL_DATA_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_INTERNAL_DATA_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"

struct NTPSponsoredImagesInternalData {
  NTPSponsoredImagesInternalData();
  NTPSponsoredImagesInternalData(const NTPSponsoredImagesInternalData& data);
  ~NTPSponsoredImagesInternalData();

  void Print() const;

  base::FilePath logo_image_file;
  std::string logo_alt_text;
  std::string logo_destination_url;
  std::string logo_company_name;

  std::vector<base::FilePath> wallpaper_image_files;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_INTERNAL_DATA_H_
