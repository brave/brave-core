/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_7_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_7_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade7 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 7;

  static inline const char kSQL[] = R"sql(
    ALTER TABLE publisher_info RENAME TO publisher_info_old;

    CREATE TABLE publisher_info (
      publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
      excluded INTEGER NOT NULL DEFAULT 0,
      name TEXT NOT NULL,
      favIcon TEXT NOT NULL,
      url TEXT NOT NULL,
      provider TEXT NOT NULL
    );

    PRAGMA foreign_keys = off;
      INSERT INTO publisher_info (excluded, favIcon, name, provider,
        publisher_id, url)
      SELECT excluded, favIcon, name, provider, publisher_id, url
      FROM publisher_info_old;

      DROP TABLE publisher_info_old;
    PRAGMA foreign_keys = on;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE server_publisher_info (
      publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,
      status INTEGER DEFAULT 0 NOT NULL,
      excluded INTEGER DEFAULT 0 NOT NULL,
      address TEXT NOT NULL
    );

    CREATE INDEX server_publisher_info_publisher_key_index ON
      server_publisher_info (publisher_key);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_banner;
    PRAGMA foreign_keys = on;

    CREATE TABLE server_publisher_banner (
      publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,
      title TEXT,
      description TEXT,
      background TEXT,
      logo TEXT,
      CONSTRAINT fk_server_publisher_banner_publisher_key
        FOREIGN KEY (publisher_key)
        REFERENCES server_publisher_info (publisher_key)
        ON DELETE CASCADE
    );

    CREATE INDEX server_publisher_banner_publisher_key_index
      ON server_publisher_banner (publisher_key);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_links;
    PRAGMA foreign_keys = on;

    CREATE TABLE server_publisher_links (
      publisher_key LONGVARCHAR NOT NULL,
      provider TEXT,
      link TEXT,
      CONSTRAINT server_publisher_links_unique
        UNIQUE (publisher_key, provider)
      CONSTRAINT fk_server_publisher_links_publisher_key
        FOREIGN KEY (publisher_key)
        REFERENCES server_publisher_info (publisher_key)
        ON DELETE CASCADE
    );

    CREATE INDEX server_publisher_links_publisher_key_index
      ON server_publisher_links (publisher_key);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_amounts;
    PRAGMA foreign_keys = on;

    CREATE TABLE server_publisher_amounts (
      publisher_key LONGVARCHAR NOT NULL,
      amount DOUBLE DEFAULT 0 NOT NULL,
      CONSTRAINT server_publisher_amounts_unique
        UNIQUE (publisher_key, amount)
      CONSTRAINT fk_server_publisher_amounts_publisher_key
        FOREIGN KEY (publisher_key)
        REFERENCES server_publisher_info (publisher_key)
        ON DELETE CASCADE
    );

    CREATE INDEX server_publisher_amounts_publisher_key_index
      ON server_publisher_amounts (publisher_key);
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_7_H_
