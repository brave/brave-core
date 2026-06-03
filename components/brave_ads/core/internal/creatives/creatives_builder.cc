/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"

#include <string>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_campaign_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_creative_set_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

namespace {

base::flat_set<std::string> BuildGeoTargets(
    const CatalogCampaignInfo& campaign) {
  base::flat_set<std::string> geo_targets;
  for (const auto& geo_target : campaign.geo_targets) {
    geo_targets.insert(geo_target.code);
  }
  return geo_targets;
}

CreativeDaypartSet BuildDayparts(const CatalogCampaignInfo& campaign) {
  CreativeDaypartSet dayparts;
  dayparts.reserve(campaign.dayparts.size());
  for (const auto& daypart : campaign.dayparts) {
    dayparts.insert({.days_of_week = daypart.days_of_week,
                     .start_minute = daypart.start_minute,
                     .end_minute = daypart.end_minute});
  }
  return dayparts;
}

CreativeNotificationAdList BuildNotificationAdsFromCreativeSet(
    const CatalogCreativeSetInfo& creative_set,
    const CatalogCampaignInfo& campaign,
    const base::flat_set<std::string>& geo_targets,
    const CreativeDaypartSet& dayparts) {
  CreativeNotificationAdList notification_ads;
  for (const auto& creative : creative_set.creative_notification_ads) {
    CreativeNotificationAdInfo creative_ad;
    creative_ad.creative_instance_id = creative.instance_id;
    creative_ad.creative_set_id = creative_set.id;
    creative_ad.campaign_id = campaign.id;
    creative_ad.advertiser_id = campaign.advertiser_id;
    creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;
    creative_ad.start_at = campaign.start_at;
    creative_ad.end_at = campaign.end_at;
    creative_ad.daily_cap = campaign.daily_cap;
    creative_ad.priority = campaign.priority;
    creative_ad.pass_through_rate = campaign.pass_through_rate;
    creative_ad.per_day = creative_set.per_day;
    creative_ad.per_week = creative_set.per_week;
    creative_ad.per_month = creative_set.per_month;
    creative_ad.total_max = creative_set.total_max;
    creative_ad.value = creative_set.value;
    creative_ad.dayparts = dayparts;
    creative_ad.geo_targets = geo_targets;
    creative_ad.target_url = creative.payload.target_url;
    creative_ad.title = creative.payload.title;
    creative_ad.body = creative.payload.body;
    for (const auto& segment : creative_set.segments) {
      CHECK(!segment.name.empty());
      creative_ad.segment = base::ToLowerASCII(segment.name);
      notification_ads.push_back(creative_ad);
    }
  }
  return notification_ads;
}

CreativeSetConversionList BuildConversionsFromCreativeSet(
    const CatalogCreativeSetInfo& creative_set) {
  CreativeSetConversionList conversions;
  for (const auto& conversion : creative_set.conversions) {
    CreativeSetConversionInfo creative_set_conversion;
    creative_set_conversion.id = conversion.creative_set_id;
    creative_set_conversion.url_pattern = conversion.url_pattern;
    creative_set_conversion.observation_window = conversion.observation_window;
    creative_set_conversion.expire_at = conversion.expire_at;
    if (!creative_set_conversion.IsValid()) {
      BLOG(1, "Creative set id " << creative_set.id
                                 << " has an invalid conversion");
      continue;
    }
    conversions.push_back(creative_set_conversion);
  }
  return conversions;
}

}  // namespace

CreativesInfo BuildCreatives(const CatalogInfo& catalog) {
  CreativesInfo creatives;

  for (const auto& campaign : catalog.campaigns) {
    const base::flat_set<std::string> geo_targets = BuildGeoTargets(campaign);
    const CreativeDaypartSet dayparts = BuildDayparts(campaign);

    for (const auto& creative_set : campaign.creative_sets) {
      if (!creative_set.DoesSupportOS()) {
        BLOG(1, "Creative set id " << creative_set.id << " does not support "
                                   << OperatingSystem::GetInstance().GetName());
        continue;
      }

      CreativeNotificationAdList notification_ads =
          BuildNotificationAdsFromCreativeSet(creative_set, campaign,
                                              geo_targets, dayparts);
      if (notification_ads.empty()) {
        BLOG(1, "Creative set id " << creative_set.id << " has no entries");
        continue;
      }

      base::Extend(creatives.notification_ads, std::move(notification_ads));

      CreativeSetConversionList conversions =
          BuildConversionsFromCreativeSet(creative_set);
      base::Extend(creatives.conversions, std::move(conversions));
    }
  }

  return creatives;
}

}  // namespace brave_ads
