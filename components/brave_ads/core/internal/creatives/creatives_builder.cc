/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"

#include <string>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-data-view.h"

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
    CreativeDaypartSet dayparts;
    dayparts.reserve(campaign.dayparts.size());
    for (const auto& daypart : campaign.dayparts) {
      dayparts.insert({.days_of_week = daypart.days_of_week,
                       .start_minute = daypart.start_minute,
                       .end_minute = daypart.end_minute});
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      size_t entries = 0;

      // Operating system
      if (!creative_set.DoesSupportOS()) {
        BLOG(1, "Creative set id " << creative_set.id << " does not support "
                                   << PlatformHelper::GetInstance().GetName());
        continue;
      }

      // Notification ad creatives
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
        creative_ad.split_test_group = creative_set.split_test_group;
        creative_ad.dayparts = dayparts;
        creative_ad.geo_targets = geo_targets;
        creative_ad.target_url = creative.payload.target_url;

        creative_ad.title = creative.payload.title;
        creative_ad.body = creative.payload.body;

        // Segments
        for (const auto& segment : creative_set.segments) {
          CHECK(!segment.name.empty());

          creative_ad.segment = base::ToLowerASCII(segment.name);
          creatives.notification_ads.push_back(creative_ad);
          ++entries;
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
