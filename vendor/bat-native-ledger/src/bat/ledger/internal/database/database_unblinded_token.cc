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
      "INSERT OR IGNORE INTO %s "
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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
        "WHERE reserved_at = 0 AND redeemed_at = 0 AND token_id IN (%s) "
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

  query =
      "UPDATE contribution_info SET step=?, retry_count=0 "
      "WHERE contribution_id = ?";

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(
      command.get(),
      0,
      static_cast<int>(ledger::ContributionStep::STEP_RESERVE));
  BindString(command.get(), 1, redeem_id);

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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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
    BLOG(0, "Records size doesn't match");
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
      "WHERE redeem_id = ? AND redeemed_at = 0",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, redeem_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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
      "WHERE ut.redeem_id = ? AND ut.redeemed_at = 0 AND ut.reserved_at != 0",
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace braveledger_database
