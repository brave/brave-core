/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_29_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_29_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

// Adds the event_log table for storing timestamped and categorized events.
class Upgrade29 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 29;

  static inline const char kSQL[] = R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS event_log;
    PRAGMA foreign_keys = on;

    CREATE TABLE event_log (
      event_log_id LONGVARCHAR NOT NULL PRIMARY KEY,
      key TEXT NOT NULL,
      value TEXT NOT NULL,
      created_at TIMESTAMP NOT NULL
    );
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_29_H_
