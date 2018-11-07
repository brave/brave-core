/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog.h"
#include "json_helper.h"
#include "static_values.h"
#include "logging.h"

namespace ads {

Catalog::Catalog(AdsClient* ads_client) :
    ads_client_(ads_client),
    catalog_state_(new CATALOG_STATE()) {
}

Catalog::~Catalog() = default;

bool Catalog::FromJson(const std::string& json) {
  CATALOG_STATE state;
  auto jsonSchema = ads_client_->Load("catalog-schema.json");
  if (!LoadFromJson(state, json.c_str(), jsonSchema)) {
    LOG(ads_client_, LogLevel::ERROR) << "Failed to parse catalog: " << json;
    return false;
  }

  if (!catalog_state_->catalog_id.empty() &&
      state.catalog_id != catalog_state_->catalog_id) {
    LOG(ads_client_, LogLevel::ERROR) << "Current catalog id " <<
      catalog_state_->catalog_id << " does not match catalog id " <<
      state.catalog_id;
    return false;
  }

  catalog_state_.reset(new CATALOG_STATE(state));

  LOG(ads_client_, LogLevel::INFO) << "Successfully loaded catalog";

  return true;
}

const std::shared_ptr<CATALOG_STATE> Catalog::GetCatalogState() const {
  return catalog_state_;
}

}  // namespace ads
