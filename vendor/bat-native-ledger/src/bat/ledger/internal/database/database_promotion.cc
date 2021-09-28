/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "promotion";

}  // namespace

DatabasePromotion::DatabasePromotion(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabasePromotion::~DatabasePromotion() = default;

void DatabasePromotion::InsertOrUpdate(
    type::PromotionPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(1, "Info is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, expires_at, claimed_at, legacy) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, info->version);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindString(command.get(), 3, info->public_keys);
  BindInt64(command.get(), 4, info->suggestions);
  BindDouble(command.get(), 5, info->approximate_value);
  BindInt(command.get(), 6, static_cast<int>(info->status));
  BindInt64(command.get(), 7, info->created_at);
  BindInt64(command.get(), 8, info->expires_at);
  BindInt64(command.get(), 9, info->claimed_at);
  BindBool(command.get(), 10, info->legacy_claimed);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::GetRecord(
    const std::string& id,
    GetPromotionCallback callback) {
  if (id.empty()) {
    BLOG(1, "Id is empty");
    return callback({});
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, expires_at, claimed_at, "
      "claim_id, legacy "
      "FROM %s WHERE promotion_id=?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, id);

  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::OnGetRecord(
    type::DBCommandResponsePtr response,
    GetPromotionCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = type::Promotion::New();
  info->id = GetStringColumn(record, 0);
  info->version = GetIntColumn(record, 1);
  info->type = static_cast<type::PromotionType>(GetIntColumn(record, 2));
  info->public_keys = GetStringColumn(record, 3);
  info->suggestions = GetInt64Column(record, 4);
  info->approximate_value = GetDoubleColumn(record, 5);
  info->status = static_cast<type::PromotionStatus>(GetIntColumn(record, 6));
  info->created_at = GetInt64Column(record, 7);
  info->expires_at = GetInt64Column(record, 8);
  info->claimed_at = GetInt64Column(record, 9);
  info->claim_id = GetStringColumn(record, 10);
  info->legacy_claimed = GetBoolColumn(record, 11);

  callback(std::move(info));
}

void DatabasePromotion::GetAllRecords(
    ledger::GetAllPromotionsCallback callback) {
  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, expires_at, claimed_at, "
      "claim_id, legacy "
      "FROM %s",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::OnGetAllRecords(
    type::DBCommandResponsePtr response,
    ledger::GetAllPromotionsCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  type::PromotionMap map;
  for (auto const& record : response->result->get_records()) {
    auto info = type::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<type::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<type::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->created_at = GetInt64Column(record_pointer, 7);
    info->expires_at = GetInt64Column(record_pointer, 8);
    info->claimed_at = GetInt64Column(record_pointer, 9);
    info->claim_id = GetStringColumn(record_pointer, 10);
    info->legacy_claimed = GetBoolColumn(record_pointer, 11);

    map.insert(std::make_pair(info->id, std::move(info)));
  }

  callback(std::move(map));
}

void DatabasePromotion::SaveClaimId(
    const std::string& promotion_id,
    const std::string& claim_id,
    ledger::ResultCallback callback) {
  if (promotion_id.empty() || claim_id.empty()) {
    BLOG(1, "Data is empty " << promotion_id << "/" << claim_id);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET claim_id = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, claim_id);
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::UpdateStatus(
    const std::string& promotion_id,
    const type::PromotionStatus status,
    ledger::ResultCallback callback) {
  if (promotion_id.empty()) {
    BLOG(0, "Promotion id is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::UpdateRecordsStatus(
    const std::vector<std::string>& ids,
    const type::PromotionStatus status,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(1, "List of ids is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::CredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (promotion_id.empty()) {
    BLOG(1, "Promotion id is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ?, claimed_at = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  const uint64_t current_time = util::GetCurrentTimeStamp();

  BindInt(command.get(), 0,
      static_cast<int>(type::PromotionStatus::FINISHED));
  BindInt64(command.get(), 1, current_time);
  BindString(command.get(), 2, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::GetRecords(
    const std::vector<std::string>& ids,
    client::GetPromotionListCallback callback) {
  if (ids.empty()) {
    BLOG(1, "List of ids is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, expires_at, claimed_at, "
      "claim_id, legacy "
      "FROM %s WHERE promotion_id IN (%s)",
      kTableName, GenerateStringInCase(ids).c_str());

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::OnGetRecords(
    type::DBCommandResponsePtr response,
    client::GetPromotionListCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  type::PromotionList list;
  type::PromotionPtr info;
  for (auto const& record : response->result->get_records()) {
    info = type::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<type::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<type::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->created_at = GetInt64Column(record_pointer, 7);
    info->expires_at = GetInt64Column(record_pointer, 8);
    info->claimed_at = GetInt64Column(record_pointer, 9);
    info->claim_id = GetStringColumn(record_pointer, 10);
    info->legacy_claimed = GetBoolColumn(record_pointer, 11);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabasePromotion::GetRecordsByType(
    const std::vector<type::PromotionType>& types,
    client::GetPromotionListCallback callback) {
  if (types.empty()) {
    BLOG(1, "List of types is empty");
    callback({});
    return;
  }
  auto transaction = type::DBTransaction::New();

  std::vector<std::string> in_case;

  for (const auto& type : types) {
    in_case.push_back(std::to_string(static_cast<int>(type)));
  }

  const std::string query = base::StringPrintf(
      "SELECT promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, expires_at, claimed_at, "
      "claim_id, legacy "
      "FROM %s WHERE type IN (%s)",
      kTableName, base::JoinString(in_case, ",").c_str());

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePromotion::UpdateRecordsBlankPublicKey(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(1, "List of ids is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s as p SET public_keys = "
      "(SELECT PRINTF('[\"%%s\"]', public_key) FROM creds_batch as cb "
      "WHERE cb.trigger_id = p.promotion_id) WHERE p.promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace database
}  // namespace ledger
