/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/bundle.h"

#include <functional>
#include <limits>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/bundle_state.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/database/tables/ad_conversions_database_table.h"
#include "bat/ads/internal/database/tables/campaigns_database_table.h"
#include "bat/ads/internal/database/tables/categories_database_table.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/database/tables/creative_ads_database_table.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using std::placeholders::_1;

Bundle::Bundle(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Bundle::~Bundle() = default;

bool Bundle::UpdateFromCatalog(
    const Catalog& catalog) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  auto bundle_state = GenerateFromCatalog(catalog);
  if (!bundle_state) {
    return false;
  }

  catalog_id_ = bundle_state->catalog_id;
  catalog_version_ = bundle_state->catalog_version;
  catalog_ping_ = bundle_state->catalog_ping;
  catalog_last_updated_ = bundle_state->catalog_last_updated;

  // TODO(https://github.com/brave/brave-browser/issues/3661): Merge in diffs
  // to Brave Ads catalog instead of rebuilding the database
  DeleteCreativeAdNotifications();
  DeleteCreativeNewTabPageAds();
  DeleteCampaigns();
  DeleteCategories();
  DeleteCreativeAds();
  DeleteDayparts();
  DeleteGeoTargets();

  SaveCreativeAdNotifications(bundle_state->creative_ad_notifications);
  SaveCreativeNewTabPageAds(bundle_state->creative_new_tab_page_ads);
  SaveAdConversions(bundle_state->ad_conversions);

  return true;
}

std::string Bundle::GetCatalogId() const {
  return catalog_id_;
}

uint64_t Bundle::GetCatalogVersion() const {
  return catalog_version_;
}

uint64_t Bundle::GetCatalogPing() const {
  return catalog_ping_ / base::Time::kMillisecondsPerSecond;
}

void Bundle::DeleteCreativeAdNotifications() {
  database::table::CreativeAdNotifications database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnCreativeAdNotificationsDeleted,
      this, _1));
}

void Bundle::DeleteCreativeNewTabPageAds() {
  database::table::CreativeNewTabPageAds database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnCreativeNewTabPageAdsDeleted,
      this, _1));
}

void Bundle::DeleteCampaigns() {
  database::table::Campaigns database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnCampaignsDeleted, this, _1));
}

void Bundle::DeleteCategories() {
  database::table::Categories database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnCategoriesDeleted, this, _1));
}

void Bundle::DeleteCreativeAds() {
  database::table::CreativeAds database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnCreativeAdsDeleted, this, _1));
}

void Bundle::DeleteDayparts() {
  database::table::Dayparts database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnDaypartsDeleted, this, _1));
}

void Bundle::DeleteGeoTargets() {
  database::table::GeoTargets database_table(ads_);
  database_table.Delete(std::bind(&Bundle::OnGeoTargetsDeleted, this, _1));
}

void Bundle::SaveCreativeAdNotifications(
    const CreativeAdNotificationList& creative_ad_notifications) {
  database::table::CreativeAdNotifications database_table(ads_);

  database_table.Save(creative_ad_notifications,
      std::bind(&Bundle::OnCreativeAdNotificationsSaved, this, _1));
}

void Bundle::SaveCreativeNewTabPageAds(
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  database::table::CreativeNewTabPageAds database_table(ads_);

  database_table.Save(creative_new_tab_page_ads,
      std::bind(&Bundle::OnCreativeNewTabPageAdsSaved, this, _1));
}

void Bundle::SaveAdConversions(
    const AdConversionList& ad_conversions) {
  database::table::AdConversions database_table(ads_);

  database_table.PurgeExpiredAdConversions(
      std::bind(&Bundle::OnPurgedExpiredAdConversions, this, _1));

  database_table.Save(ad_conversions,
      std::bind(&Bundle::OnAdConversionsSaved, this, _1));
}

bool Bundle::IsOlderThanOneDay() const {
  const base::Time now = base::Time::Now();

  if (now > catalog_last_updated_ + base::TimeDelta::FromDays(1)) {
    return true;
  }

  return false;
}

bool Bundle::Exists() const {
  if (GetCatalogVersion() == 0) {
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

// TODO(Terry Mancey): We should consider optimizing memory consumption when
// generating the bundle by saving each campaign individually on the Client
std::unique_ptr<BundleState> Bundle::GenerateFromCatalog(
    const Catalog& catalog) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  CreativeAdNotificationList creative_ad_notifications;
  CreativeNewTabPageAdList creative_new_tab_page_ads;
  AdConversionList ad_conversions;

  // Campaigns
  for (const auto& campaign : catalog.GetCampaigns()) {
    // Geo Targets
    std::vector<std::string> geo_targets;
    for (const auto& geo_target : campaign.geo_targets) {
      std::string code = geo_target.code;

      if (std::find(geo_targets.begin(), geo_targets.end(), code)
          != geo_targets.end()) {
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
        if (base::Time::FromUTCString(campaign.end_at.c_str(),
            &end_at_time)) {
          info.end_at_timestamp =
              static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
              << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion =
            creative_set.ad_conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
        info.total_max = creative_set.total_max;
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

          info.category = segment_name;
          creative_ad_notifications.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.category = top_level_segment_name;
            creative_ad_notifications.push_back(info);
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
        if (base::Time::FromUTCString(campaign.end_at.c_str(),
            &end_at_time)) {
          info.end_at_timestamp =
              static_cast<int64_t>(end_at_time.ToDoubleT());
        } else {
          info.end_at_timestamp = std::numeric_limits<int64_t>::max();

          BLOG(1, "Creative set id " << creative_set.creative_set_id
              << " has an invalid endAt timestamp");
        }

        info.daily_cap = campaign.daily_cap;
        info.advertiser_id = campaign.advertiser_id;
        info.priority = campaign.priority;
        info.ptr = campaign.ptr;
        info.conversion =
            creative_set.ad_conversions.size() != 0 ? true : false;
        info.per_day = creative_set.per_day;
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

          info.category = segment_name;
          creative_new_tab_page_ads.push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            info.category = top_level_segment_name;
            creative_new_tab_page_ads.push_back(info);
            entries++;
          }
        }
      }

      if (entries == 0) {
        BLOG(1, "creative set id " << creative_set.creative_set_id
            << " has no entries");

        continue;
      }

      // Ad conversions
      ad_conversions.insert(ad_conversions.end(),
          creative_set.ad_conversions.begin(),
              creative_set.ad_conversions.end());
    }
  }

  auto state = std::make_unique<BundleState>();
  state->catalog_id = catalog.GetId();
  state->catalog_version = catalog.GetVersion();
  state->catalog_ping = catalog.GetPing();
  state->catalog_last_updated = base::Time::Now();
  state->creative_ad_notifications = creative_ad_notifications;
  state->creative_new_tab_page_ads = creative_new_tab_page_ads;
  state->ad_conversions = ad_conversions;

  return state;
}

bool Bundle::DoesOsSupportCreativeSet(
    const CatalogCreativeSetInfo& creative_set) {
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

void Bundle::OnCreativeAdNotificationsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete creative ad notifications");
    return;
  }

  BLOG(3, "Successfully deleted creative ad notifications");
}

void Bundle::OnCreativeNewTabPageAdsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete new tab page ads");
    return;
  }

  BLOG(3, "Successfully deleted new tab page ads");
}

void Bundle::OnCampaignsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete campaigns");
    return;
  }

  BLOG(3, "Successfully deleted campaigns");
}

void Bundle::OnCategoriesDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete categories");
    return;
  }

  BLOG(3, "Successfully deleted categories");
}

void Bundle::OnCreativeAdsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete creative ads");
    return;
  }

  BLOG(3, "Successfully deleted creative ads");
}

void Bundle::OnDaypartsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete dayparts");
    return;
  }

  BLOG(3, "Successfully deleted dayparts");
}

void Bundle::OnGeoTargetsDeleted(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to delete geo targets");
    return;
  }

  BLOG(3, "Successfully deleted geo targets");
}

void Bundle::OnCreativeAdNotificationsSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save creative ad notifications state");
    return;
  }

  BLOG(3, "Successfully saved creative ad notifications state");
}

void Bundle::OnCreativeNewTabPageAdsSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save creative new tab page ads state");
    return;
  }

  BLOG(3, "Successfully saved creative new tab page ads state");
}

void Bundle::OnPurgedExpiredAdConversions(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to purge expired ad conversions");
    return;
  }

  BLOG(3, "Successfully purged expired ad conversions");
}

void Bundle::OnAdConversionsSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save ad conversions state");
    return;
  }

  BLOG(3, "Successfully saved ad conversions state");
}

}  // namespace ads
