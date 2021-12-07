/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_21_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_21_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade21 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 21;

  static inline const char kSQL[] = R"sql(
    ALTER TABLE contribution_info_publishers
      RENAME TO contribution_info_publishers_temp;

    DROP INDEX IF EXISTS contribution_info_publishers_contribution_id_index;

    DROP INDEX IF EXISTS contribution_info_publishers_publisher_key_index;

    CREATE TABLE contribution_info_publishers (
      contribution_id TEXT NOT NULL,
      publisher_key TEXT NOT NULL,
      total_amount DOUBLE NOT NULL,
      contributed_amount DOUBLE,
      CONSTRAINT contribution_info_publishers_unique
        UNIQUE (contribution_id, publisher_key)
    );

    CREATE INDEX contribution_info_publishers_contribution_id_index
      ON contribution_info_publishers (contribution_id);

    CREATE INDEX contribution_info_publishers_publisher_key_index
      ON contribution_info_publishers (publisher_key);

    INSERT OR IGNORE INTO contribution_info_publishers (contribution_id,
      publisher_key, total_amount, contributed_amount)
    SELECT contribution_id, publisher_key, total_amount, contributed_amount
    FROM contribution_info_publishers_temp;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_info_publishers_temp;
    PRAGMA foreign_keys = on;
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_21_H_
