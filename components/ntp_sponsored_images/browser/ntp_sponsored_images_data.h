/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "ui/gfx/geometry/point.h"

namespace ntp_sponsored_images {

struct Background {
  base::FilePath image_file;
  gfx::Point focal_point;
};

struct NTPSponsoredImagesData {
  NTPSponsoredImagesData();
  NTPSponsoredImagesData(const std::string& photo_json,
                         const base::FilePath& base_dir);
  NTPSponsoredImagesData(const NTPSponsoredImagesData& data);
  NTPSponsoredImagesData& operator=(const NTPSponsoredImagesData& data);
  NTPSponsoredImagesData(NTPSponsoredImagesData&& data);
  ~NTPSponsoredImagesData();

  bool IsValid() const;
  // Generate Value with background image at |index|.
  base::Value GetValueAt(size_t index);

  std::string logo_image_url() const;
  std::vector<std::string> wallpaper_image_urls() const;

  base::FilePath logo_image_file;
  std::string logo_alt_text;
  std::string logo_destination_url;
  std::string logo_company_name;
  std::vector<Background> backgrounds;
  std::string url_prefix;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_DATA_H_
