/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bundle.h"
#include "string_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace ads {

Bundle::Bundle() :
    bundle_state_(new BUNDLE_STATE()) {
}

Bundle::~Bundle() = default;

bool Bundle::FromJson(const std::string& json) {
  BUNDLE_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    return false;
  }

  bundle_state_.reset(new BUNDLE_STATE(state));
  return true;
}

const std::string Bundle::ToJson() {
  std::string json;
  SaveToJson(*bundle_state_, json);
  return json;
}

bool Bundle::GenerateFromCatalog(
    const CATALOG_STATE& catalog_state) {
  std::map<std::string, std::vector<AdInfo>> categories;

  for (const auto& campaign : catalog_state.campaigns) {
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
        ad_info.uuid = creative.creative_id;

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

  BUNDLE_STATE state;
  state.catalog_id = catalog_state.catalog_id;
  state.catalog_version = catalog_state.version;
  state.catalog_ping = catalog_state.ping;
  state.categories = categories;

  bundle_state_.reset(new BUNDLE_STATE(state));

  return true;
}

void Bundle::Reset() {
  bundle_state_.reset(new BUNDLE_STATE());
  // TODO(Brian Johnson): Save() was removed from here, this is important
  // otherwise the client will have a previous bundle, see commit #e82bda44
  // where Save() was removed
}

std::string Bundle::GetCatalogId() const {
  return bundle_state_->catalog_id;
}

uint64_t Bundle::GetCatalogVersion() const {
  return bundle_state_->catalog_version;
}

uint64_t Bundle::GetCatalogPing() const {
  return bundle_state_->catalog_ping / kMillisecondsInASecond;
}

}  // namespace ads
