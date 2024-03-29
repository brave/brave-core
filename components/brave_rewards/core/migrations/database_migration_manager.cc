/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/migrations/database_migration_manager.h"

#include <string>
#include <utility>

namespace brave_rewards::internal {

namespace {

constexpr int kCurrentVersion = 40;

}  // namespace

DatabaseMigrationManager::DatabaseMigrationManager(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {}

DatabaseMigrationManager::~DatabaseMigrationManager() = default;

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<1>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS activity_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE activity_info (
      publisher_id LONGVARCHAR NOT NULL,
      duration INTEGER DEFAULT 0 NOT NULL,
      score DOUBLE DEFAULT 0 NOT NULL,
      percent INTEGER DEFAULT 0 NOT NULL,
      weight DOUBLE DEFAULT 0 NOT NULL,
      category INTEGER NOT NULL,
      month INTEGER NOT NULL,
      year INTEGER NOT NULL,
      CONSTRAINT fk_activity_info_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS media_publisher_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE media_publisher_info (
      media_key TEXT NOT NULL PRIMARY KEY UNIQUE,
      publisher_id LONGVARCHAR NOT NULL,
      CONSTRAINT fk_media_publisher_info_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS publisher_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE publisher_info (
      publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,
      verified BOOLEAN DEFAULT 0 NOT NULL,
      excluded INTEGER DEFAULT 0 NOT NULL,
      name TEXT NOT NULL,
      favIcon TEXT NOT NULL,
      url TEXT NOT NULL,
      provider TEXT NOT NULL
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<2>() {
  return SQLStore::CreateCommand(R"sql(
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
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<3>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS pending_contribution;
    PRAGMA foreign_keys = on;

    CREATE TABLE pending_contribution (
      publisher_id LONGVARCHAR NOT NULL,
      amount DOUBLE DEFAULT 0 NOT NULL,
      added_date INTEGER DEFAULT 0 NOT NULL,
      viewing_id LONGVARCHAR NOT NULL,
      category INTEGER NOT NULL,
      CONSTRAINT fk_pending_contribution_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX pending_contribution_publisher_id_index
      ON pending_contribution (publisher_id);
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<4>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE activity_info RENAME TO activity_info_temp;

    DROP INDEX IF EXISTS activity_info_publisher_id_index;

    CREATE TABLE activity_info (
      publisher_id LONGVARCHAR NOT NULL,
      duration INTEGER DEFAULT 0 NOT NULL,
      visits INTEGER DEFAULT 0 NOT NULL,
      score DOUBLE DEFAULT 0 NOT NULL,
      percent INTEGER DEFAULT 0 NOT NULL,
      weight DOUBLE DEFAULT 0 NOT NULL,
      month INTEGER NOT NULL,
      year INTEGER NOT NULL,
      reconcile_stamp INTEGER DEFAULT 0 NOT NULL,
      CONSTRAINT activity_unique
        UNIQUE (publisher_id, month, year, reconcile_stamp)
      CONSTRAINT fk_activity_info_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX activity_info_publisher_id_index
      ON activity_info (publisher_id);

    PRAGMA foreign_keys = off;
      INSERT INTO activity_info (duration, month, percent, publisher_id,
        reconcile_stamp, score, weight, year)
      SELECT duration, month, percent, publisher_id, reconcile_stamp, score,
        weight, year FROM activity_info_temp;

      DROP TABLE activity_info_temp;
    PRAGMA foreign_keys = on;

    UPDATE activity_info SET visits=5;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<5>() {
  return SQLStore::CreateCommand(R"sql(
    UPDATE activity_info SET visits = 1 WHERE visits = 0;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<6>() {
  return SQLStore::CreateCommand(R"sql(
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
      CONSTRAINT fk_activity_info_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX activity_info_publisher_id_index
      ON activity_info (publisher_id);

    PRAGMA foreign_keys = off;
      INSERT INTO activity_info (publisher_id, reconcile_stamp, duration,
        percent, score, visits, weight)
      SELECT publisher_id, reconcile_stamp, sum(duration) as duration,
        sum(percent) as percent, sum(score) as score, sum(visits) as visits,
        sum(weight) as weight FROM activity_info_temp
      GROUP BY publisher_id, reconcile_stamp;

      DROP TABLE activity_info_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<7>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE publisher_info RENAME TO publisher_info_old;

    CREATE TABLE publisher_info (
      publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,
      excluded INTEGER DEFAULT 0 NOT NULL,
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
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<8>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE contribution_info RENAME TO contribution_info_temp;

    DROP INDEX IF EXISTS contribution_info_publisher_id_index;

    CREATE TABLE contribution_info (
      publisher_id LONGVARCHAR,
      probi TEXT '0'  NOT NULL,
      date INTEGER NOT NULL,
      type INTEGER NOT NULL,
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
      INSERT INTO contribution_info (type, date, month, probi, publisher_id,
        year)
      SELECT category, date, month, probi, publisher_id, year
      FROM contribution_info_temp;

      DROP TABLE contribution_info_temp;
    PRAGMA foreign_keys = on;

    ALTER TABLE pending_contribution RENAME TO pending_contribution_temp;

    DROP INDEX IF EXISTS pending_contribution_publisher_id_index;

    CREATE TABLE pending_contribution (
      publisher_id LONGVARCHAR NOT NULL,
      amount DOUBLE DEFAULT 0 NOT NULL,
      added_date INTEGER DEFAULT 0 NOT NULL,
      viewing_id LONGVARCHAR NOT NULL,
      type INTEGER NOT NULL,
      CONSTRAINT fk_pending_contribution_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX pending_contribution_publisher_id_index
      ON pending_contribution (publisher_id);

    PRAGMA foreign_keys = off;
      INSERT INTO pending_contribution (added_date, amount, type,
        publisher_id, viewing_id)
      SELECT added_date, amount, category, publisher_id, viewing_id
      FROM pending_contribution_temp;

      DROP TABLE pending_contribution_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<9>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_queue;
    PRAGMA foreign_keys = on;

    CREATE TABLE contribution_queue (
      contribution_queue_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      partial INTEGER NOT NULL DEFAULT 0,
      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_queue_publishers;
    PRAGMA foreign_keys = on;

    CREATE TABLE contribution_queue_publishers (
      contribution_queue_id INTEGER NOT NULL,
      publisher_key TEXT NOT NULL,
      amount_percent DOUBLE NOT NULL,
      CONSTRAINT fk_contribution_queue_publishers_publisher_key
        FOREIGN KEY (publisher_key)
        REFERENCES publisher_info (publisher_id),
      CONSTRAINT fk_contribution_queue_publishers_id
        FOREIGN KEY (contribution_queue_id)
        REFERENCES contribution_queue (contribution_queue_id)
        ON DELETE CASCADE
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<10>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS promotion;
    PRAGMA foreign_keys = on;

    CREATE TABLE promotion (
      promotion_id TEXT NOT NULL,
      version INTEGER NOT NULL,
      type INTEGER NOT NULL,
      public_keys TEXT NOT NULL,
      suggestions INTEGER NOT NULL DEFAULT 0,
      approximate_value DOUBLE NOT NULL DEFAULT 0,
      status INTEGER NOT NULL DEFAULT 0,
      expires_at TIMESTAMP NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (promotion_id)
    );

    CREATE INDEX promotion_promotion_id_index
      ON promotion (promotion_id);

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
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<11>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE contribution_info RENAME TO contribution_info_temp;

    DROP INDEX IF EXISTS contribution_info_publisher_id_index;

    CREATE TABLE contribution_info (
      contribution_id TEXT NOT NULL,
      amount DOUBLE NOT NULL,
      type INTEGER NOT NULL,
      step INTEGER NOT NULL DEFAULT -1,
      retry_count INTEGER NOT NULL DEFAULT -1,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (contribution_id)
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_info_publishers;
    PRAGMA foreign_keys = on;

    CREATE TABLE contribution_info_publishers (
      contribution_id TEXT NOT NULL,
      publisher_key TEXT NOT NULL,
      total_amount DOUBLE NOT NULL,
      contributed_amount DOUBLE,
      CONSTRAINT fk_contribution_info_publishers_contribution_id
        FOREIGN KEY (contribution_id)
        REFERENCES contribution_info (contribution_id)
        ON DELETE CASCADE,
      CONSTRAINT fk_contribution_info_publishers_publisher_id
        FOREIGN KEY (publisher_key)
        REFERENCES publisher_info (publisher_id)
    );

    CREATE INDEX contribution_info_publishers_contribution_id_index
      ON contribution_info_publishers (contribution_id);

    CREATE INDEX contribution_info_publishers_publisher_key_index
      ON contribution_info_publishers (publisher_key);

    ALTER TABLE contribution_info_temp ADD contribution_id TEXT;

    ALTER TABLE contribution_info_temp ADD amount DOUBLE;

    UPDATE contribution_info_temp SET
    contribution_id = PRINTF('id_%s_%s', date, ABS(RANDOM())),
    amount = CAST(
      PRINTF('%s.%s', SUBSTR(probi, 0, LENGTH(probi)-17),
      SUBSTR(SUBSTR(probi, LENGTH(probi)-17, LENGTH(probi)), 0, 2))
        as decimal
    );

    INSERT INTO contribution_info (contribution_id, amount, type, step,
      retry_count, created_at)
    SELECT contribution_id, amount, type, -1, -1, date
    FROM contribution_info_temp;

    INSERT INTO contribution_info_publishers (contribution_id, publisher_key,
      total_amount, contributed_amount)
    SELECT contribution_id, publisher_id, amount, amount
    FROM contribution_info_temp
    WHERE publisher_id IS NOT NULL AND
    publisher_id != '';

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS contribution_info_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<12>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE pending_contribution RENAME TO pending_contribution_temp;

    DROP INDEX IF EXISTS pending_contribution_publisher_id_index;

    CREATE TABLE pending_contribution (
      pending_contribution_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      publisher_id LONGVARCHAR NOT NULL,
      amount DOUBLE DEFAULT 0 NOT NULL,
      added_date INTEGER DEFAULT 0 NOT NULL,
      viewing_id LONGVARCHAR NOT NULL,
      type INTEGER NOT NULL,
      CONSTRAINT fk_pending_contribution_publisher_id
        FOREIGN KEY (publisher_id)
        REFERENCES publisher_info (publisher_id)
        ON DELETE CASCADE
    );

    CREATE INDEX pending_contribution_publisher_id_index
      ON pending_contribution (publisher_id);

    PRAGMA foreign_keys = off;
      INSERT INTO pending_contribution (added_date, amount, publisher_id,
        type, viewing_id)
      SELECT added_date, amount, publisher_id, type, viewing_id
      FROM pending_contribution_temp;

      DROP TABLE pending_contribution_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<13>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE promotion ADD claimed_at TIMESTAMP;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<14>() {
  return SQLStore::CreateCommand(R"sql(
    UPDATE promotion SET approximate_value = (
      SELECT (suggestions * 0.25)
      FROM promotion as ps
      WHERE ps.promotion_id = promotion.promotion_id);

    UPDATE unblinded_tokens SET value = 0.25;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<15>() {
  return SQLStore::CreateCommand(R"sql(
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
      INSERT INTO activity_info (duration, percent, publisher_id,
        reconcile_stamp, score, visits, weight)
      SELECT duration, percent, publisher_id, reconcile_stamp, score, visits,
        weight
      FROM activity_info_temp;

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
        contribution_id, publisher_key, total_amount)
      SELECT contributed_amount, contribution_id, publisher_key, total_amount
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
      INSERT INTO media_publisher_info (media_key, publisher_id)
      SELECT media_key, publisher_id
      FROM media_publisher_info_temp;

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
        pending_contribution_id, publisher_id, type, viewing_id)
      SELECT added_date, amount, pending_contribution_id, publisher_id, type,
        viewing_id
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
        promotion_id, public_key, signed_creds, tokens)
      SELECT batch_proof, blinded_creds, claim_id, promotion_id, public_key,
        signed_creds, tokens
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
        publisher_key, title)
      SELECT background, description, logo, publisher_key, title
      FROM server_publisher_banner_temp;

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

    ALTER TABLE server_publisher_amounts
    RENAME TO server_publisher_amounts_temp;

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
        token_id, token_value, value)
      SELECT created_at, promotion_id, public_key, token_id, token_value, value
      FROM unblinded_tokens_temp;

      DROP TABLE unblinded_tokens_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<16>() {
  return SQLStore::CreateCommand(R"sql(
    UPDATE contribution_info SET
    created_at = (
      CASE WHEN datetime(created_at, 'unixepoch') IS NULL
      THEN strftime('%s', datetime(created_at))
      ELSE created_at END
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<17>() {
  return SQLStore::CreateCommand(R"sql(
      ALTER TABLE contribution_info ADD processor INTEGER NOT NULL DEFAULT 1;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<18>() {
  return SQLStore::CreateCommand(R"sql(
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
      signed_creds, public_key, batch_proof
    FROM promotion_creds;

    UPDATE creds_batch as cb SET status = (
        SELECT p.status FROM promotion as p
        WHERE cb.trigger_id = p.promotion_id);

    ALTER TABLE promotion ADD claim_id TEXT;

    UPDATE promotion as p SET claim_id = (
      SELECT claim_id FROM promotion_creds as pc
      WHERE pc.promotion_id = p.promotion_id);

    UPDATE promotion SET status = 1 WHERE status = 2 OR status = 3;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS promotion_creds;
    PRAGMA foreign_keys = on;

    ALTER TABLE unblinded_tokens ADD creds_id TEXT;

    ALTER TABLE unblinded_tokens ADD expires_at TIMESTAMP NOT NULL DEFAULT 0;

    UPDATE unblinded_tokens as ut SET creds_id = (
      SELECT creds_id FROM creds_batch as cb
      WHERE cb.trigger_id = ut.promotion_id), expires_at = IFNULL((
        SELECT p.expires_at FROM promotion as p
        WHERE p.promotion_id = ut.promotion_id AND p.type = 0), 0);

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

    CREATE INDEX unblinded_tokens_creds_id_index
    ON unblinded_tokens (creds_id);

    PRAGMA foreign_keys = off;
      INSERT INTO unblinded_tokens (created_at, creds_id, expires_at,
        public_key, token_id, token_value, value)
      SELECT created_at, creds_id, expires_at, public_key, token_id,
        token_value, value
      FROM unblinded_tokens_temp;

      DROP TABLE unblinded_tokens_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<19>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_order;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_order (order_id TEXT NOT NULL,
      total_amount DOUBLE,
      merchant_id TEXT,
      location TEXT,
      status INTEGER NOT NULL DEFAULT 0,
      contribution_id TEXT,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (order_id)
    );

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_order_items;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_order_items (order_item_id TEXT NOT NULL,
      order_id TEXT NOT NULL,
      sku TEXT,
      quantity INTEGER,
      price DOUBLE,
      name TEXT,
      description TEXT,
      type INTEGER,
      expires_at TIMESTAMP,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      CONSTRAINT sku_order_items_unique
        UNIQUE (order_item_id,order_id)
    );

    CREATE INDEX sku_order_items_order_id_index
      ON sku_order_items (order_id);

    CREATE INDEX sku_order_items_order_item_id_index
      ON sku_order_items (order_item_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS sku_transaction;
    PRAGMA foreign_keys = on;

    CREATE TABLE sku_transaction (transaction_id TEXT NOT NULL,
      order_id TEXT NOT NULL,
      external_transaction_id TEXT NOT NULL,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      status INTEGER NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (transaction_id)
    );

    CREATE INDEX sku_transaction_order_id_index ON sku_transaction (order_id);
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<20>() {
  return SQLStore::CreateCommand(R"sql(
    DROP INDEX IF EXISTS unblinded_tokens_creds_id_index;

    ALTER TABLE unblinded_tokens
      ADD redeemed_at TIMESTAMP NOT NULL DEFAULT 0;

    ALTER TABLE unblinded_tokens ADD redeem_id TEXT;

    ALTER TABLE unblinded_tokens ADD redeem_type INTEGER NOT NULL DEFAULT 0;

    CREATE INDEX unblinded_tokens_creds_id_index
      ON unblinded_tokens (creds_id);

    CREATE INDEX unblinded_tokens_redeem_id_index
      ON unblinded_tokens (redeem_id);
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<21>() {
  return SQLStore::CreateCommand(R"sql(
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
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<22>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS balance_report_info;
    PRAGMA foreign_keys = on;

    CREATE TABLE balance_report_info (
      balance_report_id LONGVARCHAR PRIMARY KEY NOT NULL,
      grants_ugp DOUBLE DEFAULT 0 NOT NULL,
      grants_ads DOUBLE DEFAULT 0 NOT NULL,
      auto_contribute DOUBLE DEFAULT 0 NOT NULL,
      tip_recurring DOUBLE DEFAULT 0 NOT NULL,
      tip DOUBLE DEFAULT 0 NOT NULL
    );

    CREATE INDEX balance_report_info_balance_report_id_index
      ON balance_report_info (balance_report_id);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS processed_publisher;
    PRAGMA foreign_keys = on;

    CREATE TABLE processed_publisher (
      publisher_key TEXT PRIMARY KEY NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<23>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE contribution_queue RENAME TO contribution_queue_temp;

    CREATE TABLE contribution_queue (
      contribution_queue_id TEXT PRIMARY KEY NOT NULL,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      partial INTEGER NOT NULL DEFAULT 0,
      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
    );

    INSERT INTO contribution_queue (contribution_queue_id, type, amount,
      partial, created_at)
    SELECT CAST(contribution_queue_id AS TEXT), type, amount, partial,
    created_at FROM contribution_queue_temp;

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
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<24>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE contribution_queue ADD completed_at TIMESTAMP NOT NULL
      DEFAULT 0;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<25>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE promotion ADD legacy BOOLEAN DEFAULT 0 NOT NULL;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<26>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE unblinded_tokens RENAME TO unblinded_tokens_temp;

    DROP INDEX IF EXISTS unblinded_tokens_creds_id_index;

    DROP INDEX IF EXISTS unblinded_tokens_redeem_id_index;

    CREATE TABLE unblinded_tokens (
      token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      token_value TEXT,
      public_key TEXT,
      value DOUBLE NOT NULL DEFAULT 0,
      creds_id TEXT,
      expires_at TIMESTAMP NOT NULL DEFAULT 0,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      redeemed_at TIMESTAMP NOT NULL DEFAULT 0,
      redeem_id TEXT,
      redeem_type INTEGER NOT NULL DEFAULT 0,
      CONSTRAINT unblinded_tokens_unique
        UNIQUE (token_value, public_key)
    );

    CREATE INDEX unblinded_tokens_creds_id_index
      ON unblinded_tokens (creds_id);

    CREATE INDEX unblinded_tokens_redeem_id_index
      ON unblinded_tokens (redeem_id);

    INSERT OR IGNORE INTO unblinded_tokens (token_id, token_value, public_key,
      value, creds_id, expires_at, created_at, redeemed_at, redeem_id,
      redeem_type)
    SELECT token_id, token_value, public_key, value, creds_id, expires_at,
      created_at, redeemed_at, redeem_id, redeem_type
    FROM unblinded_tokens_temp;

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS unblinded_tokens_temp;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<27>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE unblinded_tokens ADD reserved_at TIMESTAMP DEFAULT 0 NOT NULL;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<28>() {
  return SQLStore::CreateCommand(R"sql(
    DELETE FROM server_publisher_info
    WHERE status = 0 OR publisher_key NOT IN (
      SELECT publisher_id FROM publisher_info
    );

    ALTER TABLE server_publisher_info RENAME TO server_publisher_info_temp;

    CREATE TABLE server_publisher_info (
      publisher_key LONGVARCHAR PRIMARY KEY NOT NULL,
      status INTEGER DEFAULT 0 NOT NULL,
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
    WHERE publisher_key NOT IN
      (SELECT publisher_key FROM server_publisher_info);

    DELETE FROM server_publisher_links
    WHERE publisher_key NOT IN
      (SELECT publisher_key FROM server_publisher_info);

    DELETE FROM server_publisher_amounts
    WHERE publisher_key NOT IN
      (SELECT publisher_key FROM server_publisher_info);

    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS publisher_prefix_list;
    PRAGMA foreign_keys = on;

    CREATE TABLE publisher_prefix_list (
      hash_prefix BLOB PRIMARY KEY NOT NULL
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<29>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS event_log;
    PRAGMA foreign_keys = on;

    CREATE TABLE event_log (
      event_log_id LONGVARCHAR PRIMARY KEY NOT NULL,
      key TEXT NOT NULL,
      value TEXT NOT NULL,
      created_at TIMESTAMP NOT NULL
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<30>() {
  std::string country_code;
  client().GetClientCountryCode(&country_code);
  if (country_code != "JP") {
    return nullptr;
  }
  return SQLStore::CreateCommand(R"sql(
    CREATE TABLE unblinded_tokens_bap AS SELECT * from unblinded_tokens;
    DELETE FROM unblinded_tokens;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<31>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE pending_contribution ADD processor INTEGER DEFAULT 0 NOT NULL;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<32>() {
  std::string country_code;
  client().GetClientCountryCode(&country_code);
  if (country_code != "JP") {
    return nullptr;
  }
  return SQLStore::CreateCommand(R"sql(
    CREATE TABLE balance_report_info_bap AS SELECT * from balance_report_info;
    DELETE FROM balance_report_info;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<33>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE pending_contribution DROP COLUMN processor;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<34>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE promotion ADD COLUMN claimable_until INTEGER;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<35>() {
  return SQLStore::CreateCommand(R"sql(
    PRAGMA foreign_keys = off;
      DROP TABLE IF EXISTS server_publisher_amounts;
    PRAGMA foreign_keys = on;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<36>() {
  return SQLStore::CreateCommand(R"sql(
    UPDATE server_publisher_info SET status = 0 WHERE status = 1;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<37>() {
  return SQLStore::CreateCommand(R"sql(
    CREATE TABLE external_transactions (
      transaction_id TEXT NOT NULL CHECK(transaction_id <> ''),
      contribution_id TEXT NOT NULL CHECK(contribution_id <> ''),
      destination TEXT NOT NULL CHECK(destination <> ''),
      amount TEXT NOT NULL CHECK(amount <> ''),
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (contribution_id, destination),
      FOREIGN KEY (contribution_id)
      REFERENCES contribution_info (contribution_id)
      ON UPDATE RESTRICT ON DELETE RESTRICT
    );
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<38>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE recurring_donation ADD COLUMN next_contribution_at TIMESTAMP;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<39>() {
  return SQLStore::CreateCommand(R"sql(
    ALTER TABLE server_publisher_banner ADD COLUMN web3_url TEXT;
  )sql");
}

template <>
mojom::DBCommandPtr DatabaseMigrationManager::Migration<40>() {
  return SQLStore::CreateCommand(R"sql(
    DROP TABLE IF EXISTS pending_contribution;
    DROP TABLE IF EXISTS processed_publisher;
  )sql");
}

void DatabaseMigrationManager::MigrateDatabase(MigrateCallback callback) {
  MigrateDatabaseToVersion(kCurrentVersion, std::move(callback));
}

void DatabaseMigrationManager::MigrateDatabaseForTesting(
    int target_version,
    MigrateCallback callback) {
  MigrateDatabaseToVersion(target_version, std::move(callback));
}

void DatabaseMigrationManager::MigrateDatabaseToVersion(
    int target_version,
    MigrateCallback callback) {
  Get<SQLStore>().Initialize(
      kCurrentVersion, base::BindOnce(&DatabaseMigrationManager::OnInitialized,
                                      weak_factory_.GetWeakPtr(),
                                      target_version, std::move(callback)));
}

int DatabaseMigrationManager::GetCurrentVersionForTesting() {
  return kCurrentVersion;
}

void DatabaseMigrationManager::OnInitialized(int target_version,
                                             MigrateCallback callback,
                                             SQLReader reader) {
  if (!reader.Step()) {
    LogError(FROM_HERE) << "Error initializing database";
    std::move(callback).Run(false);
    return;
  }

  int db_version = reader.ColumnInt(0);

  Log(FROM_HERE) << "Migrating database from version " << db_version
                 << " to version " << target_version;

  auto commands =
      GetMigrationCommands(db_version, target_version,
                           std::make_integer_sequence<int, kCurrentVersion>());

  Get<SQLStore>().Migrate(
      kCurrentVersion, std::move(commands),
      base::BindOnce(&DatabaseMigrationManager::OnMigrationComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

template <int... kVersions>
SQLStore::CommandList DatabaseMigrationManager::GetMigrationCommands(
    int db_version,
    int target_version,
    std::integer_sequence<int, kVersions...>) {
  SQLStore::CommandList commands;
  ((MaybeAddMigration<kVersions + 1>(commands, db_version, target_version)),
   ...);
  return commands;
}

template <int kVersion>
void DatabaseMigrationManager::MaybeAddMigration(
    SQLStore::CommandList& commands,
    int db_version,
    int target_version) {
  static_assert(kVersion > 0 && kVersion <= kCurrentVersion,
                "Invalid database migration version number");

  if (db_version < kVersion && kVersion <= target_version) {
    if (auto command = Migration<kVersion>()) {
      commands.push_back(std::move(command));
    }
  }
}

void DatabaseMigrationManager::OnMigrationComplete(MigrateCallback callback,
                                                   SQLReader reader) {
  if (!reader.Succeeded()) {
    LogError(FROM_HERE) << "Error migrating database";
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(true);
}

}  // namespace brave_rewards::internal
