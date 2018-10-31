/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog.h"
#include "json_helper.h"
#include "static_values.h"

namespace state {

Catalog::Catalog(ads::AdsClient* ads_client) :
    ads_client_(ads_client),
    catalog_state_(new CATALOG_STATE()) {
}

Catalog::~Catalog() = default;

bool Catalog::LoadJson(const std::string& json) {
  CATALOG_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    ads_client_->Log(ads::LogLevel::ERROR, "Failed to parse catalog json");
    return false;
  }

  if (!catalog_state_->catalog_id.empty() &&
      state.catalog_id != catalog_state_->catalog_id) {
    return false;
  }

  catalog_state_.reset(new CATALOG_STATE(state));

  return true;
}

std::shared_ptr<CATALOG_STATE> Catalog::GetCatalogState() const {
  return catalog_state_;
}

}  // namespace state
