/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_creds_batch.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "creds_batch";

}  // namespace

DatabaseCredsBatch::DatabaseCredsBatch(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseCredsBatch::~DatabaseCredsBatch() = default;

void DatabaseCredsBatch::InsertOrUpdate(
    ledger::CredsBatchPtr creds,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(1, "Creds is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, creds->creds_id);
  BindString(command.get(), 1, creds->trigger_id);
  BindInt(command.get(), 2, static_cast<int>(creds->trigger_type));
  BindString(command.get(), 3, creds->creds);
  BindString(command.get(), 4, creds->blinded_creds);
  BindString(command.get(), 5, creds->signed_creds);
  BindString(command.get(), 6, creds->public_key);
  BindString(command.get(), 7, creds->batch_proof);
  BindInt(command.get(), 8, static_cast<int>(creds->status));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::GetRecordByTrigger(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    ledger::GetCredsBatchCallback callback) {
  DCHECK(!trigger_id.empty());
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s "
      "WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, trigger_id);
  BindInt(command.get(), 1, static_cast<int>(trigger_type));

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseCredsBatch::OnGetRecordByTrigger,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::OnGetRecordByTrigger(
    ledger::DBCommandResponsePtr response,
    ledger::GetCredsBatchCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::CredsBatch::New();
  info->creds_id = GetStringColumn(record, 0);
  info->trigger_id = GetStringColumn(record, 1);
  info->trigger_type =
      static_cast<ledger::CredsBatchType>(GetIntColumn(record, 2));
  info->creds = GetStringColumn(record, 3);
  info->blinded_creds = GetStringColumn(record, 4);
  info->signed_creds = GetStringColumn(record, 5);
  info->public_key = GetStringColumn(record, 6);
  info->batch_proof = GetStringColumn(record, 7);
  info->status =
      static_cast<ledger::CredsBatchStatus>(GetIntColumn(record, 8));

  callback(std::move(info));
}

void DatabaseCredsBatch::SaveSignedCreds(
    ledger::CredsBatchPtr creds,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(1, "Creds is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET signed_creds = ?, public_key = ?, batch_proof = ?, "
      "status = ? WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, creds->signed_creds);
  BindString(command.get(), 1, creds->public_key);
  BindString(command.get(), 2, creds->batch_proof);
  BindInt(command.get(), 3,
      static_cast<int>(ledger::CredsBatchStatus::SIGNED));
  BindString(command.get(), 4, creds->trigger_id);
  BindInt(command.get(), 5, static_cast<int>(creds->trigger_type));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::GetAllRecords(
    ledger::GetCredsBatchListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseCredsBatch::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::OnGetRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetCredsBatchListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::CredsBatchList list;
  ledger::CredsBatchPtr info;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    info = ledger::CredsBatch::New();

    info->creds_id = GetStringColumn(record_pointer, 0);
    info->trigger_id = GetStringColumn(record_pointer, 1);
    info->trigger_type =
        static_cast<ledger::CredsBatchType>(GetIntColumn(record_pointer, 2));
    info->creds = GetStringColumn(record_pointer, 3);
    info->blinded_creds = GetStringColumn(record_pointer, 4);
    info->signed_creds = GetStringColumn(record_pointer, 5);
    info->public_key = GetStringColumn(record_pointer, 6);
    info->batch_proof = GetStringColumn(record_pointer, 7);
    info->status =
        static_cast<ledger::CredsBatchStatus>(GetIntColumn(record_pointer, 8));
    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseCredsBatch::UpdateStatus(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    const ledger::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  if (trigger_id.empty()) {
    BLOG(0, "Trigger id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, trigger_id);
  BindInt(command.get(), 2, static_cast<int>(trigger_type));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::UpdateRecordsStatus(
    const std::vector<std::string>& trigger_ids,
    const ledger::CredsBatchType trigger_type,
    const ledger::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  if (trigger_ids.empty()) {
    BLOG(0, "Trigger id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE trigger_id IN (%s) AND trigger_type = ?",
      kTableName,
      GenerateStringInCase(trigger_ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindInt(command.get(), 1, static_cast<int>(trigger_type));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseCredsBatch::GetRecordsByTriggers(
    const std::vector<std::string>& trigger_ids,
    ledger::GetCredsBatchListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s "
      "WHERE trigger_id IN (%s)",
    kTableName,
    GenerateStringInCase(trigger_ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseCredsBatch::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace braveledger_database
