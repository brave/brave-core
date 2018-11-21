/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/bundle_state.h"
#include "bundle.h"
#include "catalog.h"
#include "json_helper.h"
#include "logging.h"
#include "string_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace ads {

Bundle::Bundle(AdsClient* ads_client) :
    catalog_id_(""),
    catalog_version_(0),
    catalog_ping_(0),
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
    bundle_state->catalog_ping, _1);
  ads_client_->SaveBundleState(std::move(bundle_state), callback);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Generated bundle'

  return true;
}

void Bundle::Reset() {
  auto bundle_state = std::make_unique<BundleState>();

  auto callback = std::bind(&Bundle::OnStateReset,
    this, bundle_state->catalog_id, bundle_state->catalog_version,
    bundle_state->catalog_ping, _1);
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

///////////////////////////////////////////////////////////////////////////////

// TODO(Terry Mancey): We should consider optimizing memory consumption when
// generating the bundle by saving each campaign individually on the Client
std::unique_ptr<BundleState> Bundle::GenerateFromCatalog(
    const Catalog& catalog) {
  // TODO(Terry Mancey): Refactor function to use callbacks
  std::map<std::string, std::vector<AdInfo>> categories;

  // Campaigns
  for (const auto& campaign : catalog.GetCampaigns()) {
    std::vector<std::string> heirarchy = {};

    // Geo Targets
    std::vector<std::string>regions = {};

    for (const auto& geo_target : campaign.geo_targets) {
      std::string code = geo_target.code;

      if (std::find(regions.begin(), regions.end(), code)
          != heirarchy.end()) {
        continue;
      }

      regions.push_back(code);
    }

    // Creative Sets
    for (const auto& creative_set : campaign.creative_sets) {
      // Segments
      for (const auto& segment : creative_set.segments) {
        auto name = helper::String::ToLower(segment.name);

        if (std::find(heirarchy.begin(), heirarchy.end(), name)
            != heirarchy.end()) {
          continue;
        }

        heirarchy.push_back(name);
      }

      if (heirarchy.empty()) {
        return nullptr;
      }

      std::string category;
      helper::String::Join(heirarchy, '-', &category);

      auto top_level = heirarchy.front();
      uint64_t entries = 0;

      for (const auto& creative : creative_set.creatives) {
        AdInfo ad_info;
        ad_info.creative_set_id = creative_set.creative_set_id;
        ad_info.startTimestamp = campaign.start_at;
        ad_info.endTimestamp = campaign.end_at;
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
        return nullptr;
      }
    }
  }

  auto state = std::make_unique<BundleState>();
  state->catalog_id = catalog.GetId();
  state->catalog_version = catalog.GetVersion();
  state->catalog_ping = catalog.GetPing();
  state->categories = categories;

  return state;
}

void Bundle::OnStateSaved(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const Result result) {
  if (result == Result::FAILED) {
    LOG(LogLevel::ERROR) << "Failed to save bundle state";

    // If the bundle fails to save, we will retry the next time a bundle is
    // downloaded from the Ads Serve
    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;

  LOG(LogLevel::INFO) << "Successfully saved bundle state";
}

void Bundle::OnStateReset(
    const std::string& catalog_id,
    const uint64_t& catalog_version,
    const uint64_t& catalog_ping,
    const Result result) {
  if (result == Result::FAILED) {
    LOG(LogLevel::ERROR) << "Failed to reset bundle state";

    // TODO(Terry Mancey): If the bundle fails to reset we need to decide what
    // action to take
    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;

  LOG(LogLevel::INFO) << "Successfully reset bundle state";
}

}  // namespace ads
