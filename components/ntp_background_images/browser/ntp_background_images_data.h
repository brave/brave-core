/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "ui/gfx/geometry/point.h"

namespace ntp_background_images {

struct Background {
  base::FilePath image_file;
  gfx::Point focal_point;
};

struct NTPBackgroundImagesData {
  NTPBackgroundImagesData();
  NTPBackgroundImagesData(const std::string& photo_json,
                         const base::FilePath& base_dir);
  NTPBackgroundImagesData(const NTPBackgroundImagesData& data);
  NTPBackgroundImagesData& operator=(const NTPBackgroundImagesData& data);
  NTPBackgroundImagesData(NTPBackgroundImagesData&& data);
  ~NTPBackgroundImagesData();

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

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_
