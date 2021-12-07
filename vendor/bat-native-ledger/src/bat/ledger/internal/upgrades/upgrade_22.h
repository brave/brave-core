/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_22_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_22_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade22 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 22;

  static inline const char kSQL[] = R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS balance_report_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE balance_report_info (
      balance_report_id LONGVARCHAR NOT NULL PRIMARY KEY,
      grants_ugp DOUBLE NOT NULL DEFAULT 0,
      grants_ads DOUBLE NOT NULL DEFAULT 0,
      auto_contribute DOUBLE NOT NULL DEFAULT 0,
      tip_recurring DOUBLE NOT NULL DEFAULT 0,
      tip DOUBLE NOT NULL DEFAULT 0
    );

    CREATE INDEX balance_report_info_balance_report_id_index
      ON balance_report_info (balance_report_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS processed_publisher;
    PRAGMA foreign_keys = on;

    CREATE TABLE processed_publisher (
      publisher_key TEXT NOT NULL PRIMARY KEY,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_22_H_
