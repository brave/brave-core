/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/bundle_state.h"
#include "bundle.h"
#include "catalog.h"
#include "json_helper.h"
#include "logging.h"
#include "string_helper.h"
#include "time_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace ads {

Bundle::Bundle(AdsClient* ads_client) :
    catalog_id_(""),
    catalog_version_(0),
    catalog_ping_(0),
    catalog_last_updated_timestamp_(0),
    ads_client_(ads_client) {
}

Bundle::~Bundle() = default;

bool Bundle::UpdateFromCatalog(const Catalog& catalog) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  auto bundle_state = GenerateFromCatalog(catalog);
  if (!bundle_state) {
    return false;
  }

  auto callback = std::bind(&Bundle::OnStateSaved,
      this, bundle_state->catalog_id, bundle_state->catalog_version,
      bundle_state->catalog_ping, bundle_state->catalog_last_updated_timestamp,
      _1);
  ads_client_->SaveBundleState(std::move(bundle_state), callback);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Generated bundle'

  LOG(INFO) << "Generated bundle";

  return true;
}

void Bundle::Reset() {
  auto bundle_state = std::make_unique<BundleState>();

  auto callback = std::bind(&Bundle::OnStateReset,
      this, bundle_state->catalog_id, bundle_state->catalog_version,
      bundle_state->catalog_ping, bundle_state->catalog_last_updated_timestamp,
      _1);
  ads_client_->SaveBundleState(std::move(bundle_state), callback);
}

const std::string Bundle::GetCatalogId() const {
  return catalog_id_;
}

uint64_t Bundle::GetCatalogVersion() const {
  return catalog_version_;
}

uint64_t Bundle::GetCatalogPing() const {
  return catalog_ping_ / kMillisecondsInASecond;
}

uint64_t Bundle::GetCatalogLastUpdatedTimestamp() const {
  return catalog_last_updated_timestamp_;
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

  std::map<std::string, std::vector<AdInfo>> categories;

  // Campaigns
  for (const auto& campaign : catalog.GetCampaigns()) {
    // Geo Targets
    std::vector<std::string>regions = {};
    for (const auto& geo_target : campaign.geo_targets) {
      std::string code = geo_target.code;

      if (std::find(regions.begin(), regions.end(), code)
          != regions.end()) {
        continue;
      }

      regions.push_back(code);
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      // Segments
      std::vector<std::string> hierarchy = {};
      for (const auto& segment : creative_set.segments) {
        auto name = helper::String::ToLower(segment.name);

        if (std::find(hierarchy.begin(), hierarchy.end(), name)
            != hierarchy.end()) {
          continue;
        }

        hierarchy.push_back(name);
      }

      if (hierarchy.empty()) {
        LOG(ERROR) << "creativeSet segments are empty";
        return nullptr;
      }

      std::string category;
      helper::String::Join(hierarchy, '-', &category);

      auto top_level = hierarchy.front();
      uint64_t entries = 0;

      for (const auto& creative : creative_set.creatives) {
        AdInfo ad_info;
        ad_info.creative_set_id = creative_set.creative_set_id;
        ad_info.campaign_id = campaign.campaign_id;
        ad_info.start_timestamp = campaign.start_at;
        ad_info.end_timestamp = campaign.end_at;
        ad_info.daily_cap = campaign.daily_cap;
        ad_info.per_day = creative_set.per_day;
        ad_info.total_max = creative_set.total_max;
        ad_info.regions = regions;
        ad_info.advertiser = creative.payload.title;
        ad_info.notification_text = creative.payload.body;
        ad_info.notification_url = creative.payload.target_url;
        ad_info.uuid = creative.creative_instance_id;

        if (categories.find(category) == categories.end()) {
          categories.insert({category, {}});
        }
        categories.at(category).push_back(ad_info);
        entries++;

        if (categories.find(top_level) == categories.end()) {
          categories.insert({top_level, {}});
        }
        categories.at(top_level).push_back(ad_info);
        entries++;
      }

      if (entries == 0) {
        LOG(ERROR) << "creativeSet creatives are empty";
        return nullptr;
      }
    }
  }

  auto state = std::make_unique<BundleState>();
  state->catalog_id = catalog.GetId();
  state->catalog_version = catalog.GetVersion();
  state->catalog_ping = catalog.GetPing();
  state->catalog_last_updated_timestamp = helper::Time::Now();
  state->categories = categories;

  return state;
}

void Bundle::OnStateSaved(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const uint64_t& catalog_last_updated_timestamp,
    const Result result) {
  if (result != SUCCESS) {
    LOG(ERROR) << "Failed to save bundle state";

    // If the bundle fails to save, we will retry the next time a bundle is
    // downloaded from the Ads Serve
    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;
  catalog_last_updated_timestamp_ = catalog_last_updated_timestamp;

  LOG(INFO) << "Successfully saved bundle state";
}

void Bundle::OnStateReset(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const uint64_t& catalog_last_updated_timestamp,
    const Result result) {
  if (result != SUCCESS) {
    LOG(ERROR) << "Failed to reset bundle state";

    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;
  catalog_last_updated_timestamp_ = catalog_last_updated_timestamp;

  LOG(INFO) << "Successfully reset bundle state";
}

}  // namespace ads
