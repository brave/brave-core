/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_35_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_35_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

// Adds a "job_state" table for tracking progress of resumable jobs.
class Upgrade35 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 35;

  static inline const char kSQL[] = R"sql(

    CREATE TABLE job_state (
      job_id TEXT NOT NULL PRIMARY KEY,
      job_type TEXT NOT NULL,
      state TEXT,
      error TEXT,
      created_at TEXT NOT NULL,
      completed_at TEXT
    );

    CREATE INDEX job_state_job_type_index ON job_state (job_type);

  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_35_H_
