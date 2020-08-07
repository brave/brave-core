/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_event_log.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "event_log";

}  // namespace

DatabaseEventLog::DatabaseEventLog(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseEventLog::~DatabaseEventLog() = default;

void DatabaseEventLog::Insert(
    const std::string& key,
    const std::string& value) {
  if (key.empty() || value.empty()) {
    BLOG_IF(1, key.empty(), "Key is empty");
    BLOG_IF(1, value.empty(), "Value is empty");
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT INTO %s (event_log_id, key, value, created_at) "
      "VALUES (?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, base::GenerateGUID());
  BindString(command.get(), 1, key);
  BindString(command.get(), 2, value);
  BindInt64(command.get(), 3, braveledger_time_util::GetCurrentTimeStamp());

  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      [](ledger::DBCommandResponsePtr response){});
}

void DatabaseEventLog::InsertRecords(
    const std::map<std::string, std::string>& records,
    ledger::ResultCallback callback) {
  if (records.empty()) {
    BLOG(0, "No records");
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  auto transaction = ledger::DBTransaction::New();
  auto time = braveledger_time_util::GetCurrentTimeStamp();
  const std::string base_query = base::StringPrintf(
      "INSERT INTO %s (event_log_id, key, value, created_at) VALUES ",
      kTableName);

  std::string query = base_query;
  for (const auto& record : records) {
    query += base::StringPrintf(
        R"(("%s","%s","%s",%u),)",
        base::GenerateGUID().c_str(),
        record.first.c_str(),
        record.second.c_str(),
        static_cast<uint32_t>(time));
  }

  query.pop_back();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseEventLog::GetLastRecords(ledger::GetEventLogsCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT event_log_id, key, value, created_at "
    "FROM %s ORDER BY created_at DESC LIMIT 2000",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseEventLog::OnGetAllRecords,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseEventLog::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetEventLogsCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::EventLogs list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::EventLog::New();
    auto* record_pointer = record.get();

    info->event_log_id = GetStringColumn(record_pointer, 0);
    info->key = GetStringColumn(record_pointer, 1);
    info->value = GetStringColumn(record_pointer, 2);
    info->created_at = GetInt64Column(record_pointer, 3);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace braveledger_database
