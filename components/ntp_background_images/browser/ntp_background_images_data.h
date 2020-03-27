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

struct TopSite {
  std::string name;
  std::string destination_url;
  std::string background_color;
  std::string image_path;
  base::FilePath image_file;

  TopSite();
  TopSite(const std::string& name, const std::string destination_url,
          const std::string& image_path, const base::FilePath& image_file);
  TopSite(const TopSite& data);
  TopSite& operator=(const TopSite& data);
  TopSite(TopSite&& data);
  ~TopSite();

  bool IsValid() const;
};

struct Background {
  base::FilePath image_file;
  gfx::Point focal_point;
};

struct NTPBackgroundImagesData {
  NTPBackgroundImagesData();
  NTPBackgroundImagesData(const std::string& json_string,
                         const base::FilePath& base_dir);
  NTPBackgroundImagesData(const NTPBackgroundImagesData& data);
  NTPBackgroundImagesData& operator=(const NTPBackgroundImagesData& data);
  NTPBackgroundImagesData(NTPBackgroundImagesData&& data);
  ~NTPBackgroundImagesData();

  bool IsValid() const;
  // Generate Value with background image at |index|.
  base::Value GetBackgroundAt(size_t index);
  // Returns empty list value if this data is for sponsored images wallpaper.
  // Use different key string for webui. NTP WebUI uses different key name for
  // top sites values.
  base::Value GetTopSites(bool for_webui = false) const;

  bool IsSuperReferral() const;

  std::string logo_image_url() const;
  std::vector<std::string> wallpaper_image_urls() const;

  std::string theme_name;
  base::FilePath logo_image_file;
  std::string logo_alt_text;
  std::string logo_destination_url;
  std::string logo_company_name;
  std::vector<Background> backgrounds;
  std::vector<TopSite> top_sites;
  std::string url_prefix;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_
