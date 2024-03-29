/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/sql_store.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace brave_rewards::internal {

SQLReader::SQLReader(mojom::DBCommandResponsePtr response)
    : response_(std::move(response)) {}

SQLReader::~SQLReader() = default;

SQLReader::SQLReader(SQLReader&& other)
    : response_(std::move(other.response_)), row_(other.row_) {}

SQLReader& SQLReader::operator=(SQLReader&& other) {
  if (this != &other) {
    response_ = std::move(other.response_);
    row_ = other.row_;
  }
  return *this;
}

bool SQLReader::Step() {
  if (!response_) {
    return false;
  }
  size_t record_count = response_->records.size();
  if (!row_.has_value()) {
    row_ = 0;
    return record_count > 0;
  }
  if (row_.value() >= record_count) {
    return false;
  }
  return ++row_.value() < record_count;
}

bool SQLReader::Succeeded() const {
  return response_ &&
         response_->status == mojom::DBCommandResponse::Status::kSuccess;
}

bool SQLReader::ColumnBool(int col) const {
  return static_cast<bool>(ColumnInt64(col));
}

int SQLReader::ColumnInt(int col) const {
  return static_cast<int>(ColumnInt64(col));
}

int64_t SQLReader::ColumnInt64(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value) {
    return 0;
  }

  switch (db_value->which()) {
    case mojom::DBValue::Tag::kNullValue:
      return 0;
    case mojom::DBValue::Tag::kBoolValue:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::kIntValue:
      return static_cast<int64_t>(db_value->get_int_value());
    case mojom::DBValue::Tag::kInt64Value:
      return db_value->get_int64_value();
    case mojom::DBValue::Tag::kDoubleValue:
      return static_cast<int64_t>(db_value->get_double_value());
    case mojom::DBValue::Tag::kStringValue: {
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
    case mojom::DBValue::Tag::kNullValue:
      return 0;
    case mojom::DBValue::Tag::kBoolValue:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::kIntValue:
      return static_cast<double>(db_value->get_int_value());
    case mojom::DBValue::Tag::kInt64Value:
      return static_cast<double>(db_value->get_int64_value());
    case mojom::DBValue::Tag::kDoubleValue:
      return db_value->get_double_value();
    case mojom::DBValue::Tag::kStringValue: {
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
    case mojom::DBValue::Tag::kNullValue:
      return "";
    case mojom::DBValue::Tag::kBoolValue:
      return db_value->get_bool_value() ? "1" : "0";
    case mojom::DBValue::Tag::kIntValue:
      return base::NumberToString(db_value->get_int_value());
    case mojom::DBValue::Tag::kInt64Value:
      return base::NumberToString(db_value->get_int64_value());
    case mojom::DBValue::Tag::kDoubleValue:
      return base::NumberToString(db_value->get_double_value());
    case mojom::DBValue::Tag::kStringValue:
      return db_value->get_string_value();
  }
}

mojom::DBValue* SQLReader::GetDBValue(int col) const {
  if (!response_ || !row_.has_value() ||
      row_.value() >= response_->records.size()) {
    return nullptr;
  }

  auto& record = response_->records[row_.value()];
  int column_count = static_cast<int>(record->fields.size());
  if (col < 0 || col >= column_count) {
    return nullptr;
  }

  return record->fields[col].get();
}

SQLStore::SQLStore(RewardsEngine& engine) : RewardsEngineHelper(engine) {}

SQLStore::~SQLStore() = default;

mojom::DBValuePtr SQLStore::Bind(double value) {
  return mojom::DBValue::NewDoubleValue(value);
}

mojom::DBValuePtr SQLStore::Bind(int value) {
  return mojom::DBValue::NewIntValue(value);
}

mojom::DBValuePtr SQLStore::Bind(int64_t value) {
  return mojom::DBValue::NewInt64Value(value);
}

mojom::DBValuePtr SQLStore::Bind(bool value) {
  return mojom::DBValue::NewBoolValue(value);
}

mojom::DBValuePtr SQLStore::Bind(const char* value) {
  return mojom::DBValue::NewStringValue(value);
}

mojom::DBValuePtr SQLStore::Bind(const std::string& value) {
  return mojom::DBValue::NewStringValue(value);
}

mojom::DBValuePtr SQLStore::Bind(std::nullptr_t) {
  return mojom::DBValue::NewNullValue(0);
}

void SQLStore::Initialize(int version, SQLCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kInitialize;

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));
  transaction->version = version;
  transaction->compatible_version = version;

  RunTransaction(std::move(transaction), std::move(callback));
}

void SQLStore::Close(SQLCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kClose;
  Run(std::move(command), std::move(callback));
}

void SQLStore::Migrate(int version,
                       CommandList commands,
                       SQLCallback callback) {
  DCHECK_GT(version, 0);

  // Update the database version stored in the meta table.
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kMigrate;
  commands.push_back(std::move(command));

  auto transaction = mojom::DBTransaction::New();
  transaction->commands = std::move(commands);
  transaction->version = version;
  transaction->compatible_version = version;

  RunTransaction(std::move(transaction), std::move(callback));
}

void SQLStore::Vacuum(SQLCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kVacuum;
  Run(std::move(command), std::move(callback));
}

void SQLStore::Run(CommandList commands, SQLCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  transaction->commands = std::move(commands);
  RunTransaction(std::move(transaction), std::move(callback));
}

void SQLStore::Run(mojom::DBCommandPtr command, SQLCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));
  RunTransaction(std::move(transaction), std::move(callback));
}

void SQLStore::Execute(const std::string& sql, SQLCallback callback) {
  // TODO: Isn't execute just the same as run with zero bindings? Can we just
  // always use execute (or run) in that case? Why both?
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kExecute;
  command->command = sql;
  Run(std::move(command), std::move(callback));
}

void SQLStore::Query(const std::string& sql, SQLCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = sql;
  Run(std::move(command), std::move(callback));
}

std::string SQLStore::PlaceholderList(size_t count) {
  DCHECK_GT(count, size_t(0));
  return base::StrCat(
      {"(", base::JoinString(std::vector<std::string>(count, "?"), ", "), ")"});
}

std::string SQLStore::TimeString(const base::Time& time) {
  return base::TimeFormatAsIso8601(time);
}

std::string SQLStore::TimeString() {
  return TimeString(base::Time::Now());
}

base::Time SQLStore::ParseTime(const std::string& s) {
  base::Time time;
  bool valid = base::Time::FromString(s.c_str(), &time);
  return valid ? time : base::Time();
}

void SQLStore::RunTransaction(mojom::DBTransactionPtr transaction,
                              SQLCallback callback) {
  DCHECK(transaction);

  client().RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&SQLStore::OnTransactionResult, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void SQLStore::OnTransactionResult(SQLCallback callback,
                                   mojom::DBCommandResponsePtr response) {
  std::move(callback).Run(SQLReader(std::move(response)));
}

}  // namespace brave_rewards::internal
