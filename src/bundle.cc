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
    ads_client_(ads_client),
    catalog_id_(""),
    catalog_version_(0),
    catalog_ping_(0) {
}

Bundle::~Bundle() = default;

bool Bundle::UpdateFromCatalog(
    const Catalog& catalog) {
  // TODO(bridiver) - add callback for this method
  std::map<std::string, std::vector<AdInfo>> categories;

  for (const auto& campaign : catalog.GetCampaigns()) {
    std::vector<std::string> heirarchy = {};

    for (const auto& creative_set : campaign.creative_sets) {
      for (const auto& segment : creative_set.segments) {
        std::string name = "";
        for (size_t i = 0; i < segment.name.size(); i++) {
          name += tolower(segment.name[i]);
        }

        if (std::find(heirarchy.begin(), heirarchy.end(), name)
            == heirarchy.end()) {
          heirarchy.push_back(name);
        }
      }

      if (heirarchy.empty()) {
        return false;
      }

      std::string category;
      helper::String::Join(heirarchy, '-', category);

      std::string top_level = heirarchy.front();
      uint64_t entries = 0;

      for (const auto& creative : creative_set.creatives) {
        AdInfo ad_info;
        ad_info.creative_set_id = creative_set.creative_set_id;
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
        return false;
      }
    }
  }

  auto state = std::make_unique<BUNDLE_STATE>();
  state->catalog_id = catalog.GetId();
  state->catalog_version = catalog.GetVersion();
  state->catalog_ping = catalog.GetPing();
  state->categories = categories;

  InitializeFromBundleState(std::move(state));

  return true;
}

void Bundle::Reset() {
  // TODO(bridiver) - should we set to null here instead?
  InitializeFromBundleState(nullptr);
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

void Bundle::InitializeFromBundleState(std::unique_ptr<BUNDLE_STATE> state) {
  if (!state) {
    // ads_client_->ResetBundleState(
    //     std::bind(&Bundle::OnBundleStateReset, this, _1));
    ads_client_->Reset("bundle.json",
        std::bind(&Bundle::OnBundleSavedForTesting, this, _1));
    return;
  }

  // TODO(bridiver) - this is for debugging only so maybe we should just
  // do a VLOG or something instead of saving to disk?
  ToJsonForTesting(*state);

  ads_client_->SaveBundleState(std::move(state),
      std::bind(&Bundle::OnBundleStateSaved, this,
          state->catalog_id, state->catalog_version, state->catalog_ping, _1));
}

void Bundle::OnBundleStateSaved(const std::string& catalog_id,
                                const uint64_t& catalog_version,
                                const uint64_t& catalog_ping,
                                Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::ERROR) << "Failed to save bundle";
    // TODO(bridiver) - seems like we should do more than just log here?
    return;
  }

  catalog_id_ = catalog_id;
  catalog_version_ = catalog_version;
  catalog_ping_ = catalog_ping;

  LOG(ads_client_, LogLevel::INFO) << "Bundle saved";
}

void Bundle::OnBundleStateReset(Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::ERROR) << "Bundle reset failed";
    return;
  }

  LOG(ads_client_, LogLevel::ERROR) << "Bundle reset";
  catalog_id_ = "";
  catalog_version_ = 0;
  catalog_ping_ = 0;
}

void Bundle::ToJsonForTesting(const BUNDLE_STATE& state) {
  std::string json;
  SaveToJson(state, json);
  ads_client_->Save("bundle.json", json,
    std::bind(&Bundle::OnBundleSavedForTesting, this, _1));
}

void Bundle::OnBundleSavedForTesting(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::ERROR) << "Save json bundle failed";
    return;
  }

  LOG(ads_client_, LogLevel::INFO) << "Bundle json saved";
}

// see comment above about VLOG vs saving to disk
bool Bundle::FromJsonForTesting(const std::string& json) {
  auto state = std::make_unique<BUNDLE_STATE>();
  auto jsonSchema = ads_client_->Load("bundle-schema.json");
  if (!LoadFromJson(*state, json.c_str(), jsonSchema)) {
    return false;
  }

  InitializeFromBundleState(std::move(state));
  return true;
}

}  // namespace ads
