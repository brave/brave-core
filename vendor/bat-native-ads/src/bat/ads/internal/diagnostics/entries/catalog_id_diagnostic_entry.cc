/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
constexpr char kName[] = "Catalog ID";
}  // namespace

CatalogIdDiagnosticEntry::CatalogIdDiagnosticEntry() = default;

CatalogIdDiagnosticEntry::~CatalogIdDiagnosticEntry() = default;

DiagnosticEntryType CatalogIdDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogId;
}

std::string CatalogIdDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogIdDiagnosticEntry::GetValue() const {
  return AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId);
}

}  // namespace ads
