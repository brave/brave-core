/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/catalog.h"
#include "../include/ads_impl.h"
#include "../include/catalog_state.h"
#include "../include/json_helper.h"
#include "../include/ads_client.h"

namespace state {

Catalog::Catalog(rewards_ads::AdsImpl* ads, ads::AdsClient* ads_client) :
      ads_(ads),
      ads_client_(ads_client),
      catalog_state_(new CATALOG_STATE()) {
}

Catalog::~Catalog() = default;

bool Catalog::LoadState(const std::string& json) {
  CATALOG_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    return false;
  }

  catalog_state_.reset(new CATALOG_STATE(state));

  return true;
}

void Catalog::SaveState() {
  ads_client_->SaveCatalogState(*catalog_state_, this);
}

std::string Catalog::GetCatalogId() const {
  return catalog_state_->catalog_id;
}

int64_t Catalog::GetVersion() const {
  return catalog_state_->version;
}

int64_t Catalog::GetPing() const {
  return catalog_state_->ping / 1000;
}

void Catalog::Reset() {
  catalog_state_.reset(new CATALOG_STATE());

  ads_client_->SaveCatalogState(*catalog_state_, this);
}

void Catalog::OnCatalogStateSaved(const ads::Result result) {
}

}  // namespace state
