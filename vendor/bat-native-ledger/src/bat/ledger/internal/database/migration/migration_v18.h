/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V18_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V18_H_

namespace ledger {
namespace database {
namespace migration {

const char v18[] = R"(
  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS creds_batch;
  PRAGMA foreign_keys = on;

  CREATE TABLE creds_batch (creds_id TEXT PRIMARY KEY NOT NULL,
    trigger_id TEXT NOT NULL,
    trigger_type INT NOT NULL,
    creds TEXT NOT NULL,
    blinded_creds TEXT NOT NULL,
    signed_creds TEXT,
    public_key TEXT,
    batch_proof TEXT,
    status INT NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT creds_batch_unique
      UNIQUE (trigger_id, trigger_type)
  );

  CREATE INDEX creds_batch_trigger_id_index ON creds_batch (trigger_id);

  CREATE INDEX creds_batch_trigger_type_index ON creds_batch (trigger_type);

  INSERT INTO creds_batch (creds_id, trigger_id, trigger_type, creds,
  blinded_creds, signed_creds, public_key, batch_proof)
  SELECT hex(randomblob(16)), promotion_id, 1, tokens, blinded_creds,
  signed_creds, public_key, batch_proof FROM promotion_creds;

  UPDATE creds_batch as cb SET status =
  (SELECT p.status FROM promotion as p WHERE cb.trigger_id = p.promotion_id);

  ALTER TABLE promotion ADD claim_id TEXT;

  UPDATE promotion as p SET claim_id =
  (SELECT claim_id FROM promotion_creds as pc
  WHERE pc.promotion_id = p.promotion_id);

  UPDATE promotion SET status = 1 WHERE status = 2 OR status = 3;

  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS promotion_creds;
  PRAGMA foreign_keys = on;

  ALTER TABLE unblinded_tokens ADD creds_id TEXT;

  ALTER TABLE unblinded_tokens ADD expires_at TIMESTAMP NOT NULL DEFAULT 0;

  UPDATE unblinded_tokens as ut SET creds_id =
  (SELECT creds_id FROM creds_batch as cb WHERE
  cb.trigger_id = ut.promotion_id), expires_at = IFNULL((SELECT p.expires_at
  FROM promotion as p WHERE p.promotion_id = ut.promotion_id AND p.type = 0),
  0);

  ALTER TABLE unblinded_tokens RENAME TO unblinded_tokens_temp;

  DROP INDEX IF EXISTS unblinded_tokens_promotion_id_index;

  CREATE TABLE unblinded_tokens (
    token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    token_value TEXT,
    public_key TEXT,
    value DOUBLE NOT NULL DEFAULT 0,
    creds_id TEXT,
    expires_at TIMESTAMP NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
  );

  CREATE INDEX unblinded_tokens_creds_id_index ON unblinded_tokens (creds_id);

  PRAGMA foreign_keys = off;
    INSERT INTO unblinded_tokens (created_at, creds_id, expires_at, public_key,
    token_id, token_value, value) SELECT created_at, creds_id, expires_at,
    public_key, token_id, token_value, value FROM unblinded_tokens_temp;

    DROP TABLE unblinded_tokens_temp;
  PRAGMA foreign_keys = on;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V18_H_
