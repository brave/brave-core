/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

namespace ntp_background_images {

namespace {

constexpr int kExpectedSchemaVersion = 2;
constexpr int kExpectedCampaignVersion = 1;

constexpr char kCampaignVersionKey[] = "version";
constexpr char kCreativeSetsKey[] = "creativeSets";
constexpr char kCreativeSetIdKey[] = "creativeSetId";
constexpr char kCreativesKey[] = "creatives";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";
constexpr char kCreativeCompanyNameKey[] = "companyName";
constexpr char kCreativeAltKey[] = "alt";
constexpr char kCreativeTargetUrlKey[] = "targetUrl";
constexpr char kCreativeConditionMatchersKey[] = "conditionMatchers";
constexpr char kCreativeConditionMatcherConditionKey[] = "condition";
constexpr char kCreativeConditionMatcherPrefPathKey[] = "prefPath";
constexpr char kWallpaperKey[] = "wallpaper";
constexpr char kImageWallpaperType[] = "image";
constexpr char kImageWallpaperRelativeUrlKey[] = "relativeUrl";
constexpr char kImageWallpaperFocalPointXKey[] = "focalPoint.x";
constexpr char kImageWallpaperFocalPointYKey[] = "focalPoint.y";
constexpr char kImageWallpaperViewBoxXKey[] = "viewBox.x";
constexpr char kImageWallpaperViewBoxYKey[] = "viewBox.y";
constexpr char kImageWallpaperViewBoxWidthKey[] = "viewBox.width";
constexpr char kImageWallpaperViewBoxHeightKey[] = "viewBox.height";
constexpr char kImageWallpaperBackgroundColorKey[] = "backgroundColor";
constexpr char kImageWallpaperButtonImageRelativeUrlKey[] =
    "button.image.relativeUrl";
constexpr char kRichMediaWallpaperType[] = "richMedia";
constexpr char kRichMediaWallpaperRelativeUrlKey[] = "relativeUrl";

}  // namespace

TopSite::TopSite() = default;
TopSite::TopSite(const std::string& i_name,
                 const std::string& i_destination_url,
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
SponsoredBackground::SponsoredBackground(
    const base::FilePath& file_path,
    const gfx::Point& point,
    const Logo& test_logo,
    const std::string& creative_instance_id)
    : file_path(file_path),
      focal_point(point),
      creative_instance_id(creative_instance_id),
      logo(test_logo) {}
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
    const base::Value::Dict& data,
    const base::FilePath& installed_dir)
    : NTPSponsoredImagesData() {
  const std::optional<int> schema_version = data.FindInt(kSchemaVersionKey);
  if (schema_version != kExpectedSchemaVersion) {
    DVLOG(2) << "Mismatched schema version";
    return;
  }

  url_prefix = base::StringPrintf("%s://%s/", content::kChromeUIScheme,
                                  kBrandedWallpaperHost);
  if (auto* name = data.FindString(kThemeNameKey)) {
    theme_name = *name;
    url_prefix += kSuperReferralPath;
  } else {
    url_prefix += kSponsoredImagesPath;
  }

  if (auto* campaigns_value = data.FindList(kCampaignsKey)) {
    ParseCampaignsList(*campaigns_value, installed_dir);
  }

  ParseSRProperties(data, installed_dir);

  PrintCampaignsParsingResult();
}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

void NTPSponsoredImagesData::ParseCampaignsList(
    const base::Value::List& campaigns_value,
    const base::FilePath& installed_dir) {
  for (const auto& campaign_value : campaigns_value) {
    DCHECK(campaign_value.is_dict());
    const std::optional<Campaign> campaign =
        GetCampaignFromValue(campaign_value.GetDict(), installed_dir);
    if (!campaign || !campaign->IsValid()) {
      DVLOG(2) << "Campaign is not valid. Skipping.";
      continue;
    }

    campaigns.push_back(*campaign);
  }
}

// The changes to RichNTT were made to avoid altering the legacy `Campaign`,
// `SponsoredBackground`, or `Logo` objects, minimizing changes to the
// existing code. The parsing logic will be removed once new tab page ads are
// served from the ads component for both non-Rewards and Rewards.
std::optional<Campaign> NTPSponsoredImagesData::GetCampaignFromValue(
    const base::Value::Dict& value,
    const base::FilePath& installed_dir) {
  Campaign campaign;

  const std::optional<int> campaign_version =
      value.FindInt(kCampaignVersionKey);
  if (campaign_version != kExpectedCampaignVersion) {
    // Currently, only version 1 is supported. Update this code to maintain
    // backwards compatibility when adding new schema versions.
    return std::nullopt;
  }

  const std::string* const campaign_id = value.FindString(kCampaignIdKey);
  if (!campaign_id) {
    // Campaign ID is required.
    return std::nullopt;
  }
  campaign.campaign_id = *campaign_id;

  const base::Value::List* const creative_sets =
      value.FindList(kCreativeSetsKey);
  if (!creative_sets) {
    // Creative sets are required.
    return std::nullopt;
  }

  for (const auto& creative_set : *creative_sets) {
    const base::Value::Dict* const creative_set_dict = creative_set.GetIfDict();
    if (!creative_set_dict) {
      // Invalid creative set.
      return std::nullopt;
    }

    const std::string* const creative_set_id =
        creative_set_dict->FindString(kCreativeSetIdKey);
    if (!creative_set_id) {
      // Creative set ID is required.
      return std::nullopt;
    }

    const base::Value::List* const creatives =
        creative_set_dict->FindList(kCreativesKey);
    if (!creatives) {
      // Creative are required.
      return std::nullopt;
    }

    for (const auto& creative : *creatives) {
      SponsoredBackground background;

      const base::Value::Dict* const creative_dict = creative.GetIfDict();
      if (!creative_dict) {
        // Invalid creative.
        return std::nullopt;
      }

      const std::string* const creative_instance_id =
          creative_dict->FindString(kCreativeInstanceIdKey);
      if (!creative_instance_id) {
        // Creative instance ID is required.
        return std::nullopt;
      }
      background.creative_instance_id = *creative_instance_id;

      const std::string* const company_name =
          creative_dict->FindString(kCreativeCompanyNameKey);
      if (!company_name) {
        // Company name is required.
        return std::nullopt;
      }
      background.logo.company_name = *company_name;

      const std::string* const alt = creative_dict->FindString(kCreativeAltKey);
      if (!alt) {
        // Alt is required.
        return std::nullopt;
      }
      background.logo.alt_text = *alt;

      const std::string* const target_url =
          creative_dict->FindString(kCreativeTargetUrlKey);
      if (!target_url) {
        // Target URL is required.
        return std::nullopt;
      }
      background.logo.destination_url = *target_url;

      // Condition matchers.
      const base::Value::List* const condition_matchers =
          creative_dict->FindList(kCreativeConditionMatchersKey);
      if (condition_matchers) {
        // Condition matchers are optional.
        for (const auto& condition_matcher : *condition_matchers) {
          const base::Value::Dict* const condition_matcher_dict =
              condition_matcher.GetIfDict();
          if (!condition_matcher_dict) {
            // Invalid condition matcher.
            return std::nullopt;
          }

          const std::string* const condition =
              condition_matcher_dict->FindString(
                  kCreativeConditionMatcherConditionKey);
          if (!condition) {
            // Condition is required.
            return std::nullopt;
          }

          const std::string* const pref_path =
              condition_matcher_dict->FindString(
                  kCreativeConditionMatcherPrefPathKey);
          if (!pref_path) {
            // Pref path is required.
            return std::nullopt;
          }

          background.condition_matchers.emplace(*pref_path, *condition);
        }
      }

      // Wallpaper.
      const base::Value::Dict* const wallpaper =
          creative_dict->FindDict(kWallpaperKey);
      if (!wallpaper) {
        // Wallpaper is required.
        return std::nullopt;
      }

      const std::string* const wallpaper_type =
          wallpaper->FindString(kWallpaperTypeKey);
      if (!wallpaper_type) {
        // Wallpaper type is required.
        return std::nullopt;
      }

      if (*wallpaper_type == kImageWallpaperType) {
        // Image.
        background.wallpaper_type = WallpaperType::kImage;

        const std::string* const relative_url =
            wallpaper->FindString(kImageWallpaperRelativeUrlKey);
        if (!relative_url) {
          // Relative url is required.
          return std::nullopt;
        }
        background.file_path = installed_dir.AppendASCII(*relative_url);
        background.url = GURL(url_prefix + *relative_url);

        // Focal point (optional).
        const int focal_point_x =
            wallpaper->FindIntByDottedPath(kImageWallpaperFocalPointXKey)
                .value_or(0);
        const int focal_point_y =
            wallpaper->FindIntByDottedPath(kImageWallpaperFocalPointYKey)
                .value_or(0);
        background.focal_point = {focal_point_x, focal_point_y};

        // View box (optional for iOS).
        const int view_box_x =
            wallpaper->FindIntByDottedPath(kImageWallpaperViewBoxXKey)
                .value_or(0);
        const int view_box_y =
            wallpaper->FindIntByDottedPath(kImageWallpaperViewBoxYKey)
                .value_or(0);
        const int view_box_width =
            wallpaper->FindIntByDottedPath(kImageWallpaperViewBoxWidthKey)
                .value_or(0);
        const int view_box_height =
            wallpaper->FindIntByDottedPath(kImageWallpaperViewBoxHeightKey)
                .value_or(0);
        background.viewbox = {view_box_x, view_box_y, view_box_width,
                              view_box_height};

        // Background color (optional for iOS).
        if (const std::string* const background_color =
                wallpaper->FindString(kImageWallpaperBackgroundColorKey)) {
          background.background_color = *background_color;
        }

        // Button.
        const std::string* const button_image_relative_url =
            wallpaper->FindStringByDottedPath(
                kImageWallpaperButtonImageRelativeUrlKey);
        if (!button_image_relative_url) {
          // Button image relative url is required.
          return std::nullopt;
        }
        background.logo.image_file =
            installed_dir.AppendASCII(*button_image_relative_url);
        background.logo.image_url = url_prefix + *button_image_relative_url;
      } else if (*wallpaper_type == kRichMediaWallpaperType) {
        // Rich media.
        background.wallpaper_type = WallpaperType::kRichMedia;

        const std::string* const relative_url =
            wallpaper->FindStringByDottedPath(
                kRichMediaWallpaperRelativeUrlKey);
        if (!relative_url) {
          // Relative url is required.
          return std::nullopt;
        }
        background.file_path = installed_dir.AppendASCII(*relative_url);
        background.url = GURL(kNTPSponsoredRichMediaUrl + *relative_url);
      } else {
        // Invalid wallpaper type.
        return std::nullopt;
      }

      campaign.backgrounds.push_back(background);
    }
  }

  return campaign;
}

void NTPSponsoredImagesData::ParseSRProperties(
    const base::Value::Dict& value,
    const base::FilePath& installed_dir) {
  if (theme_name.empty()) {
    DVLOG(2) << __func__ << ": Don't have NTP SR properties";
    return;
  }

  DVLOG(2) << __func__ << ": Theme name: " << theme_name;

  if (auto* sites = value.FindList(kTopSitesKey)) {
    for (const auto& item : *sites) {
      const auto& top_site_dict = item.GetDict();
      TopSite site;
      if (auto* name = top_site_dict.FindString(kTopSiteNameKey)) {
        site.name = *name;
      }

      if (auto* url = top_site_dict.FindString(kDestinationURLKey)) {
        site.destination_url = *url;
      }

      if (auto* color = top_site_dict.FindString(kBackgroundColorKey)) {
        site.background_color = *color;
      }

      if (auto* url = top_site_dict.FindString(kTopSiteIconURLKey)) {
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

std::optional<base::Value::Dict> NTPSponsoredImagesData::GetBackgroundAt(
    size_t campaign_index,
    size_t background_index) {
  DCHECK(campaign_index < campaigns.size() && background_index >= 0 &&
         background_index < campaigns[campaign_index].backgrounds.size());

  const auto campaign = campaigns[campaign_index];
  if (!campaign.IsValid())
    return std::nullopt;

  base::Value::Dict data;
  data.Set(kThemeNameKey, theme_name);
  data.Set(kIsSponsoredKey, !IsSuperReferral());
  data.Set(kIsBackgroundKey, false);
  data.Set(kWallpaperIDKey, base::Uuid::GenerateRandomV4().AsLowercaseString());

  const WallpaperType wallpaper_type =
      campaign.backgrounds[background_index].wallpaper_type;
  switch (wallpaper_type) {
    case WallpaperType::kImage: {
      data.Set(kWallpaperTypeKey, "image");
      break;
    }

    case WallpaperType::kRichMedia: {
      data.Set(kWallpaperTypeKey, "richMedia");
      break;
    }
  }

  data.Set(kWallpaperURLKey, campaign.backgrounds[background_index].url.spec());
  data.Set(kWallpaperFilePathKey,
           campaign.backgrounds[background_index].file_path.AsUTF8Unsafe());
  data.Set(kWallpaperFocalPointXKey,
           campaign.backgrounds[background_index].focal_point.x());
  data.Set(kWallpaperFocalPointYKey,
           campaign.backgrounds[background_index].focal_point.y());

  base::Value::List condition_matchers;
  for (const auto& [pref_path, condition] :
       campaign.backgrounds[background_index].condition_matchers) {
    base::Value::Dict dict;
    dict.Set(kWallpaperConditionMatcherPrefPathKey, pref_path);
    dict.Set(kWallpaperConditionMatcherKey, condition);
    condition_matchers.Append(std::move(dict));
  }
  data.Set(kWallpaperConditionMatchersKey, std::move(condition_matchers));

  data.Set(kCampaignIdKey, campaign.campaign_id);
  data.Set(kCreativeInstanceIDKey,
           campaign.backgrounds[background_index].creative_instance_id);

  base::Value::Dict logo_data;
  Logo logo = campaign.backgrounds[background_index].logo;
  logo_data.Set(kImageKey, logo.image_url);
  logo_data.Set(kImagePathKey, logo.image_file.AsUTF8Unsafe());
  logo_data.Set(kCompanyNameKey, logo.company_name);
  logo_data.Set(kAltKey, logo.alt_text);
  logo_data.Set(kDestinationURLKey, logo.destination_url);
  data.Set(kLogoKey, std::move(logo_data));
  return data;
}

std::optional<base::Value::Dict>
NTPSponsoredImagesData::GetBackgroundFromAdInfo(
    const brave_ads::NewTabPageAdInfo& ad_info) {
  // Find campaign
  size_t campaign_index = 0;
  for (; campaign_index != campaigns.size(); ++campaign_index) {
    if (campaigns[campaign_index].campaign_id == ad_info.campaign_id) {
      break;
    }
  }
  if (campaign_index == campaigns.size()) {
    VLOG(0) << "The ad campaign wasn't found in the NTP sponsored images data: "
            << ad_info.campaign_id;
    return std::nullopt;
  }

  const auto& sponsored_backgrounds = campaigns[campaign_index].backgrounds;
  size_t background_index = 0;
  for (; background_index != sponsored_backgrounds.size(); ++background_index) {
    if (sponsored_backgrounds[background_index].creative_instance_id ==
        ad_info.creative_instance_id) {
      break;
    }
  }
  if (background_index == sponsored_backgrounds.size()) {
    VLOG(0) << "Creative instance wasn't found in NTP sposored images data: "
            << ad_info.creative_instance_id;
    return std::nullopt;
  }

  if (VLOG_IS_ON(0)) {
    if (!AdInfoMatchesSponsoredImage(ad_info, campaign_index,
                                     background_index)) {
      VLOG(0) << "Served creative info does not fully match with NTP "
                 "sponsored images metadata. Campaign id: "
              << ad_info.campaign_id
              << ". Creative instance id: " << ad_info.creative_instance_id;
    }
  }

  std::optional<base::Value::Dict> data =
      GetBackgroundAt(campaign_index, background_index);
  if (data) {
    data->Set(kWallpaperIDKey, ad_info.placement_id);
  }
  return data;
}

void NTPSponsoredImagesData::PrintCampaignsParsingResult() const {
  VLOG(2) << __func__ << ": This is "
          << (IsSuperReferral() ? " NTP SR Data" : " NTP SI Data");

  for (const auto& campaign : campaigns) {
    const auto& backgrounds = campaign.backgrounds;
    for (size_t j = 0; j < backgrounds.size(); ++j) {
      const auto& background = backgrounds[j];
      VLOG(2) << __func__ << ": background(" << j << " - "
              << background.logo.company_name
              << ") - id: " << background.creative_instance_id;
    }
  }
}

bool NTPSponsoredImagesData::AdInfoMatchesSponsoredImage(
    const brave_ads::NewTabPageAdInfo& ad_info,
    size_t campaign_index,
    size_t background_index) const {
  DCHECK(campaign_index < campaigns.size() && background_index >= 0 &&
         background_index < campaigns[campaign_index].backgrounds.size());

  const Campaign& campaign = campaigns[campaign_index];
  if (!campaign.IsValid()) {
    return false;
  }

  if (ad_info.campaign_id != campaign.campaign_id) {
    return false;
  }

  const SponsoredBackground& background =
      campaign.backgrounds[background_index];
  if (ad_info.creative_instance_id != background.creative_instance_id) {
    return false;
  }

  if (ad_info.target_url != GURL(background.logo.destination_url)) {
    return false;
  }

  if (ad_info.alt != background.logo.alt_text) {
    return false;
  }

  if (ad_info.company_name != background.logo.company_name) {
    return false;
  }

  return true;
}

}  // namespace ntp_background_images
