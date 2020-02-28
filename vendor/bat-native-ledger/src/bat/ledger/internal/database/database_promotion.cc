/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char table_name_[] = "promotion";
const char creds_table_name_[] = "promotion_creds";

}  // namespace

DatabasePromotion::DatabasePromotion(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    creds_(std::make_unique<DatabasePromotionCreds>(ledger)) {
}

DatabasePromotion::~DatabasePromotion() = default;

bool DatabasePromotion::CreateTableV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "promotion_id TEXT NOT NULL,"
        "version INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "public_keys TEXT NOT NULL,"
        "suggestions INTEGER NOT NULL DEFAULT 0,"
        "approximate_value DOUBLE NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL DEFAULT 0,"
        "expires_at TIMESTAMP NOT NULL,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (promotion_id)"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::CreateIndexV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "promotion_id");
}

bool DatabasePromotion::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 10: {
      return MigrateToV10(transaction);
    }
    case 13: {
      return MigrateToV13(transaction);
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

bool DatabasePromotion::MigrateToV10(ledger::DBTransaction* transaction) {
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

  if (!creds_->Migrate(transaction, 10)) {
    return false;
  }

  return true;
}

bool DatabasePromotion::MigrateToV13(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const char column[] = "claimed_at";
  const std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD %s TIMESTAMP",
      table_name_,
      column);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::MigrateToV14(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "UPDATE %s SET approximate_value = "
      "(SELECT (suggestions * 0.25) FROM %s as ps "
      "WHERE ps.promotion_id = %s.promotion_id)",
      table_name_,
      table_name_,
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return creds_->Migrate(transaction, 15);
}

void DatabasePromotion::InsertOrUpdate(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, info->version);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindString(command.get(), 3, info->public_keys);
  BindInt64(command.get(), 4, info->suggestions);
  BindDouble(command.get(), 5, info->approximate_value);
  BindInt(command.get(), 6, static_cast<int>(info->status));
  BindInt64(command.get(), 7, info->expires_at);
  BindInt64(command.get(), 8, info->claimed_at);

  transaction->commands.push_back(std::move(command));

  if (info->credentials) {
    creds_->InsertOrUpdate(
        transaction.get(),
        info->credentials->Clone(),
        info->id);
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::GetRecord(
    const std::string& id,
    ledger::GetPromotionCallback callback) {
  if (id.empty()) {
    return callback({});
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "p.promotion_id, p.version, p.type, p.public_keys, p.suggestions, "
      "p.approximate_value, p.status, p.expires_at, p.claimed_at,"
      "c.tokens, c.blinded_creds, c.signed_creds, c.public_key, c.batch_proof,"
      "c.claim_id "
      "FROM %s as p LEFT JOIN %s as c ON p.promotion_id = c.promotion_id "
      "WHERE p.promotion_id=?",
      table_name_,
      creds_table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetPromotionCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = ledger::Promotion::New();
  info->id = GetStringColumn(record, 0);
  info->version = GetIntColumn(record, 1);
  info->type = static_cast<ledger::PromotionType>(GetIntColumn(record, 2));
  info->public_keys = GetStringColumn(record, 3);
  info->suggestions = GetInt64Column(record, 4);
  info->approximate_value = GetDoubleColumn(record, 5);
  info->status = static_cast<ledger::PromotionStatus>(GetIntColumn(record, 6));
  info->expires_at = GetInt64Column(record, 7);
  info->claimed_at = GetInt64Column(record, 8);

  const std::string tokens = GetStringColumn(record, 9);
  if (!tokens.empty()) {
    auto creds = ledger::PromotionCreds::New();
    creds->tokens = tokens;
    creds->blinded_creds = GetStringColumn(record, 10);
    creds->signed_creds = GetStringColumn(record, 11);
    creds->public_key = GetStringColumn(record, 12);
    creds->batch_proof = GetStringColumn(record, 13);
    creds->claim_id = GetStringColumn(record, 14);
    info->credentials = std::move(creds);
  }

  callback(std::move(info));
}

void DatabasePromotion::GetAllRecords(
    ledger::GetAllPromotionsCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "p.promotion_id, p.version, p.type, p.public_keys, p.suggestions, "
      "p.approximate_value, p.status, p.expires_at, p.claimed_at,"
      "c.tokens, c.blinded_creds, c.signed_creds, c.public_key, c.batch_proof,"
      "c.claim_id "
      "FROM %s as p LEFT JOIN %s as c ON p.promotion_id = c.promotion_id",
      table_name_,
      creds_table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetAllPromotionsCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::PromotionMap map;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<ledger::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<ledger::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->expires_at = GetInt64Column(record_pointer, 7);
    info->claimed_at = GetInt64Column(record_pointer, 8);

    const std::string tokens = GetStringColumn(record_pointer, 9);
    if (!tokens.empty()) {
      auto creds = ledger::PromotionCreds::New();
      creds->tokens = tokens;
      creds->blinded_creds = GetStringColumn(record_pointer, 10);
      creds->signed_creds = GetStringColumn(record_pointer, 11);
      creds->public_key = GetStringColumn(record_pointer, 12);
      creds->batch_proof = GetStringColumn(record_pointer, 13);
      creds->claim_id = GetStringColumn(record_pointer, 14);
      info->credentials = std::move(creds);
    }

    map.insert(std::make_pair(info->id, std::move(info)));
  }

  callback(std::move(map));
}

void DatabasePromotion::DeleteRecordList(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE promotion_id IN (%s)",
      table_name_,
      GenerateStringInCase(ids).c_str());

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  creds_->DeleteRecordListByPromotion(transaction.get(), ids);

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
