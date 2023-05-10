/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/extend.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"

namespace brave_ads {

// TODO(https://github.com/brave/brave-browser/issues/24938): Reduce cognitive
// complexity.
CreativesInfo BuildCreatives(const CatalogInfo& catalog) {
  CreativesInfo creatives;

  // Campaigns
  for (const auto& campaign : catalog.campaigns) {
    // Geo Targets
    base::flat_set<std::string> geo_targets;
    for (const auto& geo_target : campaign.geo_targets) {
      const std::string code = geo_target.code;

      if (base::Contains(geo_targets, code)) {
        continue;
      }

      geo_targets.insert(code);
    }

    // Dayparts
    CreativeDaypartList creative_dayparts;
    for (const auto& daypart : campaign.dayparts) {
      CreativeDaypartInfo creative_daypart;
      creative_daypart.days_of_week = daypart.days_of_week;
      creative_daypart.start_minute = daypart.start_minute;
      creative_daypart.end_minute = daypart.end_minute;

      creative_dayparts.push_back(creative_daypart);
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      int entries = 0;

      // Operating system
      if (!creative_set.DoesSupportOS()) {
        const std::string platform_name =
            PlatformHelper::GetInstance().GetName();

        BLOG(1, "Creative set id " << creative_set.creative_set_id
                                   << " does not support " << platform_name);

        continue;
      }

      // Notification ad creatives
      for (const auto& creative : creative_set.creative_notification_ads) {
        CreativeNotificationAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.creative_instance_id;
        creative_ad.creative_set_id = creative_set.creative_set_id;
        creative_ad.campaign_id = campaign.campaign_id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          creative_ad.start_at = base::Time();
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          creative_ad.end_at = base::Time();
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.ptr = campaign.ptr;
        creative_ad.has_conversion = !creative_set.conversions.empty();
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.embedding = creative_set.embedding;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = creative_dayparts;
        creative_ad.geo_targets = geo_targets;
        creative_ad.target_url = creative.payload.target_url;

        creative_ad.title = creative.payload.title;
        creative_ad.body = creative.payload.body;

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.notification_ads.push_back(creative_ad);
          entries++;

          const std::string top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.notification_ads.push_back(creative_ad);
            entries++;
          }
        }
      }

      // inline content ad creatives
      for (const auto& creative : creative_set.creative_inline_content_ads) {
        CreativeInlineContentAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.creative_instance_id;
        creative_ad.creative_set_id = creative_set.creative_set_id;
        creative_ad.campaign_id = campaign.campaign_id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          creative_ad.start_at = base::Time();
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          creative_ad.end_at = base::Time();
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.ptr = campaign.ptr;
        creative_ad.has_conversion = !creative_set.conversions.empty();
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = creative_dayparts;
        creative_ad.geo_targets = geo_targets;
        creative_ad.target_url = creative.payload.target_url;

        creative_ad.title = creative.payload.title;
        creative_ad.description = creative.payload.description;
        creative_ad.image_url = creative.payload.image_url;
        creative_ad.dimensions = creative.payload.dimensions;
        creative_ad.cta_text = creative.payload.cta_text;

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.inline_content_ads.push_back(creative_ad);
          entries++;

          const std::string top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.inline_content_ads.push_back(creative_ad);
            entries++;
          }
        }
      }

      // New tab page ad creatives
      for (const auto& creative : creative_set.creative_new_tab_page_ads) {
        CreativeNewTabPageAdInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &info.start_at)) {
          info.start_at = base::Time();
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(), &info.end_at)) {
          info.end_at = base::Time();
        }
        info.daily_cap = campaign.daily_cap;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.has_conversion = !creative_set.conversions.empty();
        info.per_day = creative_set.per_day;
        info.per_week = creative_set.per_week;
        info.per_month = creative_set.per_month;
        info.total_max = creative_set.total_max;
        info.value = creative_set.value;
        info.split_test_group = creative_set.split_test_group;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.target_url = creative.payload.target_url;

        info.company_name = creative.payload.company_name;
        info.image_url = creative.payload.image_url;
        info.alt = creative.payload.alt;

        CHECK(!creative.payload.wallpapers.empty());
        for (const auto& catalog_new_tab_page_ad_wallpaper :
             creative.payload.wallpapers) {
          CreativeNewTabPageAdWallpaperInfo wallpaper;

          wallpaper.image_url = catalog_new_tab_page_ad_wallpaper.image_url;

          CreativeNewTabPageAdWallpaperFocalPointInfo focal_point;
          focal_point.x = catalog_new_tab_page_ad_wallpaper.focal_point.x;
          focal_point.y = catalog_new_tab_page_ad_wallpaper.focal_point.y;
          wallpaper.focal_point = focal_point;

          info.wallpapers.push_back(wallpaper);
        }

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creatives.new_tab_page_ads.push_back(info);
          entries++;

          const std::string top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creatives.new_tab_page_ads.push_back(info);
            entries++;
          }
        }
      }

      // Promoted content ad creatives
      for (const auto& creative : creative_set.creative_promoted_content_ads) {
        CreativePromotedContentAdInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &info.start_at)) {
          info.start_at = base::Time();
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(), &info.end_at)) {
          info.end_at = base::Time();
        }
        info.daily_cap = campaign.daily_cap;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.has_conversion = !creative_set.conversions.empty();
        info.per_day = creative_set.per_day;
        info.per_week = creative_set.per_week;
        info.per_month = creative_set.per_month;
        info.total_max = creative_set.total_max;
        info.value = creative_set.value;
        info.split_test_group = creative_set.split_test_group;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.target_url = creative.payload.target_url;

        info.title = creative.payload.title;
        info.description = creative.payload.description;

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creatives.promoted_content_ads.push_back(info);
          entries++;

          const std::string top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creatives.promoted_content_ads.push_back(info);
            entries++;
          }
        }
      }

      if (entries == 0) {
        BLOG(1, "Creative set id " << creative_set.creative_set_id
                                   << " has no entries");

        continue;
      }

      // Conversions
      base::Extend(creatives.conversions, creative_set.conversions);
    }
  }

  return creatives;
}

}  // namespace brave_ads
