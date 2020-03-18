/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

namespace ntp_background_images {

namespace {
constexpr char kLogoImageURLPath[] = "logo.imageUrl";
constexpr char kLogoAltPath[] = "logo.alt";
constexpr char kLogoCompanyNamePath[] = "logo.companyName";
constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
constexpr char kWallpapersPath[] = "wallpapers";
constexpr char kWallpaperImageURLPath[] = "imageUrl";
constexpr char kWallpaperFocalPointXPath[] = "focalPoint.x";
constexpr char kWallpaperFocalPointYPath[] = "focalPoint.y";
constexpr char kSchemaVersionPath[] = "schemaVersion";

constexpr int kExpectedSchemaVersion = 1;

const std::string default_url_prefix =  // NOLINT
    base::StringPrintf("%s://%s/",
                       content::kChromeUIScheme,
                       kBrandedWallpaperHost);
}  // namespace

NTPBackgroundImagesData::NTPBackgroundImagesData()
    : url_prefix(default_url_prefix) {}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const std::string& photo_json,
    const base::FilePath& base_dir)
    : NTPBackgroundImagesData() {
  base::Optional<base::Value> photo_value = base::JSONReader::Read(photo_json);
  if (!photo_value)
    return;

  base::Optional<int> incomingSchemaVersion =
      photo_value->FindIntPath(kSchemaVersionPath);
  const bool schemaVersionIsValid = incomingSchemaVersion &&
      *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    VLOG(2) << "Incoming NTP Background images data was not valid."
            << " Schema version was "
            << (incomingSchemaVersion ? std::to_string(*incomingSchemaVersion)
                                      : "missing")
            << ", but we expected " << kExpectedSchemaVersion;
    return;
  }

  if (auto* url = photo_value->FindStringPath(kLogoImageURLPath))
    logo_image_file = base_dir.AppendASCII(*url);

  if (auto* alt_text = photo_value->FindStringPath(kLogoAltPath))
    logo_alt_text = *alt_text;

  if (auto* name = photo_value->FindStringPath(kLogoCompanyNamePath))
    logo_company_name = *name;

  if (auto* url = photo_value->FindStringPath(kLogoDestinationURLPath)) {
    logo_destination_url = *url;
  }

  if (auto* wallpapers = photo_value->FindListPath(kWallpapersPath)) {
    for (const auto& wallpaper : wallpapers->GetList()) {
      backgrounds.push_back({
        base_dir.AppendASCII(
            *wallpaper.FindStringPath(kWallpaperImageURLPath)),
        { wallpaper.FindIntPath(kWallpaperFocalPointXPath).value_or(0),
          wallpaper.FindIntPath(kWallpaperFocalPointYPath).value_or(0) }
      });
    }
  }
}

NTPBackgroundImagesData& NTPBackgroundImagesData::operator=(
    const NTPBackgroundImagesData& data) = default;
NTPBackgroundImagesData::NTPBackgroundImagesData(
    NTPBackgroundImagesData&& data) = default;
NTPBackgroundImagesData::NTPBackgroundImagesData(
    const NTPBackgroundImagesData& data) = default;
NTPBackgroundImagesData::~NTPBackgroundImagesData() = default;

bool NTPBackgroundImagesData::IsValid() const {
  return (!backgrounds.empty() && !logo_destination_url.empty());
}

base::Value NTPBackgroundImagesData::GetValueAt(size_t index) {
  DCHECK(index >= 0 && index < wallpaper_image_urls().size());

  base::Value data(base::Value::Type::DICTIONARY);
  if (!IsValid())
    return data;

  data.SetBoolKey("isSponsorship", true);
  data.SetStringKey("wallpaperImageUrl",
                    wallpaper_image_urls()[index]);
  data.SetStringKey("wallpaperImagePath",
                    backgrounds[index].image_file.AsUTF8Unsafe());
  base::Value logo_data(base::Value::Type::DICTIONARY);
  logo_data.SetStringKey("image", logo_image_url());
  logo_data.SetStringKey("imagePath", logo_image_file.AsUTF8Unsafe());
  logo_data.SetStringKey("companyName", logo_company_name);
  logo_data.SetStringKey("alt", logo_alt_text);
  logo_data.SetStringKey("destinationUrl", logo_destination_url);
  data.SetKey("logo", std::move(logo_data));
  return data;
}

std::string NTPBackgroundImagesData::logo_image_url() const {
  return url_prefix + kLogoPath;
}

std::vector<std::string> NTPBackgroundImagesData::wallpaper_image_urls() const {
  std::vector<std::string> wallpaper_image_urls;
  for (size_t i = 0; i < backgrounds.size(); i++) {
    const std::string wallpaper_image_url = url_prefix + base::StringPrintf(
        "%s%zu.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
  return wallpaper_image_urls;
}

}  // namespace ntp_background_images
