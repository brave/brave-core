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

Catalog::Catalog(AdsClient* ads_client, Bundle* bundle) :
    ads_client_(ads_client),
    bundle_(bundle) {}

Catalog::~Catalog() {}

bool Catalog::FromJson(const std::string& json) {
  auto catalog_state = std::make_unique<CATALOG_STATE>();

  auto jsonSchema = ads_client_->Load("catalog-schema.json");
  if (!LoadFromJson(*catalog_state, json, jsonSchema)) {
    LOG(LogLevel::ERROR) << "Failed to parse catalog: " << json;
    return false;
  }

  if (!IsIdValid(*catalog_state)) {
    LOG(LogLevel::ERROR) << "New catalog id " <<
      catalog_state_->catalog_id << " does not match current catalog id " <<
      bundle_->GetCatalogId();

    return false;
  }

  catalog_state_.reset(catalog_state.release());

  LOG(LogLevel::INFO) << "Successfully loaded catalog";

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

void Catalog::Save(const std::string& json) {
  auto callback = std::bind(&Catalog::OnCatalogSaved, this, _1);
  ads_client_->Save("catalog.json", json, callback);
}

void Catalog::Reset() {
  auto callback = std::bind(&Catalog::OnCatalogReset, this, _1);
  ads_client_->Reset("catalog.json", callback);
}

///////////////////////////////////////////////////////////////////////////////

bool Catalog::IsIdValid(const CATALOG_STATE& catalog_state) {
  auto current_catalog_id = bundle_->GetCatalogId();
  auto new_catalog_id = catalog_state.catalog_id;

  if (current_catalog_id.empty()) {
    // First time the catalog has been downloaded
    return true;
  }

  // Catalog id should not change as it is used to download a catalog diff
  if (current_catalog_id != new_catalog_id) {
    return false;
  }

  return true;
}

void Catalog::OnCatalogSaved(const Result result) {
  if (result == Result::FAILED) {
    LOG(LogLevel::ERROR) << "Failed to save catalog";

    // If the catalog fails to save, we will retry the next time a we collect
    // activity
    return;
  }

  LOG(LogLevel::INFO) << "Successfully saved catalog";
}

void Catalog::OnCatalogReset(const Result result) {
  if (result == Result::FAILED) {
    LOG(LogLevel::ERROR) << "Failed to reset catalog";

    // TODO(Terry Mancey): If the catalog fails to reset we need to decide what
    // action to take
    return;
  }

  LOG(LogLevel::INFO) << "Successfully reset catalog";
}

}  // namespace ads
