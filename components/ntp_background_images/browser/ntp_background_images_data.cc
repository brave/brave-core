/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"

#include <optional>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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

Background::Background(const base::FilePath& file_path,
                       const std::string& author_name,
                       const std::string& author_link)
    : file_path(file_path), author(author_name), link(author_link) {}

Background::Background(const Background& other) = default;

Background& Background::operator=(const Background& other) = default;

Background::Background(Background&& other) = default;

Background& Background::operator=(Background&& other) = default;

Background::~Background() = default;

NTPBackgroundImagesData::NTPBackgroundImagesData()
    : url_prefix(absl::StrFormat("%s://%s/",
                                 content::kChromeUIScheme,
                                 kBackgroundWallpaperHost)) {}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const std::string& json_string,
    const base::FilePath& installed_dir)
    : NTPBackgroundImagesData() {
  std::optional<base::Value::Dict> dict = base::JSONReader::ReadDict(
      json_string, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!dict) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }

  const std::optional<int> schema_version = dict->FindInt(kSchemaVersionKey);
  if (schema_version != kExpectedSchemaVersion) {
    return;
  }

  if (const base::Value::List* const list = dict->FindList(kImagesKey)) {
    for (const auto& value : *list) {
      const base::Value::Dict* const image_dict = value.GetIfDict();
      if (!dict) {
        continue;
      }

      const std::string* const image_source =
          image_dict->FindString(kImageSourceKey);
      if (!image_source) {
        continue;
      }

      const std::string* const image_author =
          image_dict->FindString(kImageAuthorKey);
      if (!image_author) {
        continue;
      }

      const std::string* const image_link =
          image_dict->FindString(kImageLinkKey);
      if (!image_link) {
        continue;
      }

      Background background;
      background.file_path = installed_dir.AppendASCII(*image_source);
      background.author = *image_author;
      background.link = *image_link;

      backgrounds.push_back(background);
    }
  }
}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const NTPBackgroundImagesData& other) = default;

NTPBackgroundImagesData& NTPBackgroundImagesData::operator=(
    const NTPBackgroundImagesData& other) = default;

NTPBackgroundImagesData::NTPBackgroundImagesData(
    NTPBackgroundImagesData&& other) = default;

NTPBackgroundImagesData& NTPBackgroundImagesData::operator=(
    NTPBackgroundImagesData&& other) = default;

NTPBackgroundImagesData::~NTPBackgroundImagesData() = default;

bool NTPBackgroundImagesData::IsValid() const {
  return !backgrounds.empty();
}

base::Value::Dict NTPBackgroundImagesData::GetBackgroundAt(size_t index) const {
  DCHECK(index < backgrounds.size());

  if (!IsValid()) {
    return {};
  }

  return base::Value::Dict()
      .Set(kWallpaperURLKey,
           url_prefix + backgrounds[index].file_path.BaseName().AsUTF8Unsafe())
      .Set(kWallpaperFilePathKey, backgrounds[index].file_path.AsUTF8Unsafe())
      .Set(kIsBackgroundKey, true)
      .Set(kImageAuthorKey, backgrounds[index].author)
      .Set(kImageLinkKey, backgrounds[index].link)
      .Set(kWallpaperTypeKey, "brave");
}

}  // namespace ntp_background_images
