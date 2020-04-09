/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/catalog.h"
#include "bat/ads/internal/catalog_state.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/logging.h"

namespace ads {

Catalog::Catalog(
    AdsImpl* ads)
    : ads_(ads),
      catalog_state_(nullptr) {}

Catalog::~Catalog() {}

bool Catalog::FromJson(const std::string& json) {
  auto catalog_state = std::make_unique<CatalogState>();
  auto json_schema =
      ads_->get_ads_client()->LoadJsonSchema(_catalog_schema_resource_name);
  std::string error_description;
  auto result = LoadFromJson(catalog_state.get(), json, json_schema,
      &error_description);
  if (result != SUCCESS) {
    return false;
  }

  catalog_state_.reset(catalog_state.release());

  return true;
}

std::string Catalog::GetId() const {
  return catalog_state_->catalog_id;
}

uint64_t Catalog::GetVersion() const {
  return catalog_state_->version;
}

uint64_t Catalog::GetPing() const {
  return catalog_state_->ping;
}

CatalogCampaignList Catalog::GetCampaigns() const {
  return catalog_state_->campaigns;
}

IssuersInfo Catalog::GetIssuers() const {
  return catalog_state_->issuers;
}

void Catalog::Save(const std::string& json, ResultCallback callback) {
  ads_->get_ads_client()->Save(_catalog_resource_name, json, callback);
}

void Catalog::Reset(ResultCallback callback) {
  ads_->get_ads_client()->Reset(_catalog_resource_name, callback);
}

const std::string& Catalog::get_last_message() const {
  return last_message_;
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
