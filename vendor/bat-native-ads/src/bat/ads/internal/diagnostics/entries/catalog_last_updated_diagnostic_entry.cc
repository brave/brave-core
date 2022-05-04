/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
constexpr char kName[] = "Catalog last updated";
}  // namespace

CatalogLastUpdatedDiagnosticEntry::CatalogLastUpdatedDiagnosticEntry() =
    default;

CatalogLastUpdatedDiagnosticEntry::~CatalogLastUpdatedDiagnosticEntry() =
    default;

DiagnosticEntryType CatalogLastUpdatedDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogLastUpdated;
}

std::string CatalogLastUpdatedDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogLastUpdatedDiagnosticEntry::GetValue() const {
  const double catalog_last_updated_timestamp =
      AdsClientHelper::Get()->GetDoublePref(prefs::kCatalogLastUpdated);

  const base::Time catalog_last_updated =
      base::Time::FromDoubleT(catalog_last_updated_timestamp);
  if (catalog_last_updated.is_null()) {
    return {};
  }

  return base::TimeToISO8601(catalog_last_updated);
}

}  // namespace ads
