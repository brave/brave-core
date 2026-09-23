/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_DATABASE_MIGRATION_FAILURE_REASON_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_DATABASE_MIGRATION_FAILURE_REASON_DIAGNOSTIC_ENTRY_H_

#include <string>

#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

// Reports the reason the most recent database migration attempt in this
// session failed, from `DatabaseManager`'s in-memory state. Not persisted,
// so only reflects the current session, not history across restarts.
class LastDatabaseMigrationFailureReasonDiagnosticEntry final
    : public DiagnosticEntryInterface {
 public:
  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_DATABASE_MIGRATION_FAILURE_REASON_DIAGNOSTIC_ENTRY_H_
