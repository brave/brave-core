/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_shields/browser/ad_block_regional_catalog_provider.h"

namespace brave_shields {

AdBlockRegionalCatalogProvider::AdBlockRegionalCatalogProvider() {}

AdBlockRegionalCatalogProvider::~AdBlockRegionalCatalogProvider() {}

void AdBlockRegionalCatalogProvider::AddObserver(
    AdBlockRegionalCatalogProvider::Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockRegionalCatalogProvider::RemoveObserver(
    AdBlockRegionalCatalogProvider::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockRegionalCatalogProvider::OnRegionalCatalogLoaded(
    const std::string& catalog_json) {
  for (auto& observer : observers_) {
    observer.OnRegionalCatalogLoaded(catalog_json);
  }
}

}  // namespace brave_shields
