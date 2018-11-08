/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog.h"

#include "bundle.h"
#include "catalog_state.h"
#include "json_helper.h"
#include "static_values.h"
#include "logging.h"

using namespace std::placeholders;

namespace ads {

Catalog::Catalog(Bundle* bundle, AdsClient* ads_client) :
    bundle_(bundle),
    ads_client_(ads_client) {}

Catalog::~Catalog() {}

bool Catalog::FromJson(const std::string& json) {
  auto state = std::make_unique<CATALOG_STATE>();

  auto jsonSchema = ads_client_->Load("catalog-schema.json");
  if (!LoadFromJson(*state, json.c_str(), jsonSchema)) {
    LOG(ads_client_, LogLevel::ERROR) << "Failed to parse catalog: " << json;
    return false;
  }

  auto current_catalog_id = bundle_->GetCatalogId();
  if (!current_catalog_id.empty() &&
      current_catalog_id != catalog_state_->catalog_id) {
    // TODO is this check right? Should the id be the same every time?
    LOG(ads_client_, LogLevel::ERROR) << "New catalog id " <<
        catalog_state_->catalog_id << " does not match current catalog id " <<
        current_catalog_id;
    return false;
  }

  auto current_catalog_version = bundle_->GetCatalogVersion();
  if (current_catalog_version != 0 &&
      current_catalog_version <= state->version) {
    // TODO this might also potentially be incorrect
    LOG(ads_client_, LogLevel::ERROR) << "New catalog version " <<
        state->version << " is not greater than current version " <<
        current_catalog_version;
    return false;
  }

  catalog_state_.reset(state.release());
  ads_client_->Save("catalog.json", json,
    std::bind(&Catalog::OnCatalogSaved, this, _1));

  LOG(ads_client_, LogLevel::INFO) << "Successfully loaded catalog";

  return true;
}

const std::string Catalog::GetId() const {
  return catalog_state_->catalog_id;
}

uint64_t Catalog::GetVersion() const {
  return catalog_state_->version;
}

uint64_t Catalog::GetPing() const {
  return catalog_state_->ping;
}

const std::vector<CampaignInfo>& Catalog::GetCampaigns() const {
  return catalog_state_->campaigns;
}

void Catalog::OnCatalogSaved(Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::ERROR) << "Could not save catalog.json";
  } else {
    LOG(ads_client_, LogLevel::INFO) << "Saved catalog.json";
  }
}

}  // namespace ads
