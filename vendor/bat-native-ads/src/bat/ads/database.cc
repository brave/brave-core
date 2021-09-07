/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/database.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/check.h"
#include "base/files/file_util.h"
#include "base/notreached.h"
#include "base/sequence_checker.h"
#include "bat/ads/internal/logging.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "third_party/sqlite/sqlite3.h"

namespace ads {

namespace {

void Bind(sql::Statement* statement, const mojom::DBCommandBinding& binding) {
  DCHECK(statement);

  switch (binding.value->which()) {
    case mojom::DBValue::Tag::STRING_VALUE: {
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    }

    case mojom::DBValue::Tag::INT_VALUE: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    }

    case mojom::DBValue::Tag::INT64_VALUE: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    }

    case mojom::DBValue::Tag::DOUBLE_VALUE: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    }

    case mojom::DBValue::Tag::BOOL_VALUE: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      return;
    }

    case mojom::DBValue::Tag::NULL_VALUE: {
      statement->BindNull(binding.index);
      return;
    }
  }
}

mojom::DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<mojom::DBCommand::RecordBindingType>& bindings) {
  DCHECK(statement);

  mojom::DBRecordPtr record = mojom::DBRecord::New();

  int column = 0;

  for (const auto& binding : bindings) {
    mojom::DBValuePtr value = mojom::DBValue::New();
    switch (binding) {
      case mojom::DBCommand::RecordBindingType::STRING_TYPE: {
        value->set_string_value(statement->ColumnString(column));
        break;
      }

      case mojom::DBCommand::RecordBindingType::INT_TYPE: {
        value->set_int_value(statement->ColumnInt(column));
        break;
      }

      case mojom::DBCommand::RecordBindingType::INT64_TYPE: {
        value->set_int64_value(statement->ColumnInt64(column));
        break;
      }

      case mojom::DBCommand::RecordBindingType::DOUBLE_TYPE: {
        value->set_double_value(statement->ColumnDouble(column));
        break;
      }

      case mojom::DBCommand::RecordBindingType::BOOL_TYPE: {
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

Database::Database(const base::FilePath& path) : db_path_(path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  db_.set_error_callback(
      base::BindRepeating(&Database::OnErrorCallback, base::Unretained(this)));
}

Database::~Database() = default;

void Database::RunTransaction(mojom::DBTransactionPtr transaction,
                              mojom::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(command_response);

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status =
        mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status =
        mojom::DBCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  for (const auto& command : transaction->commands) {
    mojom::DBCommandResponse::Status status;

    BLOG(8, "Database query: " << command->command);

    switch (command->type) {
      case mojom::DBCommand::Type::INITIALIZE: {
        status = Initialize(transaction->version,
                            transaction->compatible_version, command_response);
        break;
      }

      case mojom::DBCommand::Type::READ: {
        status = Read(command.get(), command_response);
        break;
      }

      case mojom::DBCommand::Type::EXECUTE: {
        status = Execute(command.get());
        break;
      }

      case mojom::DBCommand::Type::RUN: {
        status = Run(command.get());
        break;
      }

      case mojom::DBCommand::Type::MIGRATE: {
        status = Migrate(transaction->version, transaction->compatible_version);
        break;
      }
    }

    if (status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
      committer.Rollback();
      command_response->status = status;
      return;
    }
  }

  if (!committer.Commit()) {
    command_response->status =
        mojom::DBCommandResponse::Status::TRANSACTION_ERROR;
  }
}

mojom::DBCommandResponse::Status Database::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    mojom::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(command_response);

  int table_version = 0;

  if (!is_initialized_) {
    bool table_exists = false;
    if (meta_table_.DoesTableExist(&db_)) {
      table_exists = true;
    }

    if (!meta_table_.Init(&db_, version, compatible_version)) {
      return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
    }

    if (table_exists) {
      table_version = meta_table_.GetVersionNumber();
    }

    is_initialized_ = true;
    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        FROM_HERE, base::BindRepeating(&Database::OnMemoryPressure,
                                       base::Unretained(this))));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  mojom::DBValuePtr value = mojom::DBValue::New();
  value->set_int_value(table_version);

  mojom::DBCommandResultPtr result = mojom::DBCommandResult::New();
  result->set_value(std::move(value));

  command_response->result = std::move(result);

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status Database::Execute(mojom::DBCommand* command) {
  DCHECK(command);

  if (!is_initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  const int error = db_.ExecuteAndReturnErrorCode(command->command.c_str());
  if (error != SQLITE_OK) {
    BLOG(0, "Database error: " << db_.GetErrorMessage());
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status Database::Run(mojom::DBCommand* command) {
  DCHECK(command);

  if (!is_initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(command->command.c_str()));
  if (!statement.is_valid()) {
    NOTREACHED();
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    Bind(&statement, *binding.get());
  }

  if (!statement.Run()) {
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status Database::Read(
    mojom::DBCommand* command,
    mojom::DBCommandResponse* command_response) {
  DCHECK(command);
  DCHECK(command_response);

  if (!is_initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(command->command.c_str()));
  if (!statement.is_valid()) {
    NOTREACHED();
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    Bind(&statement, *binding.get());
  }

  mojom::DBCommandResultPtr result = mojom::DBCommandResult::New();
  result->set_records(std::vector<mojom::DBRecordPtr>());

  command_response->result = std::move(result);

  while (statement.Step()) {
    command_response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status Database::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!is_initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  meta_table_.SetVersionNumber(version);
  meta_table_.SetCompatibleVersionNumber(compatible_version);

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

void Database::OnErrorCallback(const int error, sql::Statement* statement) {
  BLOG(0, "Database error: " << db_.GetDiagnosticInfo(error, statement));
}

void Database::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace ads
