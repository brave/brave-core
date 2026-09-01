/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/schema_version_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Schema version";
}  // namespace

DiagnosticEntryType SchemaVersionDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kSchemaVersion;
}

std::string SchemaVersionDiagnosticEntry::GetName() const {
  return kName;
}

std::string SchemaVersionDiagnosticEntry::GetValue() const {
  return base::NumberToString(database::kVersionNumber);
}

}  // namespace brave_ads
