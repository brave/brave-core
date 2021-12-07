/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_10_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_10_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class Upgrade10 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 10;

  static inline const char kSQL[] = R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS promotion;
    PRAGMA foreign_keys = on;

    CREATE TABLE promotion (
      promotion_id TEXT NOT NULL PRIMARY KEY,
      version INTEGER NOT NULL,
      type INTEGER NOT NULL,
      public_keys TEXT NOT NULL,
      suggestions INTEGER NOT NULL DEFAULT 0,
      approximate_value DOUBLE NOT NULL DEFAULT 0,
      status INTEGER NOT NULL DEFAULT 0,
      expires_at TIMESTAMP NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    CREATE INDEX promotion_promotion_id_index ON promotion (promotion_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS promotion_creds;
    PRAGMA foreign_keys = on;

    CREATE TABLE promotion_creds (
      promotion_id TEXT UNIQUE NOT NULL,
      tokens TEXT NOT NULL,
      blinded_creds TEXT NOT NULL,
      signed_creds TEXT,
      public_key TEXT,
      batch_proof TEXT,
      claim_id TEXT,
      CONSTRAINT fk_promotion_creds_promotion_id
        FOREIGN KEY (promotion_id)
        REFERENCES promotion (promotion_id) ON DELETE CASCADE
    );

    CREATE INDEX promotion_creds_promotion_id_index
      ON promotion_creds (promotion_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS unblinded_tokens;
    PRAGMA foreign_keys = on;

    CREATE TABLE unblinded_tokens (
      token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      token_value TEXT,
      public_key TEXT,
      value DOUBLE NOT NULL DEFAULT 0,
      promotion_id TEXT,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      CONSTRAINT fk_unblinded_tokens_promotion_id
        FOREIGN KEY (promotion_id)
        REFERENCES promotion (promotion_id) ON DELETE CASCADE
    );

    CREATE INDEX unblinded_tokens_token_id_index
      ON unblinded_tokens (token_id);
  )sql";

  void Start() {
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_10_H_
