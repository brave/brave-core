/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
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
      geo_targets.insert(geo_target.code);
    }

    // Dayparts
    CreativeDaypartList dayparts;
    dayparts.reserve(campaign.dayparts.size());
    for (const auto& daypart : campaign.dayparts) {
      dayparts.push_back(
          CreativeDaypartInfo{.days_of_week = daypart.days_of_week,
                              .start_minute = daypart.start_minute,
                              .end_minute = daypart.end_minute});
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      int entries = 0;

      // Operating system
      if (!creative_set.DoesSupportOS()) {
        const std::string platform_name =
            PlatformHelper::GetInstance().GetName();

        BLOG(1, "Creative set id " << creative_set.id << " does not support "
                                   << platform_name);

        continue;
      }

      // Notification ad creatives
      for (const auto& creative : creative_set.creative_notification_ads) {
        CreativeNotificationAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.instance_id;
        creative_ad.creative_set_id = creative_set.id;
        creative_ad.campaign_id = campaign.id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          BLOG(1, "Campaign id " << campaign.id
                                 << " has an invalid start at time");
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          BLOG(1,
               "Campaign id " << campaign.id << " has an invalid end at time");
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.pass_through_rate = campaign.pass_through_rate;
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = dayparts;
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
            BLOG(1, "creative set id " << creative_set.id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.notification_ads.push_back(creative_ad);
          ++entries;

          const std::string& top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.notification_ads.push_back(creative_ad);
            ++entries;
          }
        }
      }

      // Inline content ad creatives
      for (const auto& creative : creative_set.creative_inline_content_ads) {
        CreativeInlineContentAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.instance_id;
        creative_ad.creative_set_id = creative_set.id;
        creative_ad.campaign_id = campaign.id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          BLOG(1, "Campaign id " << campaign.id
                                 << " has an invalid start at time");
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          BLOG(1,
               "Campaign id " << campaign.id << " has an invalid end at time");
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.pass_through_rate = campaign.pass_through_rate;
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = dayparts;
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
            BLOG(1, "creative set id " << creative_set.id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.inline_content_ads.push_back(creative_ad);
          ++entries;

          const std::string& top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.inline_content_ads.push_back(creative_ad);
            ++entries;
          }
        }
      }

      // New tab page ad creatives
      for (const auto& creative : creative_set.creative_new_tab_page_ads) {
        CreativeNewTabPageAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.instance_id;
        creative_ad.creative_set_id = creative_set.id;
        creative_ad.campaign_id = campaign.id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          BLOG(1, "Campaign id " << campaign.id
                                 << " has an invalid start at time");
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          BLOG(1,
               "Campaign id " << campaign.id << " has an invalid end at time");
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.pass_through_rate = campaign.pass_through_rate;
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = dayparts;
        creative_ad.geo_targets = geo_targets;
        creative_ad.target_url = creative.payload.target_url;

        creative_ad.company_name = creative.payload.company_name;
        creative_ad.image_url = creative.payload.image_url;
        creative_ad.alt = creative.payload.alt;

        CHECK(!creative.payload.wallpapers.empty());
        for (const auto& catalog_new_tab_page_ad_wallpaper :
             creative.payload.wallpapers) {
          CreativeNewTabPageAdWallpaperInfo wallpaper;
          wallpaper.image_url = catalog_new_tab_page_ad_wallpaper.image_url;
          wallpaper.focal_point = CreativeNewTabPageAdWallpaperFocalPointInfo{
              .x = catalog_new_tab_page_ad_wallpaper.focal_point.x,
              .y = catalog_new_tab_page_ad_wallpaper.focal_point.y};
          wallpaper.condition_matchers =
              catalog_new_tab_page_ad_wallpaper.condition_matchers;
          creative_ad.wallpapers.push_back(wallpaper);
        }

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.new_tab_page_ads.push_back(creative_ad);
          ++entries;

          const std::string& top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.new_tab_page_ads.push_back(creative_ad);
            ++entries;
          }
        }
      }

      // Promoted content ad creatives
      for (const auto& creative : creative_set.creative_promoted_content_ads) {
        CreativePromotedContentAdInfo creative_ad;
        creative_ad.creative_instance_id = creative.instance_id;
        creative_ad.creative_set_id = creative_set.id;
        creative_ad.campaign_id = campaign.id;
        creative_ad.advertiser_id = campaign.advertiser_id;
        if (!base::Time::FromUTCString(campaign.start_at.c_str(),
                                       &creative_ad.start_at)) {
          BLOG(1, "Campaign id " << campaign.id
                                 << " has an invalid start at time");
        }
        if (!base::Time::FromUTCString(campaign.end_at.c_str(),
                                       &creative_ad.end_at)) {
          BLOG(1,
               "Campaign id " << campaign.id << " has an invalid end at time");
        }
        creative_ad.daily_cap = campaign.daily_cap;
        creative_ad.priority = campaign.priority;
        creative_ad.pass_through_rate = campaign.pass_through_rate;
        creative_ad.per_day = creative_set.per_day;
        creative_ad.per_week = creative_set.per_week;
        creative_ad.per_month = creative_set.per_month;
        creative_ad.total_max = creative_set.total_max;
        creative_ad.value = creative_set.value;
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = dayparts;
        creative_ad.geo_targets = geo_targets;
        creative_ad.target_url = creative.payload.target_url;

        creative_ad.title = creative.payload.title;
        creative_ad.description = creative.payload.description;

        // Segments
        for (const auto& segment : creative_set.segments) {
          const std::string segment_name = base::ToLowerASCII(segment.name);
          CHECK(!segment_name.empty());

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.id
                                       << " segment name should not be empty");

            continue;
          }

          creative_ad.segment = segment_name;
          creatives.promoted_content_ads.push_back(creative_ad);
          ++entries;

          const std::string& top_level_segment_name =
              segment_name_hierarchy.front();
          CHECK(!top_level_segment_name.empty());

          if (top_level_segment_name != segment_name) {
            creative_ad.segment = top_level_segment_name;
            creatives.promoted_content_ads.push_back(creative_ad);
            ++entries;
          }
        }
      }

      if (entries == 0) {
        BLOG(1, "Creative set id " << creative_set.id << " has no entries");
        continue;
      }

      // Creative set conversions
      creatives.conversions.reserve(creative_set.conversions.size());

      for (const auto& conversion : creative_set.conversions) {
        CreativeSetConversionInfo creative_set_conversion;

        creative_set_conversion.id = conversion.creative_set_id;
        creative_set_conversion.url_pattern = conversion.url_pattern;
        creative_set_conversion.verifiable_advertiser_public_key_base64 =
            conversion.verifiable_advertiser_public_key_base64;
        creative_set_conversion.observation_window =
            conversion.observation_window;
        creative_set_conversion.expire_at = conversion.expire_at;

        if (!creative_set_conversion.IsValid()) {
          BLOG(1, "Creative set id " << creative_set.id
                                     << " has an invalid conversion");

          continue;
        }

        creatives.conversions.push_back(creative_set_conversion);
      }
    }
  }

  return creatives;
}

}  // namespace brave_ads
