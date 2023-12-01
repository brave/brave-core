/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

/* Sample json.
{
  "schemaVersion": 1,
  "images": [
    {
      "name": "ntp-2020/2021-1",
      "source": "background-image-source.png",
      "author": "Brave Software",
      "link": "https://brave.com/",
      "originalUrl": "Contributor sent the hi-res version through email",
      "license": "https://brave.com/about/"
    },
  ]
}
*/

namespace ntp_background_images {

namespace {

constexpr int kExpectedSchemaVersion = 1;

}  // namespace

Background::Background() = default;
Background::Background(const base::FilePath& image_file_path,
                       const std::string& author_name,
                       const std::string& author_link)
    : image_file(image_file_path), author(author_name), link(author_link) {}
Background::Background(const Background&) = default;
Background::~Background() = default;

NTPBackgroundImagesData::NTPBackgroundImagesData()
    : url_prefix(base::StringPrintf("%s://%s/",
                                    content::kChromeUIScheme,
                                    kBackgroundWallpaperHost)) {}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const std::string& json_string,
    const base::FilePath& installed_dir)
    : NTPBackgroundImagesData() {
  std::optional<base::Value> json_value = base::JSONReader::Read(json_string);
  if (!json_value || !json_value->is_dict()) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }
  base::Value::Dict& root = json_value->GetDict();

  std::optional<int> incomingSchemaVersion = root.FindInt(kSchemaVersionKey);
  const bool schemaVersionIsValid = incomingSchemaVersion &&
      *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    DVLOG(2) << __func__ << "Incoming NTP background images data was not valid."
             << " Schema version was "
             << (incomingSchemaVersion ? std::to_string(*incomingSchemaVersion)
                                       : "missing")
             << ", but we expected " << kExpectedSchemaVersion;
    return;
  }

  if (auto* images = root.FindList(kImagesKey)) {
    for (const auto& item : *images) {
      const auto& image = item.GetDict();
      Background background;
      background.image_file =
          installed_dir.AppendASCII(*image.FindString(kImageSourceKey));
      background.author = *image.FindString(kImageAuthorKey);
      background.link = *image.FindString(kImageLinkKey);

      backgrounds.push_back(background);
    }
  }
}

NTPBackgroundImagesData& NTPBackgroundImagesData::operator=(
    const NTPBackgroundImagesData& data) = default;
NTPBackgroundImagesData::NTPBackgroundImagesData(
    const NTPBackgroundImagesData& data) = default;
NTPBackgroundImagesData::~NTPBackgroundImagesData() = default;

bool NTPBackgroundImagesData::IsValid() const {
  return !backgrounds.empty();
}

base::Value::Dict NTPBackgroundImagesData::GetBackgroundAt(size_t index) {
  DCHECK(index >= 0 && index < backgrounds.size());

  // The |data| should be compliant with NewTab.BackgroundWallpaper type from JS
  // side.
  base::Value::Dict data;
  if (!IsValid())
    return data;

  const std::string wallpaper_image_url =
      url_prefix + backgrounds[index].image_file.BaseName().AsUTF8Unsafe();
  // URL is used by NTP WebUI.
  data.Set(kWallpaperImageURLKey, wallpaper_image_url);
  // Path is used by android NTP.
  data.Set(kWallpaperImagePathKey,
           backgrounds[index].image_file.AsUTF8Unsafe());

  data.Set(kIsBackgroundKey, true);
  data.Set(kImageAuthorKey, backgrounds[index].author);
  data.Set(kImageLinkKey, backgrounds[index].link);
  data.Set(kWallpaperTypeKey, "brave");
  return data;
}

}  // namespace ntp_background_images
