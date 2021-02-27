/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/sql_store.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time_to_iso8601.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

namespace {

void NormalizeResponse(mojom::DBCommandResponse* response) {
  DCHECK(response);
  if (!response->result) {
    // Ensure that we always return an array of records.
    auto result = mojom::DBCommandResult::New();
    result->set_records({});
    response->result = std::move(result);
  } else if (response->result->is_value()) {
    // Ensure that values returned from the client in the "value" field are
    // returned as a record.
    auto record = mojom::DBRecord::New();
    record->fields.push_back(response->result->get_value()->Clone());

    std::vector<mojom::DBRecordPtr> records;
    records.push_back(std::move(record));

    auto result = mojom::DBCommandResult::New();
    result->set_records(std::move(records));
    response->result = std::move(result);
  }
}

}  // namespace

SQLReader::SQLReader(mojom::DBCommandResponsePtr response)
    : response_(std::move(response)) {
  DCHECK(response_);
}

SQLReader::~SQLReader() = default;

SQLReader::SQLReader(SQLReader&& other)
    : response_(std::move(other.response_)), row_(other.row_) {}

SQLReader& SQLReader::operator=(SQLReader&& other) {
  response_ = std::move(other.response_);
  row_ = other.row_;
  return *this;
}

bool SQLReader::Step() {
  if (!response_->result || !response_->result->is_records()) {
    return false;
  }

  int record_count = static_cast<int>(response_->result->get_records().size());
  if (row_ >= record_count) {
    return false;
  }

  return ++row_ < record_count;
}

bool SQLReader::Succeeded() const {
  return response_->status == mojom::DBCommandResponse::Status::RESPONSE_OK;
}

bool SQLReader::ColumnBool(int col) const {
  return static_cast<bool>(ColumnInt64(col));
}

int64_t SQLReader::ColumnInt64(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value) {
    return 0;
  }

  switch (db_value->which()) {
    case mojom::DBValue::Tag::NULL_VALUE:
      return 0;
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::INT_VALUE:
      return static_cast<int64_t>(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return db_value->get_int64_value();
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return static_cast<int64_t>(db_value->get_double_value());
    case mojom::DBValue::Tag::STRING_VALUE: {
      int64_t value = 0;
      base::StringToInt64(db_value->get_string_value(), &value);
      return value;
    }
  }
}

double SQLReader::ColumnDouble(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value) {
    return 0;
  }

  switch (db_value->which()) {
    case mojom::DBValue::Tag::NULL_VALUE:
      return 0;
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::INT_VALUE:
      return static_cast<double>(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return static_cast<double>(db_value->get_int64_value());
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return db_value->get_double_value();
    case mojom::DBValue::Tag::STRING_VALUE: {
      double value = 0;
      base::StringToDouble(db_value->get_string_value(), &value);
      return value;
    }
  }
}

std::string SQLReader::ColumnString(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value) {
    return "";
  }

  switch (db_value->which()) {
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? "1" : "0";
    case mojom::DBValue::Tag::INT_VALUE:
      return base::NumberToString(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return base::NumberToString(db_value->get_int64_value());
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return base::NumberToString(db_value->get_double_value());
    case mojom::DBValue::Tag::STRING_VALUE:
      return db_value->get_string_value();
    case mojom::DBValue::Tag::NULL_VALUE:
      return "";
  }
}

mojom::DBValue* SQLReader::GetDBValue(int col) const {
  if (!response_->result || !response_->result->is_records()) {
    return nullptr;
  }

  auto& records = response_->result->get_records();
  if (row_ >= static_cast<int>(records.size())) {
    return nullptr;
  }

  auto& record = records[row_];
  int column_count = static_cast<int>(record->fields.size());
  if (col < 0 || col >= column_count) {
    return nullptr;
  }

  return record->fields[col].get();
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, double value) {
  db_value->set_double_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, int32_t value) {
  db_value->set_int64_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, int64_t value) {
  db_value->set_int64_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, bool value) {
  db_value->set_bool_value(value ? 1 : 0);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, const char* value) {
  db_value->set_string_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, const std::string& value) {
  db_value->set_string_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, std::nullptr_t) {
  db_value->set_null_value(0);
}

Future<SQLReader> SQLStore::Open() {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));
  transaction->version = 1;
  transaction->compatible_version = 1;

  return RunTransactionImpl(std::move(transaction));
}

Future<SQLReader> SQLStore::Close() {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::CLOSE;
  return RunTransaction(std::move(command));
}

Future<SQLReader> SQLStore::ExecuteMigration(int version,
                                             const std::string& sql) {
  std::vector<mojom::DBCommandPtr> commands;
  if (!sql.empty()) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::EXECUTE;
    command->command = sql;
    commands.push_back(std::move(command));
  }

  return RunMigration(version, std::move(commands));
}

Future<SQLReader> SQLStore::RunMigration(int version, CommandList&& commands) {
  DCHECK_GT(version, 0);

  // Update the database version stored in the meta table.
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::MIGRATE;
  commands.push_back(std::move(command));

  auto transaction = mojom::DBTransaction::New();
  transaction->commands = std::move(commands);
  transaction->version = version;
  transaction->compatible_version = version;

  return RunTransactionImpl(std::move(transaction));
}

Future<SQLReader> SQLStore::Vacuum() {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::VACUUM;
  return RunTransaction(std::move(command));
}

Future<SQLReader> SQLStore::RunTransaction(CommandList&& commands) {
  auto transaction = mojom::DBTransaction::New();
  transaction->commands = std::move(commands);
  return RunTransactionImpl(std::move(transaction));
}

Future<SQLReader> SQLStore::RunTransaction(mojom::DBCommandPtr command) {
  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));
  return RunTransactionImpl(std::move(transaction));
}

Future<SQLReader> SQLStore::Execute(const std::string& sql) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = sql;
  return RunTransaction(std::move(command));
}

std::string SQLStore::PlaceholderList(size_t count) {
  DCHECK_GT(count, size_t(0));
  return base::StrCat(
      {"(", base::JoinString(std::vector<std::string>(count, "?"), ", "), ")"});
}

std::string SQLStore::TimeString(const base::Time& time) {
  return base::TimeToISO8601(time);
}

std::string SQLStore::TimeString() {
  return TimeString(base::Time::Now());
}

base::Time SQLStore::ParseTime(const std::string& s) {
  base::Time time;
  bool valid = base::Time::FromString(s.c_str(), &time);
  return valid ? time : base::Time();
}

Future<SQLReader> SQLStore::RunTransactionImpl(
    mojom::DBTransactionPtr transaction) {
  DCHECK(transaction);

  auto promise = std::make_shared<Promise<SQLReader>>();
  auto future = promise->GetFuture();

  context().GetLedgerClient()->RunDBTransaction(
      std::move(transaction), [promise](mojom::DBCommandResponsePtr response) {
        NormalizeResponse(response.get());
        promise->SetValue(SQLReader(std::move(response)));
      });

  return future;
}

}  // namespace ledger
