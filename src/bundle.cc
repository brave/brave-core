/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bundle.h"
#include "string_helper.h"
#include "static_values.h"

namespace ads {

Bundle::Bundle(AdsClient* ads_client) :
    ads_client_(ads_client),
    bundle_state_(new BUNDLE_STATE()) {
}

Bundle::~Bundle() = default;

bool Bundle::LoadJson(const std::string& json) {
  BUNDLE_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    ads_client_->Log(LogLevel::ERROR, "Failed to parse bundle json");
    return false;
  }

  bundle_state_.reset(new BUNDLE_STATE(state));
  Save();

  return true;
}

void Bundle::SaveJson() {
  std::string json;
  SaveToJson(*bundle_state_, json);
  ads_client_->SaveBundle(json, this);
}

void Bundle::Save() {
  ads_client_->SaveBundle(*bundle_state_, this);
}

bool Bundle::GenerateFromCatalog(
    const std::shared_ptr<CATALOG_STATE> catalog_state) {
  std::map<std::string, std::vector<CategoryInfo>> categories;

  for (const auto& campaign : catalog_state->campaigns) {
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
        CategoryInfo category_info;
        category_info.creative_set_id = creative_set.creative_set_id;
        category_info.advertiser = creative.payload.title;
        category_info.notification_text = creative.payload.body;
        category_info.notification_url = creative.payload.target_url;
        category_info.uuid = creative.creative_id;

        if (categories.find(category) == categories.end()) {
          categories.insert({category, {}});
        }
        categories.at(category).push_back(category_info);
        entries++;

        if (categories.find(top_level) == categories.end()) {
          categories.insert({top_level, {}});
        }
        categories.at(top_level).push_back(category_info);
        entries++;
      }

      if (entries == 0) {
        return false;
      }
    }
  }

  BUNDLE_STATE state;
  state.catalog_id = catalog_state->catalog_id;
  state.catalog_version = catalog_state->version;
  state.catalog_ping = catalog_state->ping;
  state.categories = categories;

  bundle_state_.reset(new BUNDLE_STATE(state));

  return true;
}

void Bundle::Reset() {
  bundle_state_.reset(new BUNDLE_STATE());
  Save();
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

//////////////////////////////////////////////////////////////////////////////

void Bundle::OnBundleSaved(const Result result) {
  if (result == Result::FAILED) {
    ads_client_->Log(LogLevel::WARNING, "Failed to save bundle");
  }
}

}  // namespace ads
