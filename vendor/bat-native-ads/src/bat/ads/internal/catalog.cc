/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "bat/ads/internal/catalog.h"
#include "bat/ads/internal/catalog_state.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/logging.h"

namespace ads {

Catalog::Catalog(AdsClient* ads_client) :
    ads_client_(ads_client),
    catalog_state_(nullptr) {}

Catalog::~Catalog() {}

bool Catalog::FromJson(const std::string& json) {
  auto catalog_state = std::make_unique<CatalogState>();
  auto json_schema = ads_client_->LoadJsonSchema(_catalog_schema_resource_name);
  std::string error_description;
  auto result = LoadFromJson(catalog_state.get(), json, json_schema,
      &error_description);
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to parse catalog JSON (" << error_description
        << "): " << json;

    return false;
  }

  catalog_state_.reset(catalog_state.release());

  BLOG(INFO) << "Successfully loaded catalog";

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

const std::vector<CatalogCampaignInfo>& Catalog::GetCampaigns() const {
  return catalog_state_->campaigns;
}

const IssuersInfo& Catalog::GetIssuers() const {
  return catalog_state_->issuers;
}

void Catalog::Save(const std::string& json, OnSaveCallback callback) {
  ads_client_->Save(_catalog_resource_name, json, callback);
}

void Catalog::Reset(OnSaveCallback callback) {
  ads_client_->Reset(_catalog_resource_name, callback);
}

///////////////////////////////////////////////////////////////////////////////

bool Catalog::HasChanged(const std::string& current_catalog_id) {
  if (current_catalog_id.empty()) {
    // First time the catalog has been downloaded, so does not match
    return true;
  }

  if (current_catalog_id != catalog_state_->catalog_id) {
    return true;
  }

  return false;
}

}  // namespace ads
