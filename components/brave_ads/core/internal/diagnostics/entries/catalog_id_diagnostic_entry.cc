/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Catalog ID";
constexpr char kNotApplicable[] = "N/A";
}  // namespace

DiagnosticEntryType CatalogIdDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogId;
}

std::string CatalogIdDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogIdDiagnosticEntry::GetValue() const {
  std::string catalog_id = GetCatalogId();
  if (catalog_id.empty()) {
    return kNotApplicable;
  }

  return catalog_id;
}

}  // namespace brave_ads
