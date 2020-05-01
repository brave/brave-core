/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_contribution_info.h"
#include "bat/ledger/internal/database/database_contribution_queue.h"
#include "bat/ledger/internal/database/database_creds_batch.h"
#include "bat/ledger/internal/database/database_media_publisher_info.h"
#include "bat/ledger/internal/database/database_migration.h"
#include "bat/ledger/internal/database/database_pending_contribution.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_publisher_info.h"
#include "bat/ledger/internal/database/database_recurring_tip.h"
#include "bat/ledger/internal/database/database_server_publisher_info.h"
#include "bat/ledger/internal/database/database_sku_order.h"
#include "bat/ledger/internal/database/database_sku_transaction.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

DatabaseMigration::DatabaseMigration(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
  activity_info_ = std::make_unique<DatabaseActivityInfo>(ledger_);
  contribution_queue_ = std::make_unique<DatabaseContributionQueue>(ledger_);
  contribution_info_ = std::make_unique<DatabaseContributionInfo>(ledger_);
  creds_batch_ = std::make_unique<DatabaseCredsBatch>(ledger_);
  media_publisher_info_ = std::make_unique<DatabaseMediaPublisherInfo>(ledger_);
  pending_contribution_ =
      std::make_unique<DatabasePendingContribution>(ledger_);
  promotion_ = std::make_unique<DatabasePromotion>(ledger_);
  publisher_info_ = std::make_unique<DatabasePublisherInfo>(ledger_);
  recurring_tip_ = std::make_unique<DatabaseRecurringTip>(ledger_);
  server_publisher_info_ =
      std::make_unique<DatabaseServerPublisherInfo>(ledger_);
  sku_order_ = std::make_unique<DatabaseSKUOrder>(ledger_);
  sku_transaction_ = std::make_unique<DatabaseSKUTransaction>(ledger_);
  unblinded_token_ =
      std::make_unique<DatabaseUnblindedToken>(ledger_);
}

DatabaseMigration::~DatabaseMigration() = default;

void DatabaseMigration::Start(
    const int table_version,
    ledger::ResultCallback callback) {
  const int start_version = table_version + 1;
  auto transaction = ledger::DBTransaction::New();
  int migrated_version = table_version;
  const int target_version = GetCurrentVersion();

  if (target_version == table_version) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  for (auto i = start_version; i <= target_version; i++) {
    if (!Migrate(transaction.get(), i)) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "DB: Error with MigrateV" << (i - 1) << "toV" << i;
      break;
    }

    BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
    "DB: Migrated to version " << i;

    migrated_version = i;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::MIGRATE;

  transaction->version = migrated_version;
  transaction->compatible_version = GetCompatibleVersion();
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

bool DatabaseMigration::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  if (!activity_info_->Migrate(transaction, target)) {
    return false;
  }

  if (!contribution_info_->Migrate(transaction, target)) {
    return false;
  }

  if (!contribution_queue_->Migrate(transaction, target)) {
    return false;
  }

  if (!creds_batch_->Migrate(transaction, target)) {
    return false;
  }

  if (!media_publisher_info_->Migrate(transaction, target)) {
    return false;
  }
  if (!pending_contribution_->Migrate(transaction, target)) {
    return false;
  }

  if (!promotion_->Migrate(transaction, target)) {
    return false;
  }

  if (!publisher_info_->Migrate(transaction, target)) {
    return false;
  }

  if (!recurring_tip_->Migrate(transaction, target)) {
    return false;
  }

  if (!server_publisher_info_->Migrate(transaction, target)) {
    return false;
  }

  if (!sku_order_->Migrate(transaction, target)) {
    return false;
  }


  if (!sku_transaction_->Migrate(transaction, target)) {
    return false;
  }

  if (!unblinded_token_->Migrate(transaction, target)) {
    return false;
  }

  return true;
}

}  // namespace braveledger_database
