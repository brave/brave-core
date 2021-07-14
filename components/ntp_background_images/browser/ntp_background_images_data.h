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
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace ntp_background_images {

struct TopSite {
  std::string name;
  std::string destination_url;
  std::string background_color;
  std::string image_path;
  base::FilePath image_file;

  TopSite();
  // For unit test.
  TopSite(const std::string& name, const std::string destination_url,
          const std::string& image_path, const base::FilePath& image_file);
  TopSite(const TopSite& data);
  TopSite& operator=(const TopSite& data);
  ~TopSite();

  bool IsValid() const;
};

struct Logo {
  base::FilePath image_file;
  std::string image_url;
  std::string alt_text;
  std::string destination_url;
  std::string company_name;

  Logo();
  Logo(const Logo&);
  ~Logo();
};

struct Background {
  base::FilePath image_file;
  gfx::Point focal_point;
  std::string background_color;

  std::string creative_instance_id;

  absl::optional<Logo> logo;
  absl::optional<gfx::Rect> viewbox;

  Background();
  // For unit test.
  Background(const base::FilePath& image_file_path, const gfx::Point& point);
  Background(const Background&);

  ~Background();
};

struct NTPBackgroundImagesData {
  NTPBackgroundImagesData();
  NTPBackgroundImagesData(const std::string& json_string,
                          const base::FilePath& installed_dir);
  NTPBackgroundImagesData(const NTPBackgroundImagesData& data);
  NTPBackgroundImagesData& operator=(const NTPBackgroundImagesData& data);
  ~NTPBackgroundImagesData();

  bool IsValid() const;
  // Generate Value with background image at |index|.
  base::Value GetBackgroundAt(size_t index);
  std::vector<TopSite> GetTopSitesForWebUI() const;

  bool IsSuperReferral() const;

  std::string GetURLPrefix() const;

  std::vector<std::string> wallpaper_image_urls() const;

  Logo default_logo;
  std::string theme_name;
  std::vector<Background> backgrounds;
  std::vector<TopSite> top_sites;
  std::string url_prefix;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_
