/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V15_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V15_H_

namespace ledger {
namespace database {
namespace migration {

const char v15[] = R"(
  ALTER TABLE activity_info RENAME TO activity_info_temp;

  DROP INDEX IF EXISTS activity_info_publisher_id_index;

  CREATE TABLE activity_info (
    publisher_id LONGVARCHAR NOT NULL,
    duration INTEGER DEFAULT 0 NOT NULL,
    visits INTEGER DEFAULT 0 NOT NULL,
    score DOUBLE DEFAULT 0 NOT NULL,
    percent INTEGER DEFAULT 0 NOT NULL,
    weight DOUBLE DEFAULT 0 NOT NULL,
    reconcile_stamp INTEGER DEFAULT 0 NOT NULL,
    CONSTRAINT activity_unique
      UNIQUE (publisher_id, reconcile_stamp)
  );

  CREATE INDEX activity_info_publisher_id_index
    ON activity_info (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO activity_info (duration, percent, publisher_id, reconcile_stamp,
    score, visits, weight) SELECT duration, percent, publisher_id,
    reconcile_stamp, score, visits, weight FROM activity_info_temp;

    DROP TABLE activity_info_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE contribution_info_publishers
    RENAME TO contribution_info_publishers_temp;

  DROP INDEX IF EXISTS contribution_info_publishers_contribution_id_index;
  DROP INDEX IF EXISTS contribution_info_publishers_publisher_key_index;

  CREATE TABLE contribution_info_publishers (
    contribution_id TEXT NOT NULL,
    publisher_key TEXT NOT NULL,
    total_amount DOUBLE NOT NULL,
    contributed_amount DOUBLE
  );

  CREATE INDEX contribution_info_publishers_contribution_id_index
    ON contribution_info_publishers (contribution_id);

  CREATE INDEX contribution_info_publishers_publisher_key_index
    ON contribution_info_publishers (publisher_key);

  PRAGMA foreign_keys = off;
    INSERT INTO contribution_info_publishers (contributed_amount,
    contribution_id, publisher_key, total_amount) SELECT contributed_amount,
    contribution_id, publisher_key, total_amount
    FROM contribution_info_publishers_temp;

    DROP TABLE contribution_info_publishers_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE contribution_queue_publishers
    RENAME TO contribution_queue_publishers_temp;

  CREATE TABLE contribution_queue_publishers (
    contribution_queue_id INTEGER NOT NULL,
    publisher_key TEXT NOT NULL,
    amount_percent DOUBLE NOT NULL
  );

  CREATE INDEX contribution_queue_publishers_contribution_queue_id_index
    ON contribution_queue_publishers (contribution_queue_id);

  CREATE INDEX contribution_queue_publishers_publisher_key_index
    ON contribution_queue_publishers (publisher_key);

  PRAGMA foreign_keys = off;
    INSERT INTO contribution_queue_publishers (amount_percent,
    contribution_queue_id, publisher_key) SELECT amount_percent,
    contribution_queue_id, publisher_key FROM
    contribution_queue_publishers_temp;

    DROP TABLE contribution_queue_publishers_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE media_publisher_info RENAME TO media_publisher_info_temp;

  CREATE TABLE media_publisher_info (
    media_key TEXT NOT NULL PRIMARY KEY UNIQUE,
    publisher_id LONGVARCHAR NOT NULL
  );

  CREATE INDEX media_publisher_info_media_key_index
    ON media_publisher_info (media_key);

  CREATE INDEX media_publisher_info_publisher_id_index
    ON media_publisher_info (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO media_publisher_info (media_key, publisher_id) SELECT media_key,
    publisher_id FROM media_publisher_info_temp;

    DROP TABLE media_publisher_info_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE pending_contribution RENAME TO pending_contribution_temp;

  DROP INDEX IF EXISTS pending_contribution_publisher_id_index;

  CREATE TABLE pending_contribution (
    pending_contribution_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    publisher_id LONGVARCHAR NOT NULL,
    amount DOUBLE DEFAULT 0 NOT NULL,
    added_date INTEGER DEFAULT 0 NOT NULL,
    viewing_id LONGVARCHAR NOT NULL,
    type INTEGER NOT NULL
  );

  CREATE INDEX pending_contribution_publisher_id_index
    ON pending_contribution (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO pending_contribution (added_date, amount,
    pending_contribution_id, publisher_id, type, viewing_id) SELECT added_date,
    amount, pending_contribution_id, publisher_id, type, viewing_id
    FROM pending_contribution_temp;

    DROP TABLE pending_contribution_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE promotion_creds RENAME TO promotion_creds_temp;

  DROP INDEX IF EXISTS promotion_creds_promotion_id_index;

  CREATE TABLE promotion_creds (
    promotion_id TEXT UNIQUE NOT NULL,
    tokens TEXT NOT NULL,
    blinded_creds TEXT NOT NULL,
    signed_creds TEXT,
    public_key TEXT,
    batch_proof TEXT,
    claim_id TEXT
  );

  CREATE INDEX promotion_creds_promotion_id_index
    ON promotion_creds (promotion_id);

  PRAGMA foreign_keys = off;
    INSERT INTO promotion_creds (batch_proof, blinded_creds, claim_id,
    promotion_id, public_key, signed_creds, tokens) SELECT batch_proof,
    blinded_creds, claim_id, promotion_id, public_key, signed_creds, tokens
    FROM promotion_creds_temp;

    DROP TABLE promotion_creds_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE recurring_donation RENAME TO recurring_donation_temp;

  DROP INDEX IF EXISTS recurring_donation_publisher_id_index;

  CREATE TABLE recurring_donation (
    publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
    amount DOUBLE DEFAULT 0 NOT NULL,
    added_date INTEGER DEFAULT 0 NOT NULL
  );

  CREATE INDEX recurring_donation_publisher_id_index
    ON recurring_donation (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO recurring_donation (added_date, amount, publisher_id)
    SELECT added_date, amount, publisher_id FROM recurring_donation_temp;

    DROP TABLE recurring_donation_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE server_publisher_banner RENAME TO server_publisher_banner_temp;

  DROP INDEX IF EXISTS server_publisher_banner_publisher_key_index;

  CREATE TABLE server_publisher_banner (
    publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,
    title TEXT,
    description TEXT,
    background TEXT,
    logo TEXT
  );

  CREATE INDEX server_publisher_banner_publisher_key_index
    ON server_publisher_banner (publisher_key);

  PRAGMA foreign_keys = off;
    INSERT INTO server_publisher_banner (background, description, logo,
    publisher_key, title) SELECT background, description, logo, publisher_key,
    title FROM server_publisher_banner_temp;

    DROP TABLE server_publisher_banner_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE server_publisher_links RENAME TO server_publisher_links_temp;

  DROP INDEX IF EXISTS server_publisher_links_publisher_key_index;

  CREATE TABLE server_publisher_links (
    publisher_key LONGVARCHAR NOT NULL,
    provider TEXT,
    link TEXT,
    CONSTRAINT server_publisher_links_unique
      UNIQUE (publisher_key, provider)
  );

  CREATE INDEX server_publisher_links_publisher_key_index
    ON server_publisher_links (publisher_key);

  PRAGMA foreign_keys = off;
    INSERT INTO server_publisher_links (link, provider, publisher_key)
    SELECT link, provider, publisher_key FROM server_publisher_links_temp;

    DROP TABLE server_publisher_links_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE server_publisher_amounts RENAME TO server_publisher_amounts_temp;

  DROP INDEX IF EXISTS server_publisher_amounts_publisher_key_index;

  CREATE TABLE server_publisher_amounts (
    publisher_key LONGVARCHAR NOT NULL,
    amount DOUBLE DEFAULT 0 NOT NULL,
    CONSTRAINT server_publisher_amounts_unique
      UNIQUE (publisher_key, amount)
  );

  CREATE INDEX server_publisher_amounts_publisher_key_index
    ON server_publisher_amounts (publisher_key);

  PRAGMA foreign_keys = off;
    INSERT INTO server_publisher_amounts (amount, publisher_key)
    SELECT amount, publisher_key FROM server_publisher_amounts_temp;

    DROP TABLE server_publisher_amounts_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE unblinded_tokens RENAME TO unblinded_tokens_temp;

  DROP INDEX IF EXISTS unblinded_tokens_token_id_index;

  CREATE TABLE unblinded_tokens (
    token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    token_value TEXT,
    public_key TEXT,
    value DOUBLE NOT NULL DEFAULT 0,
    promotion_id TEXT,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
  );

  CREATE INDEX unblinded_tokens_promotion_id_index
    ON unblinded_tokens (promotion_id);

  PRAGMA foreign_keys = off;
    INSERT INTO unblinded_tokens (created_at, promotion_id, public_key,
    token_id, token_value, value) SELECT created_at, promotion_id, public_key,
    token_id, token_value, value FROM unblinded_tokens_temp;

    DROP TABLE unblinded_tokens_temp;
  PRAGMA foreign_keys = on;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V15_H_
