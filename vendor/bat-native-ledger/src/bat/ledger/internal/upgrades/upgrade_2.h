/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_2_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_2_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade2 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 2;

  static inline const char kSQL[] = R"sql(
    ALTER TABLE activity_info ADD reconcile_stamp INTEGER DEFAULT 0 NOT NULL;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE contribution_info (
      publisher_id LONGVARCHAR,
      probi TEXT '0'  NOT NULL,
      date INTEGER NOT NULL,
      category INTEGER NOT NULL,
      month INTEGER NOT NULL,
      year INTEGER NOT NULL,
      CONSTRAINT fk_contribution_info_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX contribution_info_publisher_id_index
      ON contribution_info (publisher_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS recurring_donation;
    PRAGMA foreign_keys = on;

    CREATE TABLE recurring_donation (
      publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
      amount DOUBLE DEFAULT 0 NOT NULL,
      added_date INTEGER DEFAULT 0 NOT NULL,
      CONSTRAINT fk_recurring_donation_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX recurring_donation_publisher_id_index
      ON recurring_donation (publisher_id);
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_2_H_
