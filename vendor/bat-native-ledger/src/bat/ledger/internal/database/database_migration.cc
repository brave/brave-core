/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "bat/ledger/internal/database/database_migration.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/database/migration/migration_v1.h"
#include "bat/ledger/internal/database/migration/migration_v2.h"
#include "bat/ledger/internal/database/migration/migration_v3.h"
#include "bat/ledger/internal/database/migration/migration_v4.h"
#include "bat/ledger/internal/database/migration/migration_v5.h"
#include "bat/ledger/internal/database/migration/migration_v6.h"
#include "bat/ledger/internal/database/migration/migration_v7.h"
#include "bat/ledger/internal/database/migration/migration_v8.h"
#include "bat/ledger/internal/database/migration/migration_v9.h"
#include "bat/ledger/internal/database/migration/migration_v10.h"
#include "bat/ledger/internal/database/migration/migration_v11.h"
#include "bat/ledger/internal/database/migration/migration_v12.h"
#include "bat/ledger/internal/database/migration/migration_v13.h"
#include "bat/ledger/internal/database/migration/migration_v14.h"
#include "bat/ledger/internal/database/migration/migration_v15.h"
#include "bat/ledger/internal/database/migration/migration_v16.h"
#include "bat/ledger/internal/database/migration/migration_v17.h"
#include "bat/ledger/internal/database/migration/migration_v18.h"
#include "bat/ledger/internal/database/migration/migration_v19.h"
#include "bat/ledger/internal/database/migration/migration_v20.h"
#include "bat/ledger/internal/database/migration/migration_v21.h"
#include "bat/ledger/internal/database/migration/migration_v22.h"
#include "bat/ledger/internal/database/migration/migration_v23.h"
#include "bat/ledger/internal/database/migration/migration_v24.h"
#include "bat/ledger/internal/database/migration/migration_v25.h"
#include "bat/ledger/internal/database/migration/migration_v26.h"
#include "bat/ledger/internal/database/migration/migration_v27.h"
#include "bat/ledger/internal/database/migration/migration_v28.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "third_party/re2/src/re2/re2.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

DatabaseMigration::DatabaseMigration(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

DatabaseMigration::~DatabaseMigration() = default;

void DatabaseMigration::Start(
    const uint32_t table_version,
    ledger::ResultCallback callback) {
  const uint32_t start_version = table_version + 1;
  DCHECK_GT(start_version, 0u);

  auto transaction = ledger::DBTransaction::New();
  int migrated_version = table_version;
  const uint32_t target_version = braveledger_database::GetCurrentVersion();

  if (target_version == table_version) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  const std::vector<std::string> mappings {
    "",
    migration::v1,
    migration::v2,
    migration::v3,
    migration::v4,
    migration::v5,
    migration::v6,
    migration::v7,
    migration::v8,
    migration::v9,
    migration::v10,
    migration::v11,
    migration::v12,
    migration::v13,
    migration::v14,
    migration::v15,
    migration::v16,
    migration::v17,
    migration::v18,
    migration::v19,
    migration::v20,
    migration::v21,
    migration::v22,
    migration::v23,
    migration::v24,
    migration::v25,
    migration::v26,
    migration::v27,
    migration::v28,
  };

  DCHECK_LE(target_version, mappings.size());

  for (auto i = start_version; i <= target_version; i++) {
    GenerateCommand(transaction.get(), mappings[i]);

    // Database migration 28 introduced the publisher hash prefix
    // list and forced a refresh of that table by clearing a timestamp
    // stored in state. The following statement is kept here for legacy
    // compatibility.
    if (i == 28) {
      ledger_->ClearState(ledger::kStateServerPublisherListStamp);
    }

    BLOG(1, "DB: Migrated to version " << i);
    migrated_version = i;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::MIGRATE;

  transaction->version = migrated_version;
  transaction->compatible_version =
      braveledger_database::GetCompatibleVersion();
  transaction->commands.push_back(std::move(command));

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::VACUUM;
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&braveledger_database::OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseMigration::GenerateCommand(
    ledger::DBTransaction* transaction,
    const std::string& query) {
  DCHECK(transaction);

  std::string optimized_query = query;
  // remove all unnecessary spaces and new lines
  re2::RE2::GlobalReplace(&optimized_query, "\\s\\s+", " ");

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = optimized_query;
  transaction->commands.push_back(std::move(command));
}

}  // namespace database
}  // namespace ledger
