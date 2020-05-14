/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <map>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "unblinded_tokens";

}  // namespace

DatabaseUnblindedToken::DatabaseUnblindedToken(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseUnblindedToken::~DatabaseUnblindedToken() = default;

bool DatabaseUnblindedToken::CreateTableV10(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "token_value TEXT,"
        "public_key TEXT,"
        "value DOUBLE NOT NULL DEFAULT 0,"
        "promotion_id TEXT,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CONSTRAINT fk_%s_promotion_id "
          "FOREIGN KEY (promotion_id) "
          "REFERENCES promotion (promotion_id) ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
    "CREATE TABLE %s ("
      "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "token_value TEXT,"
      "public_key TEXT,"
      "value DOUBLE NOT NULL DEFAULT 0,"
      "promotion_id TEXT,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ")",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::CreateTableV18(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
    "CREATE TABLE %s ("
      "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "token_value TEXT,"
      "public_key TEXT,"
      "value DOUBLE NOT NULL DEFAULT 0,"
      "creds_id TEXT,"
      "expires_at TIMESTAMP NOT NULL DEFAULT 0,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ")",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::CreateTableV26(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
    "CREATE TABLE %s ("
      "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "token_value TEXT,"
      "public_key TEXT,"
      "value DOUBLE NOT NULL DEFAULT 0,"
      "creds_id TEXT,"
      "expires_at TIMESTAMP NOT NULL DEFAULT 0,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "redeemed_at TIMESTAMP NOT NULL DEFAULT 0,"
      "redeem_id TEXT,"
      "redeem_type INTEGER NOT NULL DEFAULT 0,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (token_value, public_key)"
    ")",
    kTableName,
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::CreateIndexV10(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "token_id");
}

bool DatabaseUnblindedToken::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "promotion_id");
}

bool DatabaseUnblindedToken::CreateIndexV18(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "creds_id");
}

bool DatabaseUnblindedToken::CreateIndexV20(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, kTableName, "creds_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, kTableName, "redeem_id");
}

bool DatabaseUnblindedToken::CreateIndexV26(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, kTableName, "creds_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, kTableName, "redeem_id");
}

bool DatabaseUnblindedToken::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 10: {
      return MigrateToV10(transaction);
    }
    case 14: {
      return MigrateToV14(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    case 18: {
      return MigrateToV18(transaction);
    }
    case 20: {
      return MigrateToV20(transaction);
    }
    case 27: {
      return MigrateToV27(transaction);
    }
    case 26: {
      return MigrateToV26(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseUnblindedToken::MigrateToV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV10(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV10(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseUnblindedToken::MigrateToV14(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "UPDATE %s SET value = 0.25",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS unblinded_tokens_token_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "token_id", "token_id" },
    { "token_value", "token_value" },
    { "public_key", "public_key" },
    { "value", "value" },
    { "promotion_id", "promotion_id" },
    { "created_at", "created_at" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    BLOG(0, "Table migration failed");
    return false;
  }
  return true;
}

bool DatabaseUnblindedToken::MigrateToV18(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD creds_id TEXT",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "ALTER TABLE %s ADD expires_at TIMESTAMP NOT NULL DEFAULT 0",
      kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "UPDATE %s as ut SET "
      "creds_id = (SELECT creds_id FROM creds_batch as cb "
      "WHERE cb.trigger_id = ut.promotion_id), "
      "expires_at = IFNULL((SELECT p.expires_at FROM promotion as p "
      "WHERE p.promotion_id = ut.promotion_id AND p.type = 0), 0)",
      kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  query =
      "DROP INDEX IF EXISTS unblinded_tokens_promotion_id_index;";
  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV18(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV18(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "token_id", "token_id" },
    { "token_value", "token_value" },
    { "public_key", "public_key" },
    { "value", "value" },
    { "creds_id", "creds_id" },
    { "expires_at", "expires_at" },
    { "created_at", "created_at" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    BLOG(0, "Table migration failed");
    return false;
  }
  return true;
}

bool DatabaseUnblindedToken::MigrateToV20(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  std::string query =
      "DROP INDEX IF EXISTS unblinded_tokens_creds_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));


  query = base::StringPrintf(
      "ALTER TABLE %s ADD redeemed_at TIMESTAMP NOT NULL DEFAULT 0;"
      "ALTER TABLE %s ADD redeem_id TEXT;"
      "ALTER TABLE %s ADD redeem_type INTEGER NOT NULL DEFAULT 0;",
      kTableName,
      kTableName,
      kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateIndexV20(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseUnblindedToken::MigrateToV27(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD reserved_at TIMESTAMP DEFAULT 0 NOT NULL;",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::MigrateToV26(ledger::DBTransaction* transaction) {
  DCHECK(transaction);
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);
  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  std::string query =
      "DROP INDEX IF EXISTS unblinded_tokens_creds_id_index; "
      "DROP INDEX IF EXISTS unblinded_tokens_redeem_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV26(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV26(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(token_id, token_value, public_key, value, creds_id, expires_at, "
      "created_at, redeemed_at, redeem_id, redeem_type) "
      "SELECT token_id, token_value, public_key, value, creds_id, expires_at, "
      "created_at, redeemed_at, redeem_id, redeem_type "
      "FROM %s",
      kTableName,
      temp_table_name.c_str());
  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!DropTable(transaction, temp_table_name)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  return true;
}

void DatabaseUnblindedToken::InsertOrUpdateList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(token_id, token_value, public_key, value, creds_id, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  for (const auto& info : list) {
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    if (info->id != 0) {
      BindInt64(command.get(), 0, info->id);
    } else {
      BindNull(command.get(), 0);
    }

    BindString(command.get(), 1, info->token_value);
    BindString(command.get(), 2, info->public_key);
    BindDouble(command.get(), 3, info->value);
    BindString(command.get(), 4, info->creds_id);
    BindInt64(command.get(), 5, info->expires_at);

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::OnGetRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetUnblindedTokenListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::UnblindedTokenList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::UnblindedToken::New();
    auto* record_pointer = record.get();

    info->id = GetInt64Column(record_pointer, 0);
    info->token_value = GetStringColumn(record_pointer, 1);
    info->public_key = GetStringColumn(record_pointer, 2);
    info->value = GetDoubleColumn(record_pointer, 3);
    info->creds_id = GetStringColumn(record_pointer, 4);
    info->expires_at = GetInt64Column(record_pointer, 5);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseUnblindedToken::GetSpendableRecordsByTriggerIds(
    const std::vector<std::string>& trigger_ids,
    ledger::GetUnblindedTokenListCallback callback) {
  if (trigger_ids.empty()) {
    BLOG(1, "Trigger id is empty");
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ut.token_id, ut.token_value, ut.public_key, ut.value, "
      "ut.creds_id, ut.expires_at FROM %s as ut "
      "LEFT JOIN creds_batch as cb ON cb.creds_id = ut.creds_id "
      "WHERE ut.redeemed_at = 0 AND "
      "(cb.trigger_id IN (%s) OR ut.creds_id IS NULL)",
      kTableName,
      GenerateStringInCase(trigger_ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseUnblindedToken::OnGetRecords,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::MarkRecordListAsSpent(
    const std::vector<std::string>& ids,
    ledger::RewardsType redeem_type,
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(1, "List of ids is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET redeemed_at = ?, redeem_id = ?, redeem_type = ? "
      "WHERE token_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, braveledger_time_util::GetCurrentTimeStamp());
  BindString(command.get(), 1, redeem_id);
  BindInt(command.get(), 2, static_cast<int>(redeem_type));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::MarkRecordListAsReserved(
    const std::vector<std::string>& ids,
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(1, "List of ids is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string id_values = GenerateStringInCase(ids);

  std::string query = base::StringPrintf(
      "UPDATE %s SET redeem_id = ?, reserved_at = ? "
      "WHERE ( "
        "SELECT COUNT(*) FROM %s "
        "WHERE reserved_at = 0 AND token_id IN (%s) "
      ") = ? AND token_id IN (%s)",
      kTableName,
      kTableName,
      id_values.c_str(),
      id_values.c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, redeem_id);
  BindInt64(command.get(), 1, braveledger_time_util::GetCurrentTimeStamp());
  BindInt64(command.get(), 2, ids.size());

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "SELECT token_id FROM %s "
      "WHERE reserved_at != 0 AND token_id IN (%s)",
      kTableName,
      id_values.c_str());

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseUnblindedToken::OnMarkRecordListAsReserved,
          this,
          _1,
          ids.size(),
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::OnMarkRecordListAsReserved(
    ledger::DBCommandResponsePtr response,
    size_t expected_row_count,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (response->result->get_records().size() != expected_row_count) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void DatabaseUnblindedToken::MarkRecordListAsSpendable(
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  if (redeem_id.empty()) {
    BLOG(1, "Redeem id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET redeem_id = '', reserved_at = 0 "
      "WHERE redeem_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, redeem_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::GetReservedRecordList(
    const std::string& redeem_id,
    ledger::GetUnblindedTokenListCallback callback) {
  if (redeem_id.empty()) {
    BLOG(1, "Redeem id is empty");
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ut.token_id, ut.token_value, ut.public_key, ut.value, "
      "ut.creds_id, ut.expires_at FROM %s as ut "
      "WHERE ut.redeem_id = ? AND ut.reserved_at != 0",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, redeem_id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseUnblindedToken::OnGetRecords,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::GetSpendableRecordListByBatchTypes(
    const std::vector<ledger::CredsBatchType>& batch_types,
    ledger::GetUnblindedTokenListCallback callback) {
  if (batch_types.empty()) {
    BLOG(1, "Batch types is empty");
    callback({});
    return;
  }

  std::vector<std::string> in_case;

  for (const auto& type : batch_types) {
    in_case.push_back(std::to_string(static_cast<int>(type)));
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ut.token_id, ut.token_value, ut.public_key, ut.value, "
      "ut.creds_id, ut.expires_at FROM %s as ut "
      "LEFT JOIN creds_batch as cb ON cb.creds_id = ut.creds_id "
      "WHERE ut.redeemed_at = 0 AND "
      "(ut.expires_at > strftime('%%s','now') OR ut.expires_at = 0) AND "
      "(cb.trigger_type IN (%s) OR ut.creds_id IS NULL)",
      kTableName,
      base::JoinString(in_case, ",").c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseUnblindedToken::OnGetRecords,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
