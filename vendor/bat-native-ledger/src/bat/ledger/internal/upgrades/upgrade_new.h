/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_NEW_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_NEW_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"

namespace ledger {

class UpgradeNew : public BATLedgerJob<bool> {
 public:
  static inline const char kSQL[] = R"sql(

    CREATE TABLE publisher_info (
      publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
      excluded INTEGER NOT NULL DEFAULT 0,
      name TEXT NOT NULL,
      favIcon TEXT NOT NULL,
      url TEXT NOT NULL,
      provider TEXT NOT NULL
    );

    CREATE TABLE promotion (
      promotion_id TEXT NOT NULL PRIMARY KEY,
      version INTEGER NOT NULL,
      type INTEGER NOT NULL,
      public_keys TEXT NOT NULL,
      suggestions INTEGER NOT NULL DEFAULT 0,
      approximate_value DOUBLE NOT NULL DEFAULT 0,
      status INTEGER NOT NULL DEFAULT 0,
      expires_at TIMESTAMP NOT NULL,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      claimed_at TIMESTAMP,
      claim_id TEXT,
      legacy BOOLEAN NOT NULL DEFAULT 0,
      claimable_until INTEGER
    );

    CREATE INDEX promotion_promotion_id_index ON promotion (promotion_id);

    CREATE TABLE contribution_info (
      contribution_id TEXT NOT NULL PRIMARY KEY,
      amount DOUBLE NOT NULL,
      type INTEGER NOT NULL,
      step INTEGER NOT NULL DEFAULT -1,
      retry_count INTEGER NOT NULL DEFAULT -1,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      processor INTEGER NOT NULL DEFAULT 1
    );

    CREATE TABLE activity_info (
      publisher_id LONGVARCHAR NOT NULL,
      duration INTEGER NOT NULL DEFAULT 0,
      visits INTEGER NOT NULL DEFAULT 0,
      score DOUBLE NOT NULL DEFAULT 0,
      percent INTEGER NOT NULL DEFAULT 0,
      weight DOUBLE NOT NULL DEFAULT 0,
      reconcile_stamp INTEGER NOT NULL DEFAULT 0,
      CONSTRAINT activity_unique UNIQUE (publisher_id, reconcile_stamp)
    );

    CREATE INDEX activity_info_publisher_id_index
    ON activity_info (publisher_id);

    CREATE TABLE media_publisher_info (
      media_key TEXT NOT NULL PRIMARY KEY UNIQUE,
      publisher_id LONGVARCHAR NOT NULL
    );

    CREATE INDEX media_publisher_info_media_key_index
    ON media_publisher_info (media_key);

    CREATE INDEX media_publisher_info_publisher_id_index
    ON media_publisher_info (publisher_id);

    CREATE TABLE pending_contribution (
      pending_contribution_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      publisher_id LONGVARCHAR NOT NULL,
      amount DOUBLE NOT NULL DEFAULT 0,
      added_date INTEGER NOT NULL DEFAULT 0,
      viewing_id LONGVARCHAR NOT NULL,
      type INTEGER NOT NULL
    );

    CREATE INDEX pending_contribution_publisher_id_index
    ON pending_contribution (publisher_id);

    CREATE TABLE recurring_donation (
      publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
      amount DOUBLE NOT NULL DEFAULT 0,
      added_date INTEGER NOT NULL DEFAULT 0
    );

    CREATE INDEX recurring_donation_publisher_id_index
    ON recurring_donation (publisher_id);

    CREATE TABLE server_publisher_banner (
      publisher_key LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,
      title TEXT,
      description TEXT,
      background TEXT,
      logo TEXT
    );

    CREATE INDEX server_publisher_banner_publisher_key_index
    ON server_publisher_banner (publisher_key);

    CREATE TABLE server_publisher_links (
      publisher_key LONGVARCHAR NOT NULL,
      provider TEXT,
      link TEXT,
      CONSTRAINT server_publisher_links_unique UNIQUE (publisher_key, provider)
    );

    CREATE INDEX server_publisher_links_publisher_key_index
    ON server_publisher_links (publisher_key);

    CREATE TABLE server_publisher_amounts (
      publisher_key LONGVARCHAR NOT NULL,
      amount DOUBLE NOT NULL DEFAULT 0,
      CONSTRAINT server_publisher_amounts_unique UNIQUE (publisher_key, amount)
    );

    CREATE INDEX server_publisher_amounts_publisher_key_index
    ON server_publisher_amounts (publisher_key);

    CREATE TABLE creds_batch (
      creds_id TEXT NOT NULL PRIMARY KEY,
      trigger_id TEXT NOT NULL,
      trigger_type INT NOT NULL,
      creds TEXT NOT NULL,
      blinded_creds TEXT NOT NULL,
      signed_creds TEXT,
      public_key TEXT,
      batch_proof TEXT,
      status INT NOT NULL DEFAULT 0,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      CONSTRAINT creds_batch_unique UNIQUE (trigger_id, trigger_type)
    );

    CREATE INDEX creds_batch_trigger_id_index ON creds_batch (trigger_id);

    CREATE INDEX creds_batch_trigger_type_index ON creds_batch (trigger_type);

    CREATE TABLE sku_order (
      order_id TEXT NOT NULL PRIMARY KEY,
      total_amount DOUBLE,
      merchant_id TEXT,
      location TEXT,
      status INTEGER NOT NULL DEFAULT 0,
      contribution_id TEXT,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

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

    CREATE TABLE processed_publisher (
      publisher_key TEXT NOT NULL PRIMARY KEY,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE contribution_queue (
      contribution_queue_id TEXT NOT NULL PRIMARY KEY,
      type INTEGER NOT NULL,
      amount DOUBLE NOT NULL,
      partial INTEGER NOT NULL DEFAULT 0,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      completed_at TIMESTAMP NOT NULL DEFAULT 0
    );

    CREATE TABLE contribution_queue_publishers (
      contribution_queue_id TEXT NOT NULL,
      publisher_key TEXT NOT NULL,
      amount_percent DOUBLE NOT NULL
    );

    CREATE INDEX contribution_queue_publishers_contribution_queue_id_index
    ON contribution_queue_publishers (contribution_queue_id);

    CREATE INDEX contribution_queue_publishers_publisher_key_index
    ON contribution_queue_publishers (publisher_key);

    CREATE TABLE unblinded_tokens (
      token_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      token_value TEXT,
      public_key TEXT,
      value DOUBLE NOT NULL DEFAULT 0,
      creds_id TEXT,
      expires_at TIMESTAMP NOT NULL DEFAULT 0,
      created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
      redeemed_at TIMESTAMP NOT NULL DEFAULT 0,
      redeem_id TEXT,
      redeem_type INTEGER NOT NULL DEFAULT 0,
      reserved_at TIMESTAMP NOT NULL DEFAULT 0,
      CONSTRAINT unblinded_tokens_unique UNIQUE (token_value, public_key)
    );

    CREATE INDEX unblinded_tokens_creds_id_index ON unblinded_tokens (creds_id);

    CREATE INDEX unblinded_tokens_redeem_id_index
    ON unblinded_tokens (redeem_id);

    CREATE TABLE server_publisher_info (
      publisher_key LONGVARCHAR NOT NULL PRIMARY KEY,
      status INTEGER NOT NULL DEFAULT 0,
      address TEXT NOT NULL,
      updated_at TIMESTAMP NOT NULL
    );

    CREATE TABLE publisher_prefix_list (
      hash_prefix BLOB NOT NULL PRIMARY KEY
    );

    CREATE TABLE event_log (
      event_log_id LONGVARCHAR NOT NULL PRIMARY KEY,
      key TEXT NOT NULL,
      value TEXT NOT NULL,
      created_at TIMESTAMP NOT NULL
    );

    CREATE TABLE job_state (
      job_id TEXT NOT NULL PRIMARY KEY,
      job_type TEXT NOT NULL,
      state TEXT,
      error TEXT,
      created_at TEXT NOT NULL,
      completed_at TEXT
    );

    CREATE INDEX job_state_job_type_index ON job_state (job_type);

  )sql";

  void Start(int version) {
    CompleteWithFuture(context().StartJob<MigrationJob>(version, kSQL));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_NEW_H_
