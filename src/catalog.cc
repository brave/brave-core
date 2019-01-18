/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog.h"

#include "bat/ads/ads.h"
#include "bundle.h"
#include "catalog_state.h"
#include "json_helper.h"
#include "static_values.h"
#include "logging.h"

using namespace std::placeholders;

namespace ads {

Catalog::Catalog(AdsClient* ads_client, Bundle* bundle) :
    ads_client_(ads_client),
    bundle_(bundle),
    catalog_state_(nullptr) {}

Catalog::~Catalog() {}

bool Catalog::FromJson(const std::string& json) {
  auto catalog_state = std::make_unique<CatalogState>();
  auto json_schema = ads_client_->LoadJsonSchema(_catalog_schema_name);
  std::string error_description;
  auto result = LoadFromJson(catalog_state.get(), json, json_schema,
      &error_description);
  if (result != SUCCESS) {
    LOG(ERROR) << "Failed to to load catalog JSON (" << error_description
        << "): " << json;

    return false;
  }

  if (!IsIdValid(*catalog_state)) {
    LOG(WARNING) << "New catalog id " << catalog_state->catalog_id <<
        " does not match current catalog id " << bundle_->GetCatalogId();

    return false;
  }

  catalog_state_.reset(catalog_state.release());

  LOG(INFO) << "Successfully loaded catalog";

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

const IssuersInfo& Catalog::GetIssuers() const {
  return catalog_state_->issuers;
}

void Catalog::Save(const std::string& json, OnSaveCallback callback) {
  ads_client_->Save(_catalog_name, json, callback);
}

void Catalog::Reset(OnSaveCallback callback) {
  ads_client_->Reset(_catalog_name, callback);
}

///////////////////////////////////////////////////////////////////////////////

bool Catalog::IsIdValid(const CatalogState& catalog_state) {
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

}  // namespace ads
