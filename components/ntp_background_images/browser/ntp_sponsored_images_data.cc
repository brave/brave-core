/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

/* Sample photo.json.
{
  "schemaVersion": 1,
  "logo": {
    "imageUrl": "logo.png",
    "alt": "Visit Brave Software",
    "companyName": "Brave Software",
    "destinationUrl": "https://www.brave.com/"
  },
  "wallpapers": [
    {
      "imageUrl": "background-1.jpg",
      "focalPoint": {
        "x": 1468,
        "y": 720
      }
    },
    {
      "imageUrl": "background-2.jpg",
      "focalPoint": {
        "x": 1650,
        "y": 720
      },
      "viewbox": {
        "x": 1578,
        "y": 1200,
        "height": 600,
        "width": 800
      },
      "backgroundColor": "#FFFFFF",
      "creativeInstanceId": "3e47ee7a-8d2d-445b-8e60-d987fdeea613",
      "logo": {
        "imageUrl": "logo-2.png",
        "alt": "basic attention token",
        "companyName": "BAT",
        "destinationUrl": "https://basicattentiontoken.org/"
      }
    }
  ]
*/

namespace ntp_background_images {

namespace {

constexpr int kExpectedSchemaVersion = 1;

Logo GetLogoFromValue(const base::FilePath& installed_dir,
                      const std::string& url_prefix,
                      const base::Value* value) {
  DCHECK(value && value->is_dict());
  Logo logo;

  if (auto* url = value->FindStringKey(kImageURLKey)) {
    logo.image_file = installed_dir.AppendASCII(*url);
    logo.image_url = url_prefix + *url;
  }

  if (auto* alt_text = value->FindStringKey(kAltKey))
    logo.alt_text = *alt_text;

  if (auto* name = value->FindStringKey(kCompanyNameKey))
    logo.company_name = *name;

  if (auto* url = value->FindStringKey(kDestinationURLKey))
    logo.destination_url = *url;

  return logo;
}

}  // namespace

TopSite::TopSite() = default;
TopSite::TopSite(const std::string& i_name,
                 const std::string i_destination_url,
                 const std::string& i_image_path,
                 const base::FilePath& i_image_file)
    : name(i_name),
      destination_url(i_destination_url),
      image_path(i_image_path),
      image_file(i_image_file) {}
TopSite& TopSite::operator=(const TopSite& data) = default;
TopSite::TopSite(const TopSite& data) = default;
TopSite::~TopSite() = default;

bool TopSite::IsValid() const {
  return !name.empty() && !destination_url.empty() && !image_file.empty();
}

Logo::Logo() = default;
Logo::Logo(const Logo&) = default;
Logo::~Logo() = default;

SponsoredBackground::SponsoredBackground() = default;
SponsoredBackground::SponsoredBackground(const base::FilePath& image_file_path,
                                         const gfx::Point& point,
                                         const Logo& test_logo)
    : image_file(image_file_path), focal_point(point), logo(test_logo) {}
SponsoredBackground::SponsoredBackground(const SponsoredBackground&) = default;
SponsoredBackground::~SponsoredBackground() = default;

Campaign::Campaign() = default;
Campaign::~Campaign() = default;
Campaign::Campaign(const Campaign&) = default;
Campaign& Campaign::operator=(const Campaign&) = default;

bool Campaign::IsValid() const {
  return !backgrounds.empty();
}

NTPSponsoredImagesData::NTPSponsoredImagesData() = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const std::string& json_string,
    const base::FilePath& installed_dir)
    : NTPSponsoredImagesData() {
  absl::optional<base::Value> json_value = base::JSONReader::Read(json_string);
  if (!json_value) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }

  absl::optional<int> incomingSchemaVersion =
      json_value->FindIntKey(kSchemaVersionKey);
  const bool schemaVersionIsValid =
      incomingSchemaVersion && *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    DVLOG(2) << __func__ << "Incoming NTP background images data was not valid."
             << " Schema version was "
             << (incomingSchemaVersion ? std::to_string(*incomingSchemaVersion)
                                       : "missing")
             << ", but we expected " << kExpectedSchemaVersion;
    return;
  }

  url_prefix = base::StringPrintf("%s://%s/", content::kChromeUIScheme,
                                  kBrandedWallpaperHost);
  if (auto* name = json_value->FindStringKey(kThemeNameKey)) {
    theme_name = *name;
    url_prefix += kSuperReferralPath;
  } else {
    url_prefix += kSponsoredImagesPath;
  }

  auto* campaigns_value = json_value->FindListKey(kCampaignsKey);
  if (campaigns_value) {
    ParseCampaignsList(*campaigns_value, installed_dir);
  } else {
    // Get a global campaign directly if the campaign list doesn't exist.
    const auto campaign = GetCampaignFromValue(*json_value, installed_dir);
    if (campaign.IsValid())
      campaigns.push_back(campaign);
  }

  ParseSRProperties(*json_value, installed_dir);

  PrintCampaignsParsingResult();
}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

void NTPSponsoredImagesData::ParseCampaignsList(
    const base::Value& campaigns_value,
    const base::FilePath& installed_dir) {
  DCHECK(campaigns_value.is_list());
  for (const auto& campaign_value : campaigns_value.GetListDeprecated()) {
    const auto campaign = GetCampaignFromValue(campaign_value, installed_dir);
    if (campaign.IsValid())
      campaigns.push_back(campaign);
  }
}

Campaign NTPSponsoredImagesData::GetCampaignFromValue(
    const base::Value& value,
    const base::FilePath& installed_dir) {
  DCHECK(value.is_dict());

  Campaign campaign;

  Logo default_logo;
  if (auto* logo = value.FindDictKey(kLogoKey)) {
    default_logo = GetLogoFromValue(installed_dir, url_prefix, logo);
  }

  if (auto* wallpapers = value.FindListKey(kWallpapersKey)) {
    const int wallpaper_count = wallpapers->GetListDeprecated().size();
    for (int i = 0; i < wallpaper_count; ++i) {
      const auto& wallpaper = wallpapers->GetListDeprecated()[i];
      SponsoredBackground background;
      background.image_file =
          installed_dir.AppendASCII(*wallpaper.FindStringKey(kImageURLKey));

      if (auto* focal_point = wallpaper.FindDictKey(kWallpaperFocalPointKey)) {
        background.focal_point = {focal_point->FindIntKey(kXKey).value_or(0),
                                  focal_point->FindIntKey(kYKey).value_or(0)};
      }

      if (auto* viewbox = wallpaper.FindDictKey(kViewboxKey)) {
        gfx::Rect rect(viewbox->FindIntKey(kXKey).value_or(0),
                       viewbox->FindIntKey(kYKey).value_or(0),
                       viewbox->FindIntKey(kWidthKey).value_or(0),
                       viewbox->FindIntKey(kHeightKey).value_or(0));
        background.viewbox.emplace(rect);
      }
      if (auto* background_color = wallpaper.FindStringKey(kBackgroundColorKey))
        background.background_color = *background_color;
      if (auto* creative_instance_id =
              wallpaper.FindStringKey(kCreativeInstanceIDKey)) {
        background.creative_instance_id = *creative_instance_id;
      }
      if (auto* wallpaper_logo = wallpaper.FindDictKey(kLogoKey)) {
        background.logo =
            GetLogoFromValue(installed_dir, url_prefix, wallpaper_logo);
      } else {
        background.logo = default_logo;
      }
      campaign.backgrounds.push_back(background);
    }
  }

  return campaign;
}

void NTPSponsoredImagesData::ParseSRProperties(
    const base::Value& value,
    const base::FilePath& installed_dir) {
  if (theme_name.empty()) {
    DVLOG(2) << __func__ << ": Don't have NTP SR properties";
    return;
  }

  DVLOG(2) << __func__ << ": Theme name: " << theme_name;

  if (auto* sites = value.FindListKey(kTopSitesKey)) {
    for (const auto& top_site_value : sites->GetListDeprecated()) {
      TopSite site;
      if (auto* name = top_site_value.FindStringKey(kTopSiteNameKey))
        site.name = *name;

      if (auto* url = top_site_value.FindStringKey(kDestinationURLKey))
        site.destination_url = *url;

      if (auto* color = top_site_value.FindStringKey(kBackgroundColorKey))
        site.background_color = *color;

      if (auto* url = top_site_value.FindStringKey(kTopSiteIconURLKey)) {
        site.image_path = url_prefix + *url;
        site.image_file = installed_dir.AppendASCII(*url);
      }

      // TopSite should have all properties.
      DCHECK(site.IsValid());
      top_sites.push_back(site);
    }
  }
}

bool NTPSponsoredImagesData::IsValid() const {
  return !campaigns.empty();
}

bool NTPSponsoredImagesData::IsSuperReferral() const {
  return IsValid() && !theme_name.empty();
}

base::Value NTPSponsoredImagesData::GetBackgroundAt(size_t campaign_index,
                                                    size_t background_index) {
  DCHECK(campaign_index < campaigns.size() && background_index >= 0 &&
         background_index < campaigns[campaign_index].backgrounds.size());

  base::Value data(base::Value::Type::DICTIONARY);
  const auto campaign = campaigns[campaign_index];
  if (!campaign.IsValid())
    return data;

  data.SetStringKey(kThemeNameKey, theme_name);
  data.SetBoolKey(kIsSponsoredKey, !IsSuperReferral());
  data.SetBoolKey(kIsBackgroundKey, false);

  const auto background_file_path =
      campaign.backgrounds[background_index].image_file;
  const std::string wallpaper_image_url =
      url_prefix + background_file_path.BaseName().AsUTF8Unsafe();

  data.SetStringKey(kWallpaperImageURLKey, wallpaper_image_url);
  data.SetStringKey(kWallpaperImagePathKey,
                    background_file_path.AsUTF8Unsafe());
  data.SetIntKey(kWallpaperFocalPointXKey,
                 campaign.backgrounds[background_index].focal_point.x());
  data.SetIntKey(kWallpaperFocalPointYKey,
                 campaign.backgrounds[background_index].focal_point.y());

  data.SetStringKey(
      kCreativeInstanceIDKey,
      campaign.backgrounds[background_index].creative_instance_id);

  base::Value logo_data(base::Value::Type::DICTIONARY);
  Logo logo = campaign.backgrounds[background_index].logo;
  logo_data.SetStringKey(kImageKey, logo.image_url);
  logo_data.SetStringKey(kImagePathKey, logo.image_file.AsUTF8Unsafe());
  logo_data.SetStringKey(kCompanyNameKey, logo.company_name);
  logo_data.SetStringKey(kAltKey, logo.alt_text);
  logo_data.SetStringKey(kDestinationURLKey, logo.destination_url);
  data.SetKey(kLogoKey, std::move(logo_data));
  return data;
}

void NTPSponsoredImagesData::PrintCampaignsParsingResult() const {
  VLOG(2) << __func__ << ": This is "
          << (IsSuperReferral() ? " NTP SR Data" : " NTP SI Data");

  for (size_t i = 0; i < campaigns.size(); ++i) {
    const auto& backgrounds = campaigns[i].backgrounds;
    for (size_t j = 0; j < backgrounds.size(); ++j) {
      const auto& background = backgrounds[j];
      VLOG(2) << __func__ << ": background(" << j << " - "
              << background.logo.company_name
              << ") - id: " << background.creative_instance_id;
    }
  }
}

}  // namespace ntp_background_images
