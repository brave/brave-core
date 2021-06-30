/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
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
                      int index,
                      const base::Value* value) {
  DCHECK(value && value->is_dict());
  Logo logo;

  if (auto* url = value->FindStringKey(kImageURLKey))
    logo.image_file = installed_dir.AppendASCII(*url);

  if (auto* alt_text = value->FindStringKey(kAltKey))
    logo.alt_text = *alt_text;

  if (auto* name = value->FindStringKey(kCompanyNameKey))
    logo.company_name = *name;

  if (auto* url = value->FindStringKey(kDestinationURLKey))
    logo.destination_url = *url;

  logo.image_url = url_prefix +
     (index >= 0 ? base::StringPrintf("%s%d.png", kLogoFileNamePrefix, index)
                 : kDefaultLogoFileName);

  return logo;
}

}  // namespace

TopSite::TopSite() = default;
TopSite::TopSite(
    const std::string& i_name, const std::string i_destination_url,
    const std::string& i_image_path, const base::FilePath& i_image_file)
    : name(i_name), destination_url(i_destination_url),
      image_path(i_image_path), image_file(i_image_file) {}
TopSite& TopSite::operator=(const TopSite& data) = default;
TopSite::TopSite(const TopSite& data) = default;
TopSite::~TopSite() = default;

bool TopSite::IsValid() const {
  return !name.empty() && !destination_url.empty() && !image_file.empty();
}

Logo::Logo() = default;
Logo::Logo(const Logo&) = default;
Logo::~Logo() = default;

Background::Background() = default;
Background::Background(const base::FilePath& image_file_path,
                       const gfx::Point& point)
    : image_file(image_file_path),
      focal_point(point) {}
Background::Background(const Background&) = default;
Background::~Background() = default;

NTPBackgroundImagesData::NTPBackgroundImagesData()
    : url_prefix(base::StringPrintf("%s://%s/",
                                    content::kChromeUIScheme,
                                    kBrandedWallpaperHost)) {}

NTPBackgroundImagesData::NTPBackgroundImagesData(
    const std::string& json_string,
    const base::FilePath& installed_dir)
    : NTPBackgroundImagesData() {
  absl::optional<base::Value> json_value = base::JSONReader::Read(json_string);
  if (!json_value) {
    DVLOG(2) << "Read json data failed. Invalid JSON data";
    return;
  }

  absl::optional<int> incomingSchemaVersion =
      json_value->FindIntKey(kSchemaVersionKey);
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

  if (auto* name = json_value->FindStringKey(kThemeNameKey)) {
    theme_name = *name;
    DVLOG(2) << __func__ << ": Theme name: " << theme_name;
  }

  if (auto* logo = json_value->FindDictKey(kLogoKey))
    default_logo = GetLogoFromValue(installed_dir, GetURLPrefix(), -1, logo);

  if (auto* wallpapers = json_value->FindListKey(kWallpapersKey)) {
    const int wallpaper_count = wallpapers->GetList().size();
    for (int i = 0; i < wallpaper_count; ++i) {
      const auto& wallpaper = wallpapers->GetList()[i];
      Background background;
      background.image_file =
          installed_dir.AppendASCII(*wallpaper.FindStringKey(kImageURLKey));

      if (auto* focalPoint = wallpaper.FindDictKey(kWallpaperFocalPointKey)) {
        background.focal_point = { focalPoint->FindIntKey(kXKey).value_or(0),
                                   focalPoint->FindIntKey(kYKey).value_or(0) };
      }

      if (auto* viewbox = wallpaper.FindDictKey(kViewboxKey)) {
        gfx::Rect rect(viewbox->FindIntKey(kXKey).value_or(0),
                       viewbox->FindIntKey(kYKey).value_or(0),
                       viewbox->FindIntKey(kWidthKey).value_or(0),
                       viewbox->FindIntKey(kHeightKey).value_or(0));
        background.viewbox.emplace(rect);
      }
      if (auto* background_color = wallpaper.FindStringKey(kBackgroundColorKey))
        background.background_color =  *background_color;
      if (auto* creative_instance_id =
              wallpaper.FindStringKey(kCreativeInstanceIDKey)) {
        background.creative_instance_id =  *creative_instance_id;
      }
      if (auto* wallpaper_logo = wallpaper.FindDictKey(kLogoKey)) {
        background.logo.emplace(GetLogoFromValue(
            installed_dir, GetURLPrefix(), i, wallpaper_logo));
      }
      backgrounds.push_back(background);
    }
  }

  if (auto* sites = json_value->FindListKey(kTopSitesKey)) {
    for (const auto& top_site_value : sites->GetList()) {
      TopSite site;
      if (auto* name = top_site_value.FindStringKey(kTopSiteNameKey))
        site.name = *name;

      if (auto* url = top_site_value.FindStringKey(kDestinationURLKey))
        site.destination_url = *url;

      if (auto* color =
              top_site_value.FindStringKey(kBackgroundColorKey))
        site.background_color = *color;

      if (auto* url = top_site_value.FindStringKey(kTopSiteIconURLKey)) {
        site.image_path = *url;
        site.image_file = installed_dir.AppendASCII(*url);
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
    const NTPBackgroundImagesData& data) = default;
NTPBackgroundImagesData::~NTPBackgroundImagesData() = default;

bool NTPBackgroundImagesData::IsValid() const {
  return !backgrounds.empty();
}

bool NTPBackgroundImagesData::IsSuperReferral() const {
  return IsValid() && !theme_name.empty();
}

base::Value NTPBackgroundImagesData::GetBackgroundAt(size_t index) {
  DCHECK(index >= 0 && index < wallpaper_image_urls().size());

  base::Value data(base::Value::Type::DICTIONARY);
  if (!IsValid())
    return data;

  data.SetStringKey(kThemeNameKey, theme_name);
  data.SetBoolKey(kIsSponsoredKey, !IsSuperReferral());
  data.SetStringKey(kWallpaperImageURLKey,
                    wallpaper_image_urls()[index]);
  data.SetStringKey(kWallpaperImagePathKey,
                    backgrounds[index].image_file.AsUTF8Unsafe());
  data.SetIntKey(kWallpaperFocalPointXKey, backgrounds[index].focal_point.x());
  data.SetIntKey(kWallpaperFocalPointYKey, backgrounds[index].focal_point.y());

  data.SetStringKey(kCreativeInstanceIDKey,
                    backgrounds[index].creative_instance_id);

  base::Value logo_data(base::Value::Type::DICTIONARY);
  Logo logo = backgrounds[index].logo ? backgrounds[index].logo.value()
                                      : default_logo;
  logo_data.SetStringKey(kImageKey, logo.image_url);
  logo_data.SetStringKey(kImagePathKey, logo.image_file.AsUTF8Unsafe());
  logo_data.SetStringKey(kCompanyNameKey, logo.company_name);
  logo_data.SetStringKey(kAltKey, logo.alt_text);
  logo_data.SetStringKey(kDestinationURLKey, logo.destination_url);
  data.SetKey(kLogoKey, std::move(logo_data));
  return data;
}

std::string NTPBackgroundImagesData::GetURLPrefix() const {
  return url_prefix + (theme_name.empty() ? kSponsoredImagesPath
                                          : kSuperReferralPath);
}

std::vector<TopSite> NTPBackgroundImagesData::GetTopSitesForWebUI() const {
  std::vector<TopSite> top_sites_for_webui;
  for (const auto& top_site : top_sites) {
    TopSite top_site_for_webui = top_site;
    top_site_for_webui.image_path =  GetURLPrefix() + top_site.image_path;
    top_sites_for_webui.push_back(top_site_for_webui);
  }
  return top_sites_for_webui;
}

std::vector<std::string> NTPBackgroundImagesData::wallpaper_image_urls() const {
  std::vector<std::string> wallpaper_image_urls;
  for (size_t i = 0; i < backgrounds.size(); i++) {
    const std::string wallpaper_image_url = GetURLPrefix() + base::StringPrintf(
        "%s%zu.jpg", kWallpaperPathPrefix, i);
    wallpaper_image_urls.push_back(wallpaper_image_url);
  }
  return wallpaper_image_urls;
}

}  // namespace ntp_background_images
