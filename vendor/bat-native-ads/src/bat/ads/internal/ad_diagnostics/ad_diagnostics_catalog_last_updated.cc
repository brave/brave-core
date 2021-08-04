/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_catalog_last_updated.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

AdDiagnosticsCatalogLastUpdated::AdDiagnosticsCatalogLastUpdated() = default;

AdDiagnosticsCatalogLastUpdated::~AdDiagnosticsCatalogLastUpdated() = default;

AdDiagnosticsEntryType AdDiagnosticsCatalogLastUpdated::GetEntryType() const {
  return AdDiagnosticsEntryType::kCatalogLastUpdated;
}

std::string AdDiagnosticsCatalogLastUpdated::GetKey() const {
  return "Catalog last updated";
}

std::string AdDiagnosticsCatalogLastUpdated::GetValue() const {
  const int64_t catalog_last_updated =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogLastUpdated);
  const base::Time time = base::Time::FromDoubleT(catalog_last_updated);
  return ConvertToString(time);
}

}  // namespace ads
