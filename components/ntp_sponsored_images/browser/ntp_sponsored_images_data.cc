/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"

#include "base/json/json_reader.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ntp_sponsored_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

namespace ntp_sponsored_images {

namespace {
constexpr char kLogoImageURLPath[] = "logo.imageUrl";
constexpr char kLogoAltPath[] = "logo.alt";
constexpr char kLogoCompanyNamePath[] = "logo.companyName";
constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
constexpr char kWallpapersPath[] = "wallpapers";
constexpr char kWallpaperImageURLPath[] = "imageUrl";
constexpr char kSchemaVersionPath[] = "schemaVersion";

constexpr int kExpectedSchemaVersion = 1;

const std::string default_url_prefix =  // NOLINT
    base::StringPrintf("%s://%s/",
                       content::kChromeUIScheme,
                       kSponsoredWallpaperHost);
}  // namespace

NTPSponsoredImagesData::NTPSponsoredImagesData()
    : url_prefix(default_url_prefix) {}

NTPSponsoredImagesData::NTPSponsoredImagesData(
    const std::string& photo_json,
    const base::FilePath& base_dir)
    : NTPSponsoredImagesData() {
  base::Optional<base::Value> photo_value = base::JSONReader::Read(photo_json);
  if (!photo_value)
    return;

  base::Optional<int> incomingSchemaVersion =
      photo_value->FindIntPath(kSchemaVersionPath);
  const bool schemaVersionIsValid = incomingSchemaVersion &&
      *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    LOG(ERROR) << "Incoming NTP Sponsored images component data was not valid."
               << " Schema version was "
               << (incomingSchemaVersion ?
                       std::to_string(*incomingSchemaVersion) : "missing")
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
      wallpaper_image_files.push_back(base_dir.AppendASCII(
          *wallpaper.FindStringPath(kWallpaperImageURLPath)));
    }
  }
}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    NTPSponsoredImagesData&& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

bool NTPSponsoredImagesData::IsValid() const {
  return (!wallpaper_image_files.empty() && !logo_destination_url.empty());
}

base::Value NTPSponsoredImagesData::GetValueAt(size_t index) {
  DCHECK(index >= 0 && index < wallpaper_image_urls().size());

  base::Value data(base::Value::Type::DICTIONARY);
  if (!IsValid())
    return data;

  data.SetBoolKey("isSponsorship", true);
  data.SetStringKey("wallpaperImageUrl",
                    wallpaper_image_urls()[index]);
  base::Value logo_data(base::Value::Type::DICTIONARY);
  logo_data.SetStringKey("image", logo_image_url());
  logo_data.SetStringKey("companyName", logo_company_name);
  logo_data.SetStringKey("alt", logo_alt_text);
  logo_data.SetStringKey("destinationUrl", logo_destination_url);
  data.SetKey("logo", std::move(logo_data));
  return data;
}

std::string NTPSponsoredImagesData::logo_image_url() const {
  return url_prefix + kLogoPath;
}

std::vector<std::string> NTPSponsoredImagesData::wallpaper_image_urls() const {
  std::vector<std::string> wallpaper_image_urls;
  for (size_t i = 0; i < wallpaper_image_files.size(); i++) {
    const std::string wallpaper_image_url = url_prefix + base::StringPrintf(
        "%s%zu.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
  return wallpaper_image_urls;
}

}  // namespace ntp_sponsored_images
