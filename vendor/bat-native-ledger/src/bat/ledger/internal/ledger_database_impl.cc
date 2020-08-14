/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_database_impl.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "bat/ledger/internal/logging/logging.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace ledger {

namespace {

void HandleBinding(
    sql::Statement* statement,
    const DBCommandBinding& binding) {
  if (!statement) {
    return;
  }

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
    default: {
      NOTREACHED();
    }
  }
}

DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<DBCommand::RecordBindingType>& bindings) {
  auto record = DBRecord::New();
  int column = 0;

  if (!statement) {
    return record;
  }

  for (const auto& binding : bindings) {
    auto value = DBValue::New();
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
      default: {
        NOTREACHED();
      }
    }
    record->fields.push_back(std::move(value));
    column++;
  }

  return record;
}

}  // namespace

LedgerDatabaseImpl::LedgerDatabaseImpl(const base::FilePath& path) :
    db_path_(path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

LedgerDatabaseImpl::~LedgerDatabaseImpl() = default;

void LedgerDatabaseImpl::RunTransaction(
    DBTransactionPtr transaction,
    DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!command_response) {
    return;
  }

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status = DBCommandResponse::Status::INITIALIZATION_ERROR;
    return;
  }

  // Close command must always be sent as single command in transaction
  if (transaction->commands.size() == 1 &&
      transaction->commands[0]->type == DBCommand::Type::CLOSE) {
    db_.Close();
    initialized_ = false;
    command_response->status = DBCommandResponse::Status::RESPONSE_OK;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status = DBCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  bool vacuum_requested = false;

  for (auto const& command : transaction->commands) {
    DBCommandResponse::Status status;

    BLOG(8, "Query: " << command->command);

    switch (command->type) {
      case DBCommand::Type::INITIALIZE: {
        status = Initialize(
            transaction->version,
            transaction->compatible_version,
            command_response);
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
        status = Migrate(
            transaction->version,
            transaction->compatible_version);
        break;
      }
      case DBCommand::Type::VACUUM: {
        vacuum_requested = true;
        status = DBCommandResponse::Status::RESPONSE_OK;
        break;
      }
      case DBCommand::Type::CLOSE: {
        NOTREACHED();
        status = DBCommandResponse::Status::COMMAND_ERROR;
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
    return;
  }

  if (vacuum_requested) {
    BLOG(8, "Performing database vacuum");
    if (!db_.Execute("VACUUM")) {
      // If vacuum was not successful, log an error but do not
      // prevent forward progress.
      BLOG(0, "Error executing VACUUM: " << db_.GetErrorMessage());
    }
  }
}

DBCommandResponse::Status LedgerDatabaseImpl::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!command_response) {
    return DBCommandResponse::Status::RESPONSE_ERROR;
  }

  int table_version = 0;
  if (!initialized_) {
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

    initialized_ = true;
    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        FROM_HERE,
        base::Bind(&LedgerDatabaseImpl::OnMemoryPressure,
        base::Unretained(this))));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  auto value = DBValue::New();
  value->set_int_value(table_version);
  auto result = DBCommandResult::New();
  result->set_value(std::move(value));
  command_response->result = std::move(result);

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status LedgerDatabaseImpl::Execute(DBCommand* command) {
  if (!initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return DBCommandResponse::Status::RESPONSE_ERROR;
  }

  bool result = db_.Execute(command->command.c_str());

  if (!result) {
    BLOG(0, "DB Execute error: " << db_.GetErrorMessage());
    return DBCommandResponse::Status::COMMAND_ERROR;
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status LedgerDatabaseImpl::Run(DBCommand* command) {
  if (!initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  if (!statement.Run()) {
    BLOG(0, "DB Run error: " << db_.GetErrorMessage() <<
        " (" << db_.GetErrorCode() << ")");
    return DBCommandResponse::Status::COMMAND_ERROR;
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status LedgerDatabaseImpl::Read(
    DBCommand* command,
    DBCommandResponse* command_response) {
  if (!initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command || !command_response) {
    return DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(
      db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  auto result = DBCommandResult::New();
  result->set_records(std::vector<DBRecordPtr>());
  command_response->result = std::move(result);
  while (statement.Step()) {
    command_response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));
  }

  return DBCommandResponse::Status::RESPONSE_OK;
}

DBCommandResponse::Status LedgerDatabaseImpl::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!initialized_) {
    return DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  meta_table_.SetVersionNumber(version);
  meta_table_.SetCompatibleVersionNumber(compatible_version);

  return DBCommandResponse::Status::RESPONSE_OK;
}

void LedgerDatabaseImpl::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace ledger
