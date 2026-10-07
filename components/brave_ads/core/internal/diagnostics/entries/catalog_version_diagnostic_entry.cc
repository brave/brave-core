/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_version_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Catalog Version";
constexpr char kNotApplicable[] = "N/A";
}  // namespace

DiagnosticEntryType CatalogVersionDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogVersion;
}

std::string CatalogVersionDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogVersionDiagnosticEntry::GetValue() const {
  const int version = GetCatalogVersion();
  if (version <= 0) {
    return kNotApplicable;
  }

  return base::NumberToString(version);
}

}  // namespace brave_ads
