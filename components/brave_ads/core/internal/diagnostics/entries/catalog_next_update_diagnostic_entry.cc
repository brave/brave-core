/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_next_update_diagnostic_entry.h"

#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Catalog next update";
constexpr char kNever[] = "Never";
}  // namespace

DiagnosticEntryType CatalogNextUpdateDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kCatalogNextUpdate;
}

std::string CatalogNextUpdateDiagnosticEntry::GetName() const {
  return kName;
}

std::string CatalogNextUpdateDiagnosticEntry::GetValue() const {
  const base::Time last_updated_at = GetCatalogLastUpdated();
  if (last_updated_at.is_null()) {
    return kNever;
  }

  const base::Time next_update_at = last_updated_at + GetCatalogPing();
  const std::string next_update_at_text =
      LongFriendlyDateAndTime(next_update_at, /*use_sentence_style=*/false);

  // Still overdue for an update if this is in the past, which is worth
  // surfacing as "ago" rather than a negative "in" duration.
  const base::Time now = base::Time::Now();
  if (next_update_at > now) {
    return base::StrCat({next_update_at_text, " (in ",
                         FormatApproximateDuration(next_update_at - now), ")"});
  }

  return base::StrCat({next_update_at_text, " (",
                       FormatApproximateDuration(now - next_update_at),
                       " ago)"});
}

}  // namespace brave_ads
