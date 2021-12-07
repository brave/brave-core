/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_19_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_19_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade19 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 19;

  static inline const char kSQL[] = R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_order;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_order (
      order_id TEXT NOT NULL PRIMARY KEY,
      total_amount DOUBLE,
      merchant_id TEXT,
      location TEXT,
      status INTEGER NOT NULL DEFAULT 0,
      contribution_id TEXT,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_order_items;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_order_items (
      order_item_id TEXT NOT NULL,
      order_id TEXT NOT NULL,
      sku TEXT,
      quantity INTEGER,
      price DOUBLE,
      name TEXT,
      description TEXT,
      type INTEGER,
      expires_at TIMESTAMP,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      CONSTRAINT sku_order_items_unique UNIQUE (order_item_id,order_id)
    );

    CREATE INDEX sku_order_items_order_id_index ON sku_order_items (order_id);

    CREATE INDEX sku_order_items_order_item_id_index
      ON sku_order_items (order_item_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_transaction;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_transaction (
      transaction_id TEXT NOT NULL PRIMARY KEY,
      order_id TEXT NOT NULL,
      external_transaction_id TEXT NOT NULL,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      status INTEGER NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    CREATE INDEX sku_transaction_order_id_index ON sku_transaction (order_id);
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_19_H_
