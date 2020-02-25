/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_data.h"

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
constexpr char kTopSitesPath[] = "topSites";
constexpr char kTopSiteNamePath[] = "name";
constexpr char kTopSiteDestinationURLPath[] = "destinationUrl";
constexpr char kTopSiteIconURLPath[] = "iconUrl";
constexpr char kSchemaVersionPath[] = "schemaVersion";

constexpr int kExpectedSchemaVersion = 1;

const std::string kDefaultURLPrefix =
    base::StringPrintf("%s://%s/",
                       content::kChromeUIScheme,
                       kReferralWallpaperHost);
}  // namespace

TopSite::TopSite() : url_prefix(kDefaultURLPrefix) {}

TopSite& TopSite::operator=(const TopSite& data) = default;
TopSite::TopSite(const TopSite& data) = default;
TopSite::TopSite(TopSite&& data) = default;
TopSite::~TopSite() = default;
std::string TopSite::icon_image_url() const {
  return url_prefix + icon_image_file.BaseName().AsUTF8Unsafe();
}

NTPReferralImagesData::NTPReferralImagesData()
    : url_prefix(kDefaultURLPrefix) {}

NTPReferralImagesData::NTPReferralImagesData(
    const std::string& data_json,
    const base::FilePath& base_dir)
    : NTPReferralImagesData() {
  base::Optional<base::Value> data_value = base::JSONReader::Read(data_json);
  if (!data_value)
    return;

  base::Optional<int> incomingSchemaVersion =
      data_value->FindIntPath(kSchemaVersionPath);
  const bool schemaVersionIsValid = incomingSchemaVersion &&
      *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    DVLOG(2) << "Incoming NTP Rreferral images component data was not valid."
             << " Schema version was "
             << (incomingSchemaVersion ?
                     std::to_string(*incomingSchemaVersion) : "missing")
             << ", but we expected " << kExpectedSchemaVersion;
    return;
  }

  if (auto* url = data_value->FindStringPath(kLogoImageURLPath))
    logo_image_file = base_dir.AppendASCII(*url);

  if (auto* alt_text = data_value->FindStringPath(kLogoAltPath))
    logo_alt_text = *alt_text;

  if (auto* company_name = data_value->FindStringPath(kLogoCompanyNamePath))
    logo_company_name = *company_name;

  if (auto* url = data_value->FindStringPath(kLogoDestinationURLPath))
    logo_destination_url = *url;

  if (auto* wallpapers = data_value->FindListPath(kWallpapersPath)) {
    for (const auto& wallpaper : wallpapers->GetList()) {
      wallpaper_image_files.push_back(base_dir.AppendASCII(
          *wallpaper.FindStringPath(kWallpaperImageURLPath)));
    }
  }

  if (auto* sites = data_value->FindListPath(kTopSitesPath)) {
    for (const auto& top_site_value : sites->GetList()) {
      TopSite site;
      // Add top site only if it has all properties.
      if (auto* name = top_site_value.FindStringPath(kTopSiteNamePath))
        site.name = *name;
      else
        continue;

      if (auto* url = top_site_value.FindStringPath(kTopSiteDestinationURLPath))
        site.destination_url = *url;
      else
        continue;

      if (auto* url = top_site_value.FindStringPath(kTopSiteIconURLPath))
        site.icon_image_file = base_dir.AppendASCII(*url);
      else
        continue;

      top_sites.push_back(site);
    }
  }
}

NTPReferralImagesData& NTPReferralImagesData::operator=(
    const NTPReferralImagesData& data) = default;
NTPReferralImagesData::NTPReferralImagesData(
    NTPReferralImagesData&& data) = default;
NTPReferralImagesData::NTPReferralImagesData(
    const NTPReferralImagesData& data) = default;
NTPReferralImagesData::~NTPReferralImagesData() = default;

bool NTPReferralImagesData::IsValid() const {
  return (!wallpaper_image_files.empty() &&
          !logo_destination_url.empty() &&
          !top_sites.empty());
}

base::Value NTPReferralImagesData::GetValueAt(size_t index) {
  DCHECK(index >= 0 && index < wallpaper_image_urls().size());

  base::Value data(base::Value::Type::DICTIONARY);
  if (!IsValid())
    return data;

  data.SetBoolKey("isSponsorship", false);
  data.SetStringKey("wallpaperImageUrl",
                    wallpaper_image_urls()[index]);
  base::Value logo_data(base::Value::Type::DICTIONARY);
  logo_data.SetStringKey("image", logo_image_url());
  logo_data.SetStringKey("companyName", logo_company_name);
  logo_data.SetStringKey("alt", logo_alt_text);
  logo_data.SetStringKey("destinationUrl", logo_destination_url);
  data.SetKey("logo", std::move(logo_data));
  base::Value top_sites_list_value(base::Value::Type::LIST);
  for (const auto& top_site : top_sites) {
    base::Value top_site_value(base::Value::Type::DICTIONARY);
    top_site_value.SetStringKey("name", top_site.name);
    top_site_value.SetStringKey("destinationUrl", top_site.destination_url);
    top_site_value.SetStringKey("iconUrl", top_site.icon_image_url());
    top_sites_list_value.GetList().push_back(std::move(top_site_value));
  }
  data.SetKey("topSites", std::move(top_sites_list_value));
  return data;
}

std::string NTPReferralImagesData::logo_image_url() const {
  return url_prefix + kLogoPath;
}

std::vector<std::string> NTPReferralImagesData::wallpaper_image_urls() const {
  std::vector<std::string> wallpaper_image_urls;
  for (size_t i = 0; i < wallpaper_image_files.size(); i++) {
    const std::string wallpaper_image_url = url_prefix + base::StringPrintf(
        "%s%zu.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
  return wallpaper_image_urls;
}

}  // namespace ntp_sponsored_images
