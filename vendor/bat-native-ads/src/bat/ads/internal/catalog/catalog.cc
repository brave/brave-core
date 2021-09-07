/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog.h"

#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/catalog/catalog_state.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

Catalog::Catalog() : catalog_state_(std::make_unique<CatalogState>()) {}

Catalog::~Catalog() = default;

bool Catalog::FromJson(const std::string& json) {
  auto json_schema =
      AdsClientHelper::Get()->LoadResourceForId(g_catalog_schema_resource_id);
  return LoadFromJson(catalog_state_.get(), json, json_schema);
}

bool Catalog::HasChanged(const std::string& catalog_id) const {
  if (catalog_id.empty()) {
    // First time the catalog has been downloaded, so does not match
    return true;
  }

  if (catalog_id != catalog_state_->catalog_id) {
    return true;
  }

  return false;
}

std::string Catalog::GetId() const {
  return catalog_state_->catalog_id;
}

int Catalog::GetVersion() const {
  return catalog_state_->version;
}

int64_t Catalog::GetPing() const {
  return catalog_state_->ping / base::Time::kMillisecondsPerSecond;
}

CatalogIssuersInfo Catalog::GetIssuers() const {
  return catalog_state_->catalog_issuers;
}

CatalogCampaignList Catalog::GetCampaigns() const {
  return catalog_state_->campaigns;
}

}  // namespace ads
