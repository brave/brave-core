/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"

#include "base/time/time.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"

namespace ads {

namespace {
constexpr char kName[] = "Catalog last updated";
}  // namespace

DiagnosticEntryType CatalogLastUpdatedDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogLastUpdated;
}

std::string CatalogLastUpdatedDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogLastUpdatedDiagnosticEntry::GetValue() const {
  const base::Time catalog_last_updated = GetCatalogLastUpdated();
  if (catalog_last_updated.is_null()) {
    return {};
  }

  return LongFriendlyDateAndTime(catalog_last_updated,
                                 /*use_sentence_style*/ false);
}

}  // namespace ads
