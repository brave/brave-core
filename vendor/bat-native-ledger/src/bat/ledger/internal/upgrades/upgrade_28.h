/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_28_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_28_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

// Removes all "server publisher info" data (previously downloaded in a giant
// JSON file) and adds support for publisher prefix lists and publisher data
// stored in a private CDN.
class Upgrade28 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 28;

  static inline const char kSQL[] = R"sql(
    DELETE FROM server_publisher_info
    WHERE status = 0 OR publisher_key NOT IN (
      SELECT publisher_id FROM publisher_info
    );

    ALTER TABLE server_publisher_info RENAME TO server_publisher_info_temp;

    CREATE TABLE server_publisher_info (
      publisher_key LONGVARCHAR NOT NULL PRIMARY KEY,
      status INTEGER NOT NULL DEFAULT 0,
      address TEXT NOT NULL,
      updated_at TIMESTAMP NOT NULL
    );

    INSERT OR IGNORE INTO server_publisher_info
      (publisher_key, status, address, updated_at)
    SELECT publisher_key, status, address, 0
    FROM server_publisher_info_temp;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_info_temp;
    PRAGMA foreign_keys = on;

    DELETE FROM server_publisher_banner
    WHERE publisher_key NOT IN (
      SELECT publisher_key FROM server_publisher_info
    );

    DELETE FROM server_publisher_links
    WHERE publisher_key NOT IN (
      SELECT publisher_key FROM server_publisher_info
    );

    DELETE FROM server_publisher_amounts
    WHERE publisher_key NOT IN (
      SELECT publisher_key FROM server_publisher_info
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS publisher_prefix_list;
    PRAGMA foreign_keys = on;

    CREATE TABLE publisher_prefix_list (
      hash_prefix BLOB NOT NULL PRIMARY KEY
    );
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_28_H_
