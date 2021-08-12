/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/catalog_id_ad_diagnostics_entry.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

CatalogIdAdDiagnosticsEntry::CatalogIdAdDiagnosticsEntry() = default;

CatalogIdAdDiagnosticsEntry::~CatalogIdAdDiagnosticsEntry() = default;

AdDiagnosticsEntryType CatalogIdAdDiagnosticsEntry::GetEntryType() const {
  return AdDiagnosticsEntryType::kCatalogId;
}

std::string CatalogIdAdDiagnosticsEntry::GetKey() const {
  return "Catalog ID";
}

std::string CatalogIdAdDiagnosticsEntry::GetValue() const {
  return AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId);
}

}  // namespace ads
