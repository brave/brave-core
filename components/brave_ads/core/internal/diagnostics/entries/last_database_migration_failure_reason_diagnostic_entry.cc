/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_database_migration_failure_reason_diagnostic_entry.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/database/database_manager.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Last migration failure reason";
constexpr char kNone[] = "None";
}  // namespace

DiagnosticEntryType LastDatabaseMigrationFailureReasonDiagnosticEntry::GetType()
    const {
  return DiagnosticEntryType::kLastDatabaseMigrationFailureReason;
}

std::string LastDatabaseMigrationFailureReasonDiagnosticEntry::GetName() const {
  return kName;
}

std::string LastDatabaseMigrationFailureReasonDiagnosticEntry::GetValue()
    const {
  const std::optional<std::string>& reason =
      DatabaseManager::GetInstance().GetLastMigrationFailureReason();
  return reason.value_or(kNone);
}

}  // namespace brave_ads
