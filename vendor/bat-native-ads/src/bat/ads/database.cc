/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/database.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

void Bind(
    sql::Statement* statement,
    const DBCommandBinding& binding) {
  DCHECK(statement);

  switch (binding.value->which()) {
    case DBValue::Tag::STRING_VALUE: {
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    }

    case DBValue::Tag::INT_VALUE: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    }

    case DBValue::Tag::INT64_VALUE: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    }

    case DBValue::Tag::DOUBLE_VALUE: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    }

    case DBValue::Tag::BOOL_VALUE: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      return;
    }

    case DBValue::Tag::NULL_VALUE: {
      statement->BindNull(binding.index);
      return;
    }
  }
}

DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<DBCommand::RecordBindingType>& bindings) {
  DCHECK(statement);

  DBRecordPtr record = DBRecord::New();

  int column = 0;

  for (const auto& binding : bindings) {
    DBValuePtr value = DBValue::New();
    switch (binding) {
      case DBCommand::RecordBindingType::STRING_TYPE: {
        value->set_string_value(statement->ColumnString(column));
        break;
      }

      case DBCommand::RecordBindingType::INT_TYPE: {
        value->set_int_value(statement->ColumnInt(column));
        break;
      }

      case DBCommand::RecordBindingType::INT64_TYPE: {
        value->set_int64_value(statement->ColumnInt64(column));
        break;
      }

      case DBCommand::RecordBindingType::DOUBLE_TYPE: {
        value->set_double_value(statement->ColumnDouble(column));
        break;
      }

      case DBCommand::RecordBindingType::BOOL_TYPE: {
        value->set_bool_value(statement->ColumnBool(column));
        break;
      }
    }

    record->fields.push_back(std::move(value));
    column++;
  }

  return record;
}

}  // namespace

Database::Database(
    const base::FilePath& path)
    : db_path_(path),
      is_initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  db_.set_error_callback(base::BindRepeating(&Database::OnErrorCallback,
      base::Unretained(this)));
}

Database::~Database() = default;

void Database::RunTransaction(
    DBTransactionPtr transaction,
    DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(command_response);

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status = DBCommandResponse::Status::INITIALIZATION_ERROR;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status = DBCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  for (const auto& command : transaction->commands) {
    DBCommandResponse::Status status;

    BLOG(8, "Database query: " << command->command);

    switch (command->type) {
      case DBCommand::Type::INITIALIZE: {
        status = Initialize(transaction->version,
            transaction->compatible_version, command_response);
        break;
      }

      case DBCommand::Type::READ: {
        status = Read(command.get(), command_response);
        break;
      }

      case DBCommand::Type::EXECUTE: {
        status = Execute(command.get());
        break;
      }

      case DBCommand::Type::RUN: {
        status = Run(command.get());
        break;
      }

      case DBCommand::Type::MIGRATE: {
        status = Migrate(transaction->version, transaction->compatible_version);
        break;
      }
    }

    if (status != DBCommandResponse::Status::RESPONSE_OK) {
      committer.Rollback();
      command_response->status = status;
      return;
    }
  }

  if (!committer.Commit()) {
    command_response->status = DBCommandResponse::Status::TRANSACTION_ERROR;
  }
}

DBCommandResponse::Status Database::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(command_response);

  int table_version = 0;

  if (!is_initialized_) {
    bool table_exists = false;
    if (meta_table_.DoesTableExist(&db_)) {
      table_exists = true;
    }

    if (!meta_table_.Init(&db_, version, compatible_version)) {
      return DBCommandResponse::Status::INITIALIZATION_ERROR;
    }

    if (table_exists) {
      table_version = meta_table_.GetVersionNumber();
    }

    is_initialized_ = true;
    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        base::Bind(&Database::OnMemoryPressure, base::Unretained(this))));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  DBValuePtr value = DBValue::New();
  value->set_int_value(table_version);

  DBCommandResultPtr result = DBCommandResult::New();
  result->set_value(std::move(value));

  command_response->result = std::move(result);

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status Database::Execute(
    DBCommand* command) {
  DCHECK(command);

  if (!is_initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  bool result = db_.Execute(command->command.c_str());

  if (!result) {
    BLOG(0, "Database error: " << db_.GetErrorMessage());

    return DBCommandResponse::Status::COMMAND_ERROR;
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status Database::Run(
    DBCommand* command) {
  DCHECK(command);

  if (!is_initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (const auto& binding : command->bindings) {
    Bind(&statement, *binding.get());
  }

  if (!statement.Run()) {
    BLOG(0, "Database error: " << db_.GetErrorMessage() << " ("
        << db_.GetErrorCode() << ")");

    return DBCommandResponse::Status::COMMAND_ERROR;
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status Database::Read(
    DBCommand* command,
    DBCommandResponse* command_response) {
  DCHECK(command);
  DCHECK(command_response);

  if (!is_initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (const auto& binding : command->bindings) {
    Bind(&statement, *binding.get());
  }

  DBCommandResultPtr result = DBCommandResult::New();
  result->set_records(std::vector<DBRecordPtr>());

  command_response->result = std::move(result);

  while (statement.Step()) {
    command_response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status Database::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!is_initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  meta_table_.SetVersionNumber(version);
  meta_table_.SetCompatibleVersionNumber(compatible_version);

  return DBCommandResponse::Status::RESPONSE_OK;
}

void Database::OnErrorCallback(
    const int error,
    sql::Statement* statement) {
  BLOG(1, "Database error: " << db_.GetDiagnosticInfo(error, statement));
}

void Database::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace ads
