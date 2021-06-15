/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/bundle.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "bat/ads/internal/bundle/bundle_state.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_creative_set_info.h"
#include "bat/ads/internal/database/tables/campaigns_database_table.h"
#include "bat/ads/internal/database/tables/conversions_database_table.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/database/tables/creative_ads_database_table.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/database/tables/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/database/tables/segments_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/result.h"

namespace ads {

namespace {

bool DoesOsSupportCreativeSet(const CatalogCreativeSetInfo& creative_set) {
  if (creative_set.oses.empty()) {
    // Creative set supports all OSes
    return true;
  }

  const std::string platform_name =
      PlatformHelper::GetInstance()->GetPlatformName();

  for (const auto& os : creative_set.oses) {
    if (os.name == platform_name) {
      return true;
    }
  }

  return false;
}

}  // namespace

Bundle::Bundle() = default;

Bundle::~Bundle() = default;

void Bundle::BuildFromCatalog(const Catalog& catalog) {
  const BundleState bundle_state = FromCatalog(catalog);

  // TODO(https://github.com/brave/brave-browser/issues/3661): Merge in diffs
  // to Brave Ads catalog instead of rebuilding the database
  DeleteDatabaseTables();

  SaveCreativeAdNotifications(bundle_state.creative_ad_notifications);
  SaveCreativeInlineContentAds(bundle_state.creative_inline_content_ads);
  SaveCreativeNewTabPageAds(bundle_state.creative_new_tab_page_ads);
  SaveCreativePromotedContentAds(bundle_state.creative_promoted_content_ads);

  PurgeExpiredConversions();
  SaveConversions(bundle_state.conversions);
}

///////////////////////////////////////////////////////////////////////////////

BundleState Bundle::FromCatalog(const Catalog& catalog) const {
  CreativeAdNotificationList creative_ad_notifications;
  CreativeInlineContentAdList creative_inline_content_ads;
  CreativeNewTabPageAdList creative_new_tab_page_ads;
  CreativePromotedContentAdList creative_promoted_content_ads;
  ConversionList conversions;

  // Campaigns
  for (const auto& campaign : catalog.GetCampaigns()) {
    // Geo Targets
    std::vector<std::string> geo_targets;
    for (const auto& geo_target : campaign.geo_targets) {
      std::string code = geo_target.code;

      if (std::find(geo_targets.begin(), geo_targets.end(), code) !=
          geo_targets.end()) {
        continue;
      }

      geo_targets.push_back(code);
    }

    std::vector<CreativeDaypartInfo> creative_dayparts;
    for (const auto& daypart : campaign.dayparts) {
      CreativeDaypartInfo creative_daypart_info;
      creative_daypart_info.dow = daypart.dow;
      creative_daypart_info.start_minute = daypart.start_minute;
      creative_daypart_info.end_minute = daypart.end_minute;

      creative_dayparts.push_back(creative_daypart_info);
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      uint64_t entries = 0;

      // Ad notification creatives
      for (const auto& creative : creative_set.creative_ad_notifications) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          const std::string platform_name =
              PlatformHelper::GetInstance()->GetPlatformName();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " does not support " << platform_name);

          continue;
        }

        CreativeAdNotificationInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;

        base::Time start_at_time;
        if (base::Time::FromUTCString(campaign.start_at.c_str(),
                                      &start_at_time)) {
          info.start_at_timestamp =
              static_cast<int64_t>(start_at_time.ToDoubleT());
        } else {
          info.start_at_timestamp = std::numeric_limits<int64_t>::min();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid startAt timestamp");
        }

        base::Time end_at_time;
        if (base::Time::FromUTCString(campaign.end_at.c_str(), &end_at_time)) {
          info.end_at_timestamp = static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion = creative_set.conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
        info.per_week = creative_set.per_week;
        info.per_month = creative_set.per_month;
        info.total_max = creative_set.total_max;
        info.split_test_group = creative_set.split_test_group;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.title = creative.payload.title;
        info.body = creative.payload.body;
        info.target_url = creative.payload.target_url;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creative_ad_notifications.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creative_ad_notifications.push_back(info);
            entries++;
          }
        }
      }

      // inline content ad creatives
      for (const auto& creative : creative_set.creative_inline_content_ads) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          const std::string platform_name =
              PlatformHelper::GetInstance()->GetPlatformName();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " does not support " << platform_name);

          continue;
        }

        CreativeInlineContentAdInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;

        base::Time start_at_time;
        if (base::Time::FromUTCString(campaign.start_at.c_str(),
                                      &start_at_time)) {
          info.start_at_timestamp =
              static_cast<int64_t>(start_at_time.ToDoubleT());
        } else {
          info.start_at_timestamp = std::numeric_limits<int64_t>::min();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid startAt timestamp");
        }

        base::Time end_at_time;
        if (base::Time::FromUTCString(campaign.end_at.c_str(), &end_at_time)) {
          info.end_at_timestamp = static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion = creative_set.conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
        info.total_max = creative_set.total_max;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.title = creative.payload.title;
        info.description = creative.payload.description;
        info.image_url = creative.payload.image_url;
        info.dimensions = creative.payload.dimensions;
        info.cta_text = creative.payload.cta_text;
        info.target_url = creative.payload.target_url;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creative_inline_content_ads.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creative_inline_content_ads.push_back(info);
            entries++;
          }
        }
      }

      // New tab page ad creatives
      for (const auto& creative : creative_set.creative_new_tab_page_ads) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          const std::string platform_name =
              PlatformHelper::GetInstance()->GetPlatformName();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " does not support " << platform_name);

          continue;
        }

        CreativeNewTabPageAdInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;

        base::Time start_at_time;
        if (base::Time::FromUTCString(campaign.start_at.c_str(),
                                      &start_at_time)) {
          info.start_at_timestamp =
              static_cast<int64_t>(start_at_time.ToDoubleT());
        } else {
          info.start_at_timestamp = std::numeric_limits<int64_t>::min();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid startAt timestamp");
        }

        base::Time end_at_time;
        if (base::Time::FromUTCString(campaign.end_at.c_str(), &end_at_time)) {
          info.end_at_timestamp = static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion = creative_set.conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
        info.per_week = creative_set.per_week;
        info.per_month = creative_set.per_month;
        info.total_max = creative_set.total_max;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.company_name = creative.payload.company_name;
        info.alt = creative.payload.alt;
        info.target_url = creative.payload.target_url;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creative_new_tab_page_ads.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creative_new_tab_page_ads.push_back(info);
            entries++;
          }
        }
      }

      // Promoted content ad creatives
      for (const auto& creative : creative_set.creative_promoted_content_ads) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          const std::string platform_name =
              PlatformHelper::GetInstance()->GetPlatformName();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " does not support " << platform_name);

          continue;
        }

        CreativePromotedContentAdInfo info;
        info.creative_instance_id = creative.creative_instance_id;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.advertiser_id = campaign.advertiser_id;

        base::Time start_at_time;
        if (base::Time::FromUTCString(campaign.start_at.c_str(),
                                      &start_at_time)) {
          info.start_at_timestamp =
              static_cast<int64_t>(start_at_time.ToDoubleT());
        } else {
          info.start_at_timestamp = std::numeric_limits<int64_t>::min();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid startAt timestamp");
        }

        base::Time end_at_time;
        if (base::Time::FromUTCString(campaign.end_at.c_str(), &end_at_time)) {
          info.end_at_timestamp = static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
                                     << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion = creative_set.conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
        info.total_max = creative_set.total_max;
        info.dayparts = creative_dayparts;
        info.geo_targets = geo_targets;
        info.title = creative.payload.title;
        info.description = creative.payload.description;
        info.target_url = creative.payload.target_url;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(1, "creative set id " << creative_set.creative_set_id
                                       << " segment name should not be empty");

            continue;
          }

          info.segment = segment_name;
          creative_promoted_content_ads.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.segment = top_level_segment_name;
            creative_promoted_content_ads.push_back(info);
            entries++;
          }
        }
      }

      if (entries == 0) {
        BLOG(1, "creative set id " << creative_set.creative_set_id
                                   << " has no entries");

        continue;
      }

      // Conversions
      conversions.insert(conversions.end(), creative_set.conversions.begin(),
                         creative_set.conversions.end());
    }
  }

  BundleState bundle_state;
  bundle_state.creative_ad_notifications = creative_ad_notifications;
  bundle_state.creative_inline_content_ads = creative_inline_content_ads;
  bundle_state.creative_new_tab_page_ads = creative_new_tab_page_ads;
  bundle_state.creative_promoted_content_ads = creative_promoted_content_ads;
  bundle_state.conversions = conversions;

  return bundle_state;
}

void Bundle::DeleteDatabaseTables() {
  DeleteCreativeAdNotifications();
  DeleteCreativeInlineContentAds();
  DeleteCreativeNewTabPageAds();
  DeleteCreativePromotedContentAds();
  DeleteCampaigns();
  DeleteSegments();
  DeleteCreativeAds();
  DeleteDayparts();
  DeleteGeoTargets();
}

void Bundle::DeleteCreativeAdNotifications() {
  database::table::CreativeAdNotifications database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete creative ad notifications state");
      return;
    }

    BLOG(3, "Successfully deleted creative ad notifications state");
  });
}

void Bundle::DeleteCreativeInlineContentAds() {
  database::table::CreativeInlineContentAds database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete creative inline content ads state");
      return;
    }

    BLOG(3, "Successfully deleted creative inline content ads state");
  });
}

void Bundle::DeleteCreativeNewTabPageAds() {
  database::table::CreativeNewTabPageAds database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete creative new tab page ads state");
      return;
    }

    BLOG(3, "Successfully deleted creative new tab page ads state");
  });
}

void Bundle::DeleteCreativePromotedContentAds() {
  database::table::CreativePromotedContentAds database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete creative promoted content ads state");
      return;
    }

    BLOG(3, "Successfully deleted creative promoted content ads state");
  });
}

void Bundle::DeleteCampaigns() {
  database::table::Campaigns database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete campaigns state");
      return;
    }

    BLOG(3, "Successfully deleted campaigns state");
  });
}

void Bundle::DeleteSegments() {
  database::table::Segments database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete segments state");
      return;
    }

    BLOG(3, "Successfully deleted segments state");
  });
}

void Bundle::DeleteCreativeAds() {
  database::table::CreativeAds database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete creative ads state");
      return;
    }

    BLOG(3, "Successfully deleted creative ads state");
  });
}

void Bundle::DeleteDayparts() {
  database::table::Dayparts database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete dayparts state");
      return;
    }

    BLOG(3, "Successfully deleted dayparts state");
  });
}

void Bundle::DeleteGeoTargets() {
  database::table::GeoTargets database_table;
  database_table.Delete([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to delete geo targets state");
      return;
    }

    BLOG(3, "Successfully deleted geo targets state");
  });
}

void Bundle::SaveCreativeAdNotifications(
    const CreativeAdNotificationList& creative_ad_notifications) {
  database::table::CreativeAdNotifications database_table;

  database_table.Save(creative_ad_notifications, [](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to save creative ad notifications state");
      return;
    }

    BLOG(3, "Successfully saved creative ad notifications state");
  });
}

void Bundle::SaveCreativeInlineContentAds(
    const CreativeInlineContentAdList& creative_inline_content_ads) {
  database::table::CreativeInlineContentAds database_table;

  database_table.Save(creative_inline_content_ads, [](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to save creative inline content ads state");
      return;
    }

    BLOG(3, "Successfully saved creative inline content ads state");
  });
}

void Bundle::SaveCreativeNewTabPageAds(
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  database::table::CreativeNewTabPageAds database_table;

  database_table.Save(creative_new_tab_page_ads, [](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to save creative new tab page ads state");
      return;
    }

    BLOG(3, "Successfully saved creative new tab page ads state");
  });
}

void Bundle::SaveCreativePromotedContentAds(
    const CreativePromotedContentAdList& creative_promoted_content_ads) {
  database::table::CreativePromotedContentAds database_table;

  database_table.Save(creative_promoted_content_ads, [](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to save creative promoted content ads state");
      return;
    }

    BLOG(3, "Successfully saved creative promoted content ads state");
  });
}

void Bundle::PurgeExpiredConversions() {
  database::table::Conversions database_table;
  database_table.PurgeExpired([](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to purge expired conversions");
      return;
    }

    BLOG(3, "Successfully purged expired conversions");
  });
}

void Bundle::SaveConversions(const ConversionList& conversions) {
  database::table::Conversions database_table;
  database_table.Save(conversions, [](const Result result) {
    if (result != SUCCESS) {
      BLOG(0, "Failed to save conversions state");
      return;
    }

    BLOG(3, "Successfully saved conversions state");
  });
}

}  // namespace ads
