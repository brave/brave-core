/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
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
constexpr char kRichMediaWallpaperRelativeUrlKey[] = "relativeUrl";

std::optional<std::string> ToString(WallpaperType wallpaper_type) {
  switch (wallpaper_type) {
    case WallpaperType::kImage: {
      return kImageWallpaperType;
    }

    case WallpaperType::kRichMedia: {
      return kRichMediaWallpaperType;
    }
  }

  return std::nullopt;
}

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

TopSite::TopSite(const TopSite& other) = default;

TopSite& TopSite::operator=(const TopSite& other) = default;

TopSite::TopSite(TopSite&& other) noexcept = default;

TopSite& TopSite::operator=(TopSite&& other) noexcept = default;

TopSite::~TopSite() = default;

bool TopSite::IsValid() const {
  return !name.empty() && !destination_url.empty() && !image_file.empty();
}

Logo::Logo() = default;

Logo::Logo(const Logo& other) = default;

Logo& Logo::operator=(const Logo& other) = default;

Logo::Logo(Logo&& other) noexcept = default;

Logo& Logo::operator=(Logo&& other) noexcept = default;

Logo::~Logo() = default;

Creative::Creative() = default;
Creative::Creative(const base::FilePath& file_path,
                   const gfx::Point& point,
                   const Logo& test_logo,
                   const std::string& creative_instance_id)
    : file_path(file_path),
      focal_point(point),
      creative_instance_id(creative_instance_id),
      logo(test_logo) {}

Creative::Creative(const Creative& other) = default;

Creative& Creative::operator=(const Creative& other) = default;

Creative::Creative(Creative&& other) noexcept = default;

Creative& Creative::operator=(Creative&& other) noexcept = default;

Creative::~Creative() = default;

Campaign::Campaign() = default;

Campaign::Campaign(const Campaign&) = default;

Campaign& Campaign::operator=(const Campaign&) = default;

Campaign::Campaign(Campaign&& other) noexcept = default;

Campaign& Campaign::operator=(Campaign&& other) noexcept = default;

Campaign::~Campaign() = default;

bool Campaign::IsValid() const {
  return !creatives.empty();
}

NTPSponsoredImagesData::NTPSponsoredImagesData() = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const base::Value::Dict& dict,
    const base::FilePath& installed_dir)
    : NTPSponsoredImagesData() {
  const std::optional<int> schema_version = dict.FindInt(kSchemaVersionKey);
  if (schema_version != kExpectedSchemaVersion) {
    // Currently, only version 2 is supported. Update this code to maintain.
    return;
  }

  url_prefix = base::StringPrintf("%s://%s/", content::kChromeUIScheme,
                                  kBrandedWallpaperHost);
  if (const std::string* const name = dict.FindString(kThemeNameKey)) {
    theme_name = *name;
    url_prefix += kSuperReferralPath;
  } else {
    url_prefix += kSponsoredImagesPath;
  }

  if (const base::Value::List* campaigns_value = dict.FindList(kCampaignsKey)) {
    ParseCampaigns(*campaigns_value, installed_dir);
  }

  ParseSuperReferrals(dict, installed_dir);
}

NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;

NTPSponsoredImagesData::NTPSponsoredImagesData(
    NTPSponsoredImagesData&& other) noexcept = default;

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    NTPSponsoredImagesData&& other) noexcept = default;

NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

void NTPSponsoredImagesData::ParseCampaigns(
    const base::Value::List& list,
    const base::FilePath& installed_dir) {
  for (const auto& value : list) {
    const base::Value::Dict* const dict = value.GetIfDict();
    if (!dict) {
      // Invalid campaign.
      continue;
    }

    if (const std::optional<Campaign> campaign =
            ParseCampaign(*dict, installed_dir)) {
      campaigns.push_back(*campaign);
    }
  }
}

// The changes to RichNTT were made to avoid altering the legacy `Campaign`,
// `Creative`, or `Logo` objects, minimizing changes to the existing code. The
// parsing logic will be removed once new tab page ads are served from the ads
// component for both non-Rewards and Rewards.
std::optional<Campaign> NTPSponsoredImagesData::ParseCampaign(
    const base::Value::Dict& dict,
    const base::FilePath& installed_dir) {
  Campaign campaign;

  const std::optional<int> campaign_version = dict.FindInt(kCampaignVersionKey);
  if (campaign_version != kExpectedCampaignVersion) {
    // Currently, only version 1 is supported. Update this code to maintain
    // backwards compatibility when adding new schema versions.
    return std::nullopt;
  }

  const std::string* const campaign_id = dict.FindString(kCampaignIdKey);
  if (!campaign_id) {
    // Campaign ID is required.
    return std::nullopt;
  }
  campaign.campaign_id = *campaign_id;

  const base::Value::List* const creative_sets =
      dict.FindList(kCreativeSetsKey);
  if (!creative_sets) {
    // Creative sets are required.
    return std::nullopt;
  }

  for (const auto& creative_set_value : *creative_sets) {
    const base::Value::Dict* const creative_set_dict =
        creative_set_value.GetIfDict();
    if (!creative_set_dict) {
      // Invalid creative set.
      continue;
    }

    const std::string* const creative_set_id =
        creative_set_dict->FindString(kCreativeSetIdKey);
    if (!creative_set_id) {
      // Creative set ID is required.
      continue;
    }

    const base::Value::List* const creatives =
        creative_set_dict->FindList(kCreativesKey);
    if (!creatives) {
      // Creative are required.
      continue;
    }

    for (const auto& creative_value : *creatives) {
      Creative creative;

      const base::Value::Dict* const creative_dict = creative_value.GetIfDict();
      if (!creative_dict) {
        // Invalid creative.
        continue;
      }

      const std::string* const creative_instance_id =
          creative_dict->FindString(kCreativeInstanceIdKey);
      if (!creative_instance_id) {
        // Creative instance ID is required.
        continue;
      }
      creative.creative_instance_id = *creative_instance_id;

      const std::string* const company_name =
          creative_dict->FindString(kCreativeCompanyNameKey);
      if (!company_name) {
        // Company name is required.
        continue;
      }
      creative.logo.company_name = *company_name;

      const std::string* const alt = creative_dict->FindString(kCreativeAltKey);
      if (!alt) {
        // Alt is required.
        continue;
      }
      creative.logo.alt_text = *alt;

      const std::string* const target_url =
          creative_dict->FindString(kCreativeTargetUrlKey);
      if (!target_url) {
        // Target URL is required.
        continue;
      }
      creative.logo.destination_url = *target_url;
      if (!brave_ads::ShouldSupportUrl(GURL(creative.logo.destination_url))) {
        // Target URL is not supported.
        continue;
      }

      // Condition matchers.
      const base::Value::List* const condition_matchers =
          creative_dict->FindList(kCreativeConditionMatchersKey);
      if (condition_matchers) {
        // Condition matchers are optional.
        for (const auto& condition_matcher_value : *condition_matchers) {
          const base::Value::Dict* const condition_matcher_dict =
              condition_matcher_value.GetIfDict();
          if (!condition_matcher_dict) {
            // Invalid condition matcher.
            continue;
          }

          const std::string* const condition =
              condition_matcher_dict->FindString(
                  kCreativeConditionMatcherConditionKey);
          if (!condition) {
            // Condition is required.
            continue;
          }

          const std::string* const pref_path =
              condition_matcher_dict->FindString(
                  kCreativeConditionMatcherPrefPathKey);
          if (!pref_path) {
            // Pref path is required.
            continue;
          }

          creative.condition_matchers.emplace(*pref_path, *condition);
        }
      }

      // Wallpaper.
      const base::Value::Dict* const wallpaper =
          creative_dict->FindDict(kWallpaperKey);
      if (!wallpaper) {
        // Wallpaper is required.
        continue;
      }

      const std::string* const wallpaper_type =
          wallpaper->FindString(kWallpaperTypeKey);
      if (!wallpaper_type) {
        // Wallpaper type is required.
        continue;
      }

      if (*wallpaper_type == kImageWallpaperType) {
        // Image.
        creative.wallpaper_type = WallpaperType::kImage;

        const std::string* const relative_url =
            wallpaper->FindString(kImageWallpaperRelativeUrlKey);
        if (!relative_url) {
          // Relative url is required.
          continue;
        }
        if (base::FilePath::FromUTF8Unsafe(*relative_url).ReferencesParent()) {
          // Path traversal, deny access.
          continue;
        }
        creative.file_path = installed_dir.AppendASCII(*relative_url);
        creative.url = GURL(url_prefix + *relative_url);

        // Focal point (optional).
        const int focal_point_x =
            wallpaper->FindIntByDottedPath(kImageWallpaperFocalPointXKey)
                .value_or(0);
        const int focal_point_y =
            wallpaper->FindIntByDottedPath(kImageWallpaperFocalPointYKey)
                .value_or(0);
        creative.focal_point = {focal_point_x, focal_point_y};

        // View box (optional, only used on iOS).
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
        creative.viewbox = {view_box_x, view_box_y, view_box_width,
                            view_box_height};

        // Background color (optional, only used on iOS).
        if (const std::string* const background_color =
                wallpaper->FindString(kImageWallpaperBackgroundColorKey)) {
          creative.background_color = *background_color;
        }

        // Button.
        const std::string* const button_image_relative_url =
            wallpaper->FindStringByDottedPath(
                kImageWallpaperButtonImageRelativeUrlKey);
        if (!button_image_relative_url) {
          // Relative url is required.
          continue;
        }
        if (base::FilePath::FromUTF8Unsafe(*button_image_relative_url)
                .ReferencesParent()) {
          // Path traversal, deny access.
          continue;
        }
        creative.logo.image_file =
            installed_dir.AppendASCII(*button_image_relative_url);
        creative.logo.image_url = url_prefix + *button_image_relative_url;
      } else if (*wallpaper_type == kRichMediaWallpaperType) {
        // Rich media.
        creative.wallpaper_type = WallpaperType::kRichMedia;

        const std::string* const relative_url =
            wallpaper->FindStringByDottedPath(
                kRichMediaWallpaperRelativeUrlKey);
        if (!relative_url) {
          // Relative url is required.
          continue;
        }
        if (base::FilePath::FromUTF8Unsafe(*relative_url).ReferencesParent()) {
          // Path traversal, deny access.
          continue;
        }
        creative.file_path = installed_dir.AppendASCII(*relative_url);
        creative.url = GURL(kNTPNewTabTakeoverRichMediaUrl + *relative_url);
      } else {
        // Invalid wallpaper type.
        continue;
      }

      campaign.creatives.push_back(creative);
    }
  }

  if (campaign.creatives.empty()) {
    // At least one creative is required.
    return std::nullopt;
  }

  return campaign;
}

void NTPSponsoredImagesData::ParseSuperReferrals(
    const base::Value::Dict& dict,
    const base::FilePath& installed_dir) {
  if (theme_name.empty()) {
    DVLOG(2) << __func__ << ": Don't have NTP SR properties";
    return;
  }

  DVLOG(2) << __func__ << ": Theme name: " << theme_name;

  const base::Value::List* const list = dict.FindList(kTopSitesKey);
  if (!list) {
    return;
  }

  for (const auto& value : *list) {
    const base::Value::Dict* const top_site_dict = value.GetIfDict();
    if (!top_site_dict) {
      continue;
    }

    const std::string* const name = top_site_dict->FindString(kTopSiteNameKey);
    if (!name) {
      continue;
    }

    const std::string* const destination_url =
        top_site_dict->FindString(kDestinationURLKey);
    if (!destination_url) {
      continue;
    }

    const std::string* const background_color =
        top_site_dict->FindString(kBackgroundColorKey);
    if (!background_color) {
      continue;
    }

    const std::string* const icon_url =
        top_site_dict->FindString(kTopSiteIconURLKey);
    if (!icon_url) {
      continue;
    }

    TopSite top_site;
    top_site.name = *name;
    top_site.destination_url = *destination_url;
    top_site.background_color = *background_color;
    top_site.image_path = url_prefix + *icon_url;
    top_site.image_file = installed_dir.AppendASCII(*icon_url);

    if (top_site.IsValid()) {
      top_sites.push_back(top_site);
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
    size_t creative_index) const {
  DCHECK(campaign_index < campaigns.size() &&
         creative_index < campaigns[campaign_index].creatives.size());

  const Campaign& campaign = campaigns[campaign_index];

  base::Value::List condition_matchers;
  for (const auto& [pref_path, condition] :
       campaign.creatives[creative_index].condition_matchers) {
    condition_matchers.Append(
        base::Value::Dict()
            .Set(kWallpaperConditionMatcherPrefPathKey, pref_path)
            .Set(kWallpaperConditionMatcherKey, condition));
  }

  const Logo& logo = campaign.creatives[creative_index].logo;

  base::Value::Dict data =
      base::Value::Dict()
          .Set(kCampaignIdKey, campaign.campaign_id)
          .Set(kCreativeInstanceIDKey,
               campaign.creatives[creative_index].creative_instance_id)
          .Set(kThemeNameKey, theme_name)
          .Set(kIsSponsoredKey, !IsSuperReferral())
          .Set(kIsBackgroundKey, false)
          .Set(kWallpaperIDKey,
               base::Uuid::GenerateRandomV4().AsLowercaseString())
          .Set(kWallpaperURLKey, campaign.creatives[creative_index].url.spec())
          .Set(kWallpaperFilePathKey,
               campaign.creatives[creative_index].file_path.AsUTF8Unsafe())
          .Set(kWallpaperFocalPointXKey,
               campaign.creatives[creative_index].focal_point.x())
          .Set(kWallpaperFocalPointYKey,
               campaign.creatives[creative_index].focal_point.y())
          .Set(kWallpaperConditionMatchersKey, std::move(condition_matchers))
          .Set(kLogoKey, base::Value::Dict()
                             .Set(kImageKey, logo.image_url)
                             .Set(kImagePathKey, logo.image_file.AsUTF8Unsafe())
                             .Set(kCompanyNameKey, logo.company_name)
                             .Set(kAltKey, logo.alt_text)
                             .Set(kDestinationURLKey, logo.destination_url));

  if (const std::optional<std::string> wallpaper_type =
          ToString(campaign.creatives[creative_index].wallpaper_type)) {
    data.Set(kWallpaperTypeKey, *wallpaper_type);
  }

  return data;
}

std::optional<base::Value::Dict> NTPSponsoredImagesData::GetBackground(
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

  const auto& creatives = campaigns[campaign_index].creatives;
  size_t creative_index = 0;
  for (; creative_index != creatives.size(); ++creative_index) {
    if (creatives[creative_index].creative_instance_id ==
        ad_info.creative_instance_id) {
      break;
    }
  }
  if (creative_index == creatives.size()) {
    VLOG(0) << "Creative instance wasn't found in NTP sponsored images data: "
            << ad_info.creative_instance_id;
    return std::nullopt;
  }

  if (VLOG_IS_ON(0)) {
    if (!AdInfoMatchesSponsoredImage(ad_info, campaign_index, creative_index)) {
      VLOG(0) << "Served creative info does not fully match with NTP "
                 "sponsored images metadata. Campaign id: "
              << ad_info.campaign_id
              << ". Creative instance id: " << ad_info.creative_instance_id;
    }
  }

  std::optional<base::Value::Dict> dict =
      GetBackgroundAt(campaign_index, creative_index);
  if (dict) {
    dict->Set(kWallpaperIDKey, ad_info.placement_id);
  }
  return dict;
}

bool NTPSponsoredImagesData::AdInfoMatchesSponsoredImage(
    const brave_ads::NewTabPageAdInfo& ad_info,
    size_t campaign_index,
    size_t creative_index) const {
  DCHECK(campaign_index < campaigns.size() &&
         creative_index < campaigns[campaign_index].creatives.size());

  const Campaign& campaign = campaigns[campaign_index];

  if (ad_info.campaign_id != campaign.campaign_id) {
    return false;
  }

  const Creative& creative = campaign.creatives[creative_index];
  if (ad_info.creative_instance_id != creative.creative_instance_id) {
    return false;
  }

  if (ad_info.target_url != GURL(creative.logo.destination_url)) {
    return false;
  }

  if (ad_info.alt != creative.logo.alt_text) {
    return false;
  }

  if (ad_info.company_name != creative.logo.company_name) {
    return false;
  }

  return true;
}

}  // namespace ntp_background_images
