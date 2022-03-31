/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/catalog_last_updated_ad_diagnostics_entry.h"

#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

CatalogLastUpdatedAdDiagnosticsEntry::CatalogLastUpdatedAdDiagnosticsEntry() =
    default;

CatalogLastUpdatedAdDiagnosticsEntry::~CatalogLastUpdatedAdDiagnosticsEntry() =
    default;

AdDiagnosticsEntryType CatalogLastUpdatedAdDiagnosticsEntry::GetEntryType()
    const {
  return AdDiagnosticsEntryType::kCatalogLastUpdated;
}

std::string CatalogLastUpdatedAdDiagnosticsEntry::GetKey() const {
  return "Catalog last updated";
}

std::string CatalogLastUpdatedAdDiagnosticsEntry::GetValue() const {
  const double catalog_last_updated_timestamp =
      AdsClientHelper::Get()->GetDoublePref(prefs::kCatalogLastUpdated);

  const base::Time time =
      base::Time::FromDoubleT(catalog_last_updated_timestamp);

  return ConvertToString(time);
}

}  // namespace ads
