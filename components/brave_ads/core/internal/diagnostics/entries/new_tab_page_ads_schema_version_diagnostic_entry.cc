/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/new_tab_page_ads_schema_version_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "New Tab Page Ads Schema Version";
constexpr char kNotApplicable[] = "N/A";
}  // namespace

NewTabPageAdsSchemaVersionDiagnosticEntry::
    NewTabPageAdsSchemaVersionDiagnosticEntry(int schema_version) {
  schema_version_ = schema_version;
}

DiagnosticEntryType NewTabPageAdsSchemaVersionDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kNewTabPageAdsSchemaVersion;
}

std::string NewTabPageAdsSchemaVersionDiagnosticEntry::GetName() const {
  return kName;
}

std::string NewTabPageAdsSchemaVersionDiagnosticEntry::GetValue() const {
  if (!schema_version_) {
    return kNotApplicable;
  }

  return base::NumberToString(*schema_version_);
}

}  // namespace brave_ads
