/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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
constexpr char kWallpaperKey[] = "wallpaper";
constexpr char kImageWallpaperRelativeUrlKey[] = "relativeUrl";
constexpr char kImageWallpaperFocalPointXKey[] = "focalPoint.x";
constexpr char kImageWallpaperFocalPointYKey[] = "focalPoint.y";
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

std::optional<Campaign> MaybeParseCampaign(
    const base::Value::Dict& dict,
    const base::FilePath& installed_dir) {
  Campaign campaign;

  const std::optional<int> campaign_version = dict.FindInt(kCampaignVersionKey);
  if (campaign_version != kExpectedCampaignVersion) {
    // Currently, only version 2 is supported. Update this code to maintain
    // backwards compatibility when adding new schema versions.
    return std::nullopt;
  }

  const std::string* const campaign_id = dict.FindString(kCampaignIdKey);
  if (!campaign_id) {
    // Campaign ID is required.
    return std::nullopt;
  }
  campaign.campaign_id = *campaign_id;

  brave_ads::mojom::NewTabPageAdMetricType metric_type =
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation;
  if (const std::string* const metrics = dict.FindString(kCampaignMetricsKey)) {
    if (*metrics == "disabled") {
      metric_type = brave_ads::mojom::NewTabPageAdMetricType::kDisabled;
    } else if (*metrics == "confirmation") {
      metric_type = brave_ads::mojom::NewTabPageAdMetricType::kConfirmation;
    }
  }

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

      // Wallpaper.
      const base::Value::Dict* const wallpaper_dict =
          creative_dict->FindDict(kWallpaperKey);
      if (!wallpaper_dict) {
        // Wallpaper is required.
        continue;
      }

      const std::string* const wallpaper_type =
          wallpaper_dict->FindString(kWallpaperTypeKey);
      if (!wallpaper_type) {
        // Wallpaper type is required.
        continue;
      }

      if (*wallpaper_type == kImageWallpaperType) {
        // Image.
        creative.wallpaper_type = WallpaperType::kImage;

        const std::string* const relative_url =
            wallpaper_dict->FindString(kImageWallpaperRelativeUrlKey);
        if (!relative_url) {
          // Relative url is required.
          continue;
        }
        if (base::FilePath::FromUTF8Unsafe(*relative_url).ReferencesParent()) {
          // Path traversal, deny access.
          continue;
        }
        creative.file_path = installed_dir.AppendASCII(*relative_url);
        const std::string creative_url_string = base::ReplaceStringPlaceholders(
            "$1://$2/$3",
            {content::kChromeUIScheme, kBrandedWallpaperHost, *relative_url},
            nullptr);
        creative.url = GURL(creative_url_string);

        // Focal point (optional).
        const int focal_point_x =
            wallpaper_dict->FindIntByDottedPath(kImageWallpaperFocalPointXKey)
                .value_or(0);
        const int focal_point_y =
            wallpaper_dict->FindIntByDottedPath(kImageWallpaperFocalPointYKey)
                .value_or(0);
        creative.focal_point = {focal_point_x, focal_point_y};

        // Button.
        const std::string* const button_image_relative_url =
            wallpaper_dict->FindStringByDottedPath(
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
        creative.logo.image_url = base::ReplaceStringPlaceholders(
            "$1://$2/$3",
            {content::kChromeUIScheme, kBrandedWallpaperHost,
             *button_image_relative_url},
            nullptr);
      } else if (*wallpaper_type == kRichMediaWallpaperType) {
        // Rich media.
        creative.wallpaper_type = WallpaperType::kRichMedia;

        const std::string* const relative_url =
            wallpaper_dict->FindString(kRichMediaWallpaperRelativeUrlKey);
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
        // Unknown wallpaper type.
        continue;
      }

      creative.metric_type = metric_type;

      campaign.creatives.push_back(creative);
    }
  }

  if (campaign.creatives.empty()) {
    // At least one creative is required.
    return std::nullopt;
  }

  return campaign;
}

}  // namespace

Logo::Logo() = default;

Logo::Logo(const Logo& other) = default;

Logo& Logo::operator=(const Logo& other) = default;

Logo::Logo(Logo&& other) noexcept = default;

Logo& Logo::operator=(Logo&& other) noexcept = default;

Logo::~Logo() = default;

Creative::Creative() = default;
Creative::Creative(WallpaperType wallpaper_type,
                   const base::FilePath& file_path,
                   const gfx::Point& point,
                   const Logo& test_logo,
                   const std::string& creative_instance_id)
    : wallpaper_type(wallpaper_type),
      file_path(file_path),
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
    // Currently, only version 1 is supported. Update this code to maintain.
    return;
  }

  url_prefix = absl::StrFormat("%s://%s/", content::kChromeUIScheme,
                               kBrandedWallpaperHost);
  url_prefix += kSponsoredImagesPath;

  if (const base::Value::List* const value = dict.FindList(kCampaignsKey)) {
    ParseCampaigns(*value, installed_dir);
  }
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
            MaybeParseCampaign(*dict, installed_dir)) {
      campaigns.push_back(*campaign);
    }
  }
}

bool NTPSponsoredImagesData::IsValid() const {
  return !campaigns.empty();
}

const Creative* NTPSponsoredImagesData::GetCreativeByInstanceId(
    const std::string& creative_instance_id) const {
  // TODO(https://github.com/brave/brave-browser/issues/49222):
  // Use a map-based lookup for creatives to improve performance.
  for (const Campaign& campaign : campaigns) {
    for (const Creative& creative : campaign.creatives) {
      if (creative.creative_instance_id == creative_instance_id) {
        return &creative;
      }
    }
  }
  SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                            creative_instance_id);
  SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                            "Failed to get creative by instance id");
  base::debug::DumpWithoutCrashing();
  return nullptr;
}

std::optional<base::Value::Dict> NTPSponsoredImagesData::MaybeGetBackgroundAt(
    size_t campaign_index,
    size_t creative_index) const {
  CHECK(campaign_index < campaigns.size());
  CHECK(creative_index < campaigns[campaign_index].creatives.size());

  const Campaign& campaign = campaigns[campaign_index];

  const Creative& creative = campaign.creatives[creative_index];

  const std::optional<std::string> wallpaper_type =
      ToString(creative.wallpaper_type);
  if (!wallpaper_type) {
    // Unknown wallpaper type.
    return std::nullopt;
  }

  return base::Value::Dict()
      .Set(kCampaignIdKey, campaign.campaign_id)
      .Set(kCreativeInstanceIDKey, creative.creative_instance_id)
      .Set(kIsSponsoredKey, true)
      .Set(kIsBackgroundKey, false)
      .Set(kWallpaperIDKey, base::Uuid::GenerateRandomV4().AsLowercaseString())
      .Set(kWallpaperMetricTypeKey, static_cast<int>(creative.metric_type))
      .Set(kWallpaperURLKey, creative.url.spec())
      .Set(kWallpaperFilePathKey, creative.file_path.AsUTF8Unsafe())
      .Set(kWallpaperFocalPointXKey, creative.focal_point.x())
      .Set(kWallpaperFocalPointYKey, creative.focal_point.y())
      .Set(kLogoKey,
           base::Value::Dict()
               .Set(kImageKey, creative.logo.image_url)
               .Set(kImagePathKey, creative.logo.image_file.AsUTF8Unsafe())
               .Set(kCompanyNameKey, creative.logo.company_name)
               .Set(kAltKey, creative.logo.alt_text)
               .Set(kDestinationURLKey, creative.logo.destination_url))
      .Set(kWallpaperTypeKey, *wallpaper_type);
}

std::optional<base::Value::Dict> NTPSponsoredImagesData::MaybeGetBackground(
    const brave_ads::NewTabPageAdInfo& ad) {
  // Find campaign
  size_t campaign_index = 0;
  for (; campaign_index != campaigns.size(); ++campaign_index) {
    if (campaigns[campaign_index].campaign_id == ad.campaign_id) {
      break;
    }
  }
  if (campaign_index == campaigns.size()) {
    VLOG(0) << "The ad campaign wasn't found in the NTP sponsored images data: "
            << ad.campaign_id;
    return std::nullopt;
  }

  const auto& creatives = campaigns[campaign_index].creatives;
  size_t creative_index = 0;
  for (; creative_index != creatives.size(); ++creative_index) {
    if (creatives[creative_index].creative_instance_id ==
        ad.creative_instance_id) {
      break;
    }
  }
  if (creative_index == creatives.size()) {
    VLOG(0) << "Creative instance wasn't found in NTP sponsored images data: "
            << ad.creative_instance_id;
    return std::nullopt;
  }

  std::optional<base::Value::Dict> dict =
      MaybeGetBackgroundAt(campaign_index, creative_index);
  if (dict) {
    dict->Set(kWallpaperIDKey, ad.placement_id);
  }
  return dict;
}

}  // namespace ntp_background_images
