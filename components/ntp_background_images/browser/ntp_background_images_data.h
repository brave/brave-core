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

struct Background {
  base::FilePath image_file;

  std::string author;
  std::string link;

  Background();
  // For unit test.
  Background(const base::FilePath& image_file_path,
             const std::string& author,
             const std::string& link);
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
  base::Value::Dict GetBackgroundAt(size_t index);

  std::vector<Background> backgrounds;
  std::string url_prefix;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_DATA_H_
