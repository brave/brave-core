/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"

namespace brave_ads {

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
  const base::Time last_updated_at = GetCatalogLastUpdated();
  if (last_updated_at.is_null()) {
    return {};
  }

  return LongFriendlyDateAndTime(last_updated_at,
                                 /*use_sentence_style=*/false);
}

}  // namespace brave_ads
