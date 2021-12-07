/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_23_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_23_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade23 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 23;

  static inline const char kSQL[] = R"sql(
    ALTER TABLE contribution_queue RENAME TO contribution_queue_temp;

    CREATE TABLE contribution_queue (
      contribution_queue_id TEXT NOT NULL PRIMARY KEY,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      partial INTEGER NOT NULL DEFAULT 0,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    INSERT INTO contribution_queue (contribution_queue_id, type, amount,
      partial, created_at)
    SELECT CAST(contribution_queue_id AS TEXT), type, amount, partial,
      created_at
    FROM contribution_queue_temp;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_queue_temp;
    PRAGMA foreign_keys = on;

    ALTER TABLE contribution_queue_publishers
      RENAME TO contribution_queue_publishers_temp;

    DROP INDEX IF EXISTS
      contribution_queue_publishers_contribution_queue_id_index;

    DROP INDEX IF EXISTS contribution_queue_publishers_publisher_key_index;

    CREATE TABLE contribution_queue_publishers (
      contribution_queue_id TEXT NOT NULL,
      publisher_key TEXT NOT NULL,
      amount_percent DOUBLE NOT NULL
    );

    CREATE INDEX contribution_queue_publishers_contribution_queue_id_index
      ON contribution_queue_publishers (contribution_queue_id);

    CREATE INDEX contribution_queue_publishers_publisher_key_index
      ON contribution_queue_publishers (publisher_key);

    INSERT INTO contribution_queue_publishers (contribution_queue_id,
      publisher_key, amount_percent)
    SELECT CAST(contribution_queue_id AS TEXT), publisher_key, amount_percent
    FROM contribution_queue_publishers_temp;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_queue_publishers_temp;
    PRAGMA foreign_keys = on;
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_23_H_
