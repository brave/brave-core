/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"

#include "bat/ads/internal/catalog/catalog_util.h"

namespace ads {

CatalogFrequencyCap::CatalogFrequencyCap() = default;

CatalogFrequencyCap::~CatalogFrequencyCap() = default;

bool CatalogFrequencyCap::ShouldAllow() {
  return DoesRespectCap();
}

std::string CatalogFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool CatalogFrequencyCap::DoesRespectCap() {
  if (!DoesCatalogExist()) {
    last_message_ = "Catalog does not exist";
    return false;
  }

  if (HasCatalogExpired()) {
    last_message_ = "Catalog has expired";
    return false;
  }

  return true;
}

}  // namespace ads
