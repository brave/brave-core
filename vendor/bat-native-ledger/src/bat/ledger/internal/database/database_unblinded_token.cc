/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char table_name_[] = "unblinded_tokens";

int64_t GetExpirationDate(const int32_t type, const int64_t stamp) {
  const auto promotion_type = static_cast<ledger::PromotionType>(type);
  if (promotion_type == ledger::PromotionType::ADS) {
    return 0;
  }

  return stamp;
}

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
      table_name_,
      table_name_);

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
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseUnblindedToken::CreateIndexV10(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "token_id");
}

bool DatabaseUnblindedToken::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "promotion_id");
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
    default: {
      return true;
    }
  }
}

bool DatabaseUnblindedToken::MigrateToV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
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

bool DatabaseUnblindedToken::MigrateToV14(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "UPDATE %s SET value = 0.25",
      table_name_);

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
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS unblinded_tokens_token_id_index;";
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
      table_name_,
      columns,
      true)) {
    return false;
  }
  return true;
}

void DatabaseUnblindedToken::InsertOrUpdateList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(token_id, token_value, public_key, value, promotion_id) "
      "VALUES (?, ?, ?, ?, ?)",
      table_name_);

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
    BindString(command.get(), 4, info->promotion_id);

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::GetAllRecords(
    ledger::GetAllUnblindedTokensCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT u.token_id, u.token_value, u.public_key, u.value, "
      "u.promotion_id, p.expires_at, p.type FROM %s as u "
      "LEFT JOIN promotion as p ON p.promotion_id = u.promotion_id",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseUnblindedToken::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetAllUnblindedTokensCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
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
    info->promotion_id = GetStringColumn(record_pointer, 4);
    info->expires_at = GetExpirationDate(
        GetIntColumn(record_pointer, 6),
        GetInt64Column(record_pointer, 5));

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseUnblindedToken::DeleteRecordList(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE token_id IN (%s)",
      table_name_,
      GenerateStringInCase(ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseUnblindedToken::DeleteRecordsForPromotion(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (promotion_id.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE promotion_id = ?",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
