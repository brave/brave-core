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
constexpr char kThemeNamePath[] = "themeName";
constexpr char kLogoImageURLPath[] = "logo.imageUrl";
constexpr char kLogoAltPath[] = "logo.alt";
constexpr char kLogoCompanyNamePath[] = "logo.companyName";
constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
constexpr char kWallpapersPath[] = "wallpapers";
constexpr char kWallpaperImageURLPath[] = "imageUrl";
constexpr char kWallpaperFocalPointXPath[] = "focalPoint.x";
constexpr char kWallpaperFocalPointYPath[] = "focalPoint.y";
constexpr char kTopSitesPath[] = "topSites";
constexpr char kTopSiteNamePath[] = "name";
constexpr char kTopSiteDestinationURLPath[] = "destinationUrl";
constexpr char kTopSiteBackgroundColorPath[] = "backgroundColor";
constexpr char kTopSiteIconURLPath[] = "iconUrl";
constexpr char kSchemaVersionPath[] = "schemaVersion";

constexpr int kExpectedSchemaVersion = 1;

const std::string default_url_prefix =  // NOLINT
    base::StringPrintf("%s://%s/",
                       content::kChromeUIScheme,
                       kBrandedWallpaperHost);
}  // namespace

TopSite::TopSite() = default;
TopSite::TopSite(
    const std::string& i_name, const std::string i_destination_url,
    const std::string& i_image_path, const base::FilePath& i_image_file)
    : name(i_name), destination_url(i_destination_url),
      image_path(i_image_path), image_file(i_image_file) {}

TopSite& TopSite::operator=(const TopSite& data) = default;
TopSite::TopSite(const TopSite& data) = default;
TopSite::TopSite(TopSite&& data) = default;
TopSite::~TopSite() = default;

bool TopSite::IsValid() const {
  return !name.empty() && !destination_url.empty() && !image_file.empty();
}

NTPBackgroundImagesData::NTPBackgroundImagesData()
    : url_prefix(default_url_prefix) {}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const std::string& json_string,
    const base::FilePath& base_dir)
    : NTPBackgroundImagesData() {
  base::Optional<base::Value> json_value = base::JSONReader::Read(json_string);
  if (!json_value) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }

  base::Optional<int> incomingSchemaVersion =
      json_value->FindIntPath(kSchemaVersionPath);
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

  if (auto* name = json_value->FindStringPath(kThemeNamePath)) {
    theme_name = *name;
    DVLOG(2) << __func__ << ": Theme name: " << theme_name;
  }

  if (auto* url = json_value->FindStringPath(kLogoImageURLPath))
    logo_image_file = base_dir.AppendASCII(*url);

  if (auto* alt_text = json_value->FindStringPath(kLogoAltPath))
    logo_alt_text = *alt_text;

  if (auto* name = json_value->FindStringPath(kLogoCompanyNamePath))
    logo_company_name = *name;

  if (auto* url = json_value->FindStringPath(kLogoDestinationURLPath)) {
    logo_destination_url = *url;
  }

  if (auto* wallpapers = json_value->FindListPath(kWallpapersPath)) {
    for (const auto& wallpaper : wallpapers->GetList()) {
      backgrounds.push_back({
        base_dir.AppendASCII(
            *wallpaper.FindStringPath(kWallpaperImageURLPath)),
        { wallpaper.FindIntPath(kWallpaperFocalPointXPath).value_or(0),
          wallpaper.FindIntPath(kWallpaperFocalPointYPath).value_or(0) }
      });
    }
  }

  if (auto* sites = json_value->FindListPath(kTopSitesPath)) {
    for (const auto& top_site_value : sites->GetList()) {
      TopSite site;
      if (auto* name = top_site_value.FindStringPath(kTopSiteNamePath))
        site.name = *name;

      if (auto* url = top_site_value.FindStringPath(kTopSiteDestinationURLPath))
        site.destination_url = *url;

      if (auto* color =
              top_site_value.FindStringPath(kTopSiteBackgroundColorPath))
        site.background_color = *color;

      if (auto* url = top_site_value.FindStringPath(kTopSiteIconURLPath)) {
        site.image_path = *url;
        site.image_file = base_dir.AppendASCII(*url);
      }

      // TopSite should have all properties.
      DCHECK(site.IsValid());
      top_sites.push_back(site);
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

bool NTPBackgroundImagesData::IsSuperReferral() const {
  return IsValid() && !top_sites.empty();
}

base::Value NTPBackgroundImagesData::GetBackgroundAt(size_t index) {
  DCHECK(index >= 0 && index < wallpaper_image_urls().size());

  base::Value data(base::Value::Type::DICTIONARY);
  if (!IsValid())
    return data;

  if (!theme_name.empty()) {
    DCHECK(IsSuperReferral());
    data.SetStringKey("themeName", theme_name);
  }

  data.SetBoolKey("isSponsored", !IsSuperReferral());
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

base::Value NTPBackgroundImagesData::GetTopSites() const {
  base::Value top_sites_list_value(base::Value::Type::LIST);
  for (const auto& top_site : top_sites) {
    base::Value top_site_value(base::Value::Type::DICTIONARY);
    top_site_value.SetStringKey("name", top_site.name);
    top_site_value.SetStringKey("destinationUrl", top_site.destination_url);
    top_site_value.SetStringKey("backgroundColor", top_site.background_color);
    top_site_value.SetStringKey("iconUrl", url_prefix + top_site.image_path);
    top_sites_list_value.Append(std::move(top_site_value));
  }
  return top_sites_list_value;
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
