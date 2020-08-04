/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const char kCatalogFilename[] = "catalog.json";
}  // namespace

Catalog::Catalog(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Catalog::~Catalog() = default;

bool Catalog::FromJson(
    const std::string& json) {
  auto catalog_state = std::make_unique<CatalogState>();
  auto json_schema =
      ads_->get_ads_client()->LoadResourceForId(_catalog_schema_resource_id);
  auto result = LoadFromJson(catalog_state.get(), json, json_schema);
  if (result != SUCCESS) {
    return false;
  }

  catalog_state_.reset(catalog_state.release());

  return true;
}

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

CatalogIssuersInfo Catalog::GetIssuers() const {
  return catalog_state_->catalog_issuers;
}

void Catalog::Save(
    const std::string& json,
    ResultCallback callback) {
  ads_->get_ads_client()->Save(kCatalogFilename, json, callback);
}

const std::string& Catalog::get_last_message() const {
  return last_message_;
}

}  // namespace ads
