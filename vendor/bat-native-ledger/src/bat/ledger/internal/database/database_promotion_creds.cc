/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_promotion_creds.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "promotion_creds";
const char kParentTableName[] = "promotion";

}  // namespace

DatabasePromotionCreds::DatabasePromotionCreds(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabasePromotionCreds::~DatabasePromotionCreds() = default;

bool DatabasePromotionCreds::CreateTableV10(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "promotion_id TEXT UNIQUE NOT NULL,"
        "tokens TEXT NOT NULL,"
        "blinded_creds TEXT NOT NULL,"
        "signed_creds TEXT,"
        "public_key TEXT,"
        "batch_proof TEXT,"
        "claim_id TEXT,"
        "CONSTRAINT fk_%s_promotion_id "
          "FOREIGN KEY (promotion_id) "
          "REFERENCES %s (promotion_id) ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName,
      kParentTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotionCreds::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "promotion_id TEXT UNIQUE NOT NULL,"
        "tokens TEXT NOT NULL,"
        "blinded_creds TEXT NOT NULL,"
        "signed_creds TEXT,"
        "public_key TEXT,"
        "batch_proof TEXT,"
        "claim_id TEXT"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotionCreds::CreateIndexV10(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "promotion_id");
}

bool DatabasePromotionCreds::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "promotion_id");
}

bool DatabasePromotionCreds::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 10: {
      return MigrateToV10(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    case 18: {
      return MigrateToV18(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabasePromotionCreds::MigrateToV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV10(transaction)) {
    return false;
  }

  if (!CreateIndexV10(transaction)) {
    return false;
  }

  return true;
}

bool DatabasePromotionCreds::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS promotion_creds_promotion_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "promotion_id", "promotion_id" },
    { "tokens", "tokens" },
    { "blinded_creds", "blinded_creds" },
    { "signed_creds", "signed_creds" },
    { "public_key", "public_key" },
    { "batch_proof", "batch_proof" },
    { "claim_id", "claim_id" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    return false;
  }
  return true;
}

bool DatabasePromotionCreds::MigrateToV18(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return DropTable(transaction, kTableName);
}

}  // namespace braveledger_database
