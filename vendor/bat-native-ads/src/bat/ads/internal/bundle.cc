/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>
#include <map>
#include <utility>

#include "bat/ads/bundle_state.h"

#include "bat/ads/internal/bundle.h"
#include "bat/ads/internal/catalog.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/static_values.h"

#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"

using std::placeholders::_1;

namespace ads {

Bundle::Bundle(
    AdsImpl* ads,
    AdsClient* ads_client)
    : catalog_version_(0),
      catalog_ping_(0),
      catalog_last_updated_timestamp_in_seconds_(0),
      ads_(ads),
      ads_client_(ads_client) {
}

Bundle::~Bundle() = default;

bool Bundle::UpdateFromCatalog(
    const Catalog& catalog) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  auto bundle_state = GenerateFromCatalog(catalog);
  if (!bundle_state) {
    return false;
  }

  auto callback = std::bind(&Bundle::OnStateSaved,
      this, bundle_state->catalog_id, bundle_state->catalog_version,
          bundle_state->catalog_ping,
              bundle_state->catalog_last_updated_timestamp_in_seconds, _1);
  ads_client_->SaveBundleState(std::move(bundle_state), callback);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Generated bundle'

  BLOG(INFO) << "Generated bundle";

  return true;
}

void Bundle::Reset() {
  auto bundle_state = std::make_unique<BundleState>();

  auto callback = std::bind(&Bundle::OnStateReset,
      this, bundle_state->catalog_id, bundle_state->catalog_version,
          bundle_state->catalog_ping,
              bundle_state->catalog_last_updated_timestamp_in_seconds, _1);
  ads_client_->SaveBundleState(std::move(bundle_state), callback);
}

const std::string Bundle::GetCatalogId() const {
  return catalog_id_;
}

uint64_t Bundle::GetCatalogVersion() const {
  return catalog_version_;
}

uint64_t Bundle::GetCatalogPing() const {
  return catalog_ping_ / base::Time::kMillisecondsPerSecond;
}

uint64_t Bundle::GetCatalogLastUpdatedTimestampInSeconds() const {
  return catalog_last_updated_timestamp_in_seconds_;
}

bool Bundle::IsReady() const {
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

  CreativeAdNotificationCategories ad_notification_categories;
  CreativePublisherAdCategories publisher_ad_categories;
  AdConversions ad_conversions;

  // Campaigns
  for (const auto& campaign : catalog.GetCampaigns()) {
    // Geo Targets
    std::vector<std::string> regions;
    for (const auto& geo_target : campaign.geo_targets) {
      std::string code = geo_target.code;

      if (std::find(regions.begin(), regions.end(), code) != regions.end()) {
        continue;
      }

      regions.push_back(code);
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      uint64_t entries = 0;

      // Ad notification creatives
      for (const auto& creative : creative_set.ad_notification_creatives) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          continue;
        }

        CreativeAdNotificationInfo info;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.start_at_timestamp = campaign.start_at;
        info.end_at_timestamp = campaign.end_at;
        info.daily_cap = campaign.daily_cap;
        info.per_day = creative_set.per_day;
        info.total_max = creative_set.total_max;
        info.geo_targets = regions;
        info.title = creative.payload.title;
        info.body = creative.payload.body;
        info.target_url = creative.payload.target_url;
        info.creative_instance_id = creative.creative_instance_id;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                  base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(WARNING) << "creativeSet id " << creative_set.creative_set_id
                << " has an invalid segment name";

            continue;
          }

          if (ad_notification_categories.find(segment_name) ==
              ad_notification_categories.end()) {
            ad_notification_categories.insert({segment_name, {}});
          }
          ad_notification_categories.at(segment_name).push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            if (ad_notification_categories.find(top_level_segment_name)
                == ad_notification_categories.end()) {
              ad_notification_categories.insert({top_level_segment_name, {}});
            }
            ad_notification_categories.at(top_level_segment_name)
                .push_back(info);
            entries++;
          }
        }
      }

      // Publisher ad creatives
      for (const auto& creative : creative_set.publisher_ad_creatives) {
        if (!DoesOsSupportCreativeSet(creative_set)) {
          continue;
        }

        CreativePublisherAdInfo info;
        info.creative_set_id = creative_set.creative_set_id;
        info.campaign_id = campaign.campaign_id;
        info.start_at_timestamp = campaign.start_at;
        info.end_at_timestamp = campaign.end_at;
        info.daily_cap = campaign.daily_cap;
        info.per_day = creative_set.per_day;
        info.total_max = creative_set.total_max;
        info.geo_targets = regions;
        info.size = creative.payload.size;
        info.creative_url = creative.payload.creative_url;
        info.target_url = creative.payload.target_url;
        info.creative_instance_id = creative.creative_instance_id;

        // Sites
        std::vector<std::string> channels;
        for (const auto& channel : creative.channels) {
          if (std::find(channels.begin(), channels.end(), channel.name) !=
              channels.end()) {
            continue;
          }

          channels.push_back(channel.name);
        }
        info.channels = channels;

        // Segments
        for (const auto& segment : creative_set.segments) {
          auto segment_name = base::ToLowerASCII(segment.name);

          std::vector<std::string> segment_name_hierarchy =
              base::SplitString(segment_name, "-", base::KEEP_WHITESPACE,
                  base::SPLIT_WANT_NONEMPTY);

          if (segment_name_hierarchy.empty()) {
            BLOG(WARNING) << "creativeSet id " << creative_set.creative_set_id
                << " has an invalid segment name";

            continue;
          }

          if (publisher_ad_categories.find(segment_name) ==
              publisher_ad_categories.end()) {
            publisher_ad_categories.insert({segment_name, {}});
          }
          publisher_ad_categories.at(segment_name).push_back(info);
          entries++;

          auto top_level_segment_name = segment_name_hierarchy.front();
          if (top_level_segment_name != segment_name) {
            if (publisher_ad_categories.find(top_level_segment_name) ==
                publisher_ad_categories.end()) {
              publisher_ad_categories.insert({top_level_segment_name, {}});
            }
            publisher_ad_categories.at(top_level_segment_name).push_back(info);
            entries++;
          }
        }
      }

      if (entries == 0) {
        BLOG(WARNING) << "creativeSet id " << creative_set.creative_set_id
            << " has an invalid creative";

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
  state->catalog_last_updated_timestamp_in_seconds = Time::NowInSeconds();
  state->ad_notification_categories = ad_notification_categories;
  state->publisher_ad_categories = publisher_ad_categories;
  state->ad_conversions = ad_conversions;

  return state;
}

bool Bundle::DoesOsSupportCreativeSet(
    const CatalogCreativeSetInfo& creative_set) {
  if (creative_set.oses.empty()) {
    // Creative set supports all OSes
    return true;
  }

  const std::string client_os = GetClientOS();
  for (const auto& os : creative_set.oses) {
    if (os.name == client_os) {
      return true;
    }
  }

  return false;
}

std::string Bundle::GetClientOS() {
  ClientInfo client_info;
  ads_client_->GetClientInfo(&client_info);

  switch (client_info.platform) {
    case UNKNOWN: {
      NOTREACHED();
      return "";
    }
    case WINDOWS: {
      return "windows";
    }
    case MACOS: {
      return "macos";
    }
    case IOS: {
      return "ios";
    }
    case ANDROID_OS: {
      return "android";
    }
    case LINUX: {
      return "linux";
    }
  }
}

void Bundle::OnStateSaved(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const uint64_t& catalog_last_updated_timestamp_in_seconds,
    const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save bundle state";

    // If the bundle fails to save, we will retry the next time a bundle is
    // downloaded from the Ads Serve
    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;
  catalog_last_updated_timestamp_in_seconds_ =
      catalog_last_updated_timestamp_in_seconds;

  ads_->BundleUpdated();

  BLOG(INFO) << "Successfully saved bundle state";
}

void Bundle::OnStateReset(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const uint64_t& catalog_last_updated_timestamp_in_seconds,
    const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to reset bundle state";

    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;
  catalog_last_updated_timestamp_in_seconds_ =
      catalog_last_updated_timestamp_in_seconds;

  BLOG(INFO) << "Successfully reset bundle state";
}

}  // namespace ads
