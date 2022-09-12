/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/public/ledger_database.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace ledger {

namespace {

void HandleBinding(sql::Statement* statement,
                   const mojom::DBCommandBinding& binding) {
  if (!statement) {
    return;
  }

  switch (binding.value->which()) {
    case mojom::DBValue::Tag::kStringValue: {
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    }
    case mojom::DBValue::Tag::kIntValue: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    }
    case mojom::DBValue::Tag::kInt64Value: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    }
    case mojom::DBValue::Tag::kDoubleValue: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    }
    case mojom::DBValue::Tag::kBoolValue: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      return;
    }
    case mojom::DBValue::Tag::kNullValue: {
      statement->BindNull(binding.index);
      return;
    }
    default: {
      NOTREACHED();
    }
  }
}

mojom::DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<mojom::DBCommand::RecordBindingType>& bindings) {
  auto record = mojom::DBRecord::New();
  int column = 0;

  if (!statement) {
    return record;
  }

  for (const auto& binding : bindings) {
    mojom::DBValuePtr value;
    switch (binding) {
      case mojom::DBCommand::RecordBindingType::STRING_TYPE: {
        value = mojom::DBValue::NewStringValue(statement->ColumnString(column));
        break;
      }
      case mojom::DBCommand::RecordBindingType::INT_TYPE: {
        value = mojom::DBValue::NewIntValue(statement->ColumnInt(column));
        break;
      }
      case mojom::DBCommand::RecordBindingType::INT64_TYPE: {
        value = mojom::DBValue::NewInt64Value(statement->ColumnInt64(column));
        break;
      }
      case mojom::DBCommand::RecordBindingType::DOUBLE_TYPE: {
        value = mojom::DBValue::NewDoubleValue(statement->ColumnDouble(column));
        break;
      }
      case mojom::DBCommand::RecordBindingType::BOOL_TYPE: {
        value = mojom::DBValue::NewBoolValue(statement->ColumnBool(column));
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

LedgerDatabase::LedgerDatabase(const base::FilePath& path) : db_path_(path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

LedgerDatabase::~LedgerDatabase() = default;

mojom::DBCommandResponsePtr LedgerDatabase::RunTransaction(
    mojom::DBTransactionPtr transaction) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto command_response = mojom::DBCommandResponse::New();

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status =
        mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
    return command_response;
  }

  // Close command must always be sent as single command in transaction
  if (transaction->commands.size() == 1 &&
      transaction->commands[0]->type == mojom::DBCommand::Type::CLOSE) {
    db_.Close();
    initialized_ = false;
    command_response->status = mojom::DBCommandResponse::Status::RESPONSE_OK;
    return command_response;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status =
        mojom::DBCommandResponse::Status::TRANSACTION_ERROR;
    return command_response;
  }

  bool vacuum_requested = false;

  for (auto const& command : transaction->commands) {
    mojom::DBCommandResponse::Status status;

    switch (command->type) {
      case mojom::DBCommand::Type::INITIALIZE: {
        status =
            Initialize(transaction->version, transaction->compatible_version,
                       command_response.get());
        break;
      }
      case mojom::DBCommand::Type::READ: {
        status = Read(command.get(), command_response.get());
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
      case mojom::DBCommand::Type::VACUUM: {
        vacuum_requested = true;
        status = mojom::DBCommandResponse::Status::RESPONSE_OK;
        break;
      }
      case mojom::DBCommand::Type::CLOSE: {
        NOTREACHED();
        status = mojom::DBCommandResponse::Status::COMMAND_ERROR;
        break;
      }
    }

    if (status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
      committer.Rollback();
      command_response->status = status;
      return command_response;
    }
  }

  if (!committer.Commit()) {
    command_response->status =
        mojom::DBCommandResponse::Status::TRANSACTION_ERROR;
    return command_response;
  }

  if (vacuum_requested) {
    if (!db_.Execute("VACUUM")) {
      // If vacuum was not successful, log an error but do not
      // prevent forward progress.
      LOG(ERROR) << "Error executing VACUUM: " << db_.GetErrorMessage();
    }
  }

  return command_response;
}

mojom::DBCommandResponse::Status LedgerDatabase::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    mojom::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!command_response) {
    return mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  int table_version = 0;
  if (!initialized_) {
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

    initialized_ = true;
    memory_pressure_listener_ = std::make_unique<base::MemoryPressureListener>(
        FROM_HERE, base::BindRepeating(&LedgerDatabase::OnMemoryPressure,
                                       base::Unretained(this)));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  auto result = mojom::DBCommandResult::NewValue(
      mojom::DBValue::NewIntValue(table_version));
  command_response->result = std::move(result);

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status LedgerDatabase::Execute(
    mojom::DBCommand* command) {
  if (!initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  bool result = db_.Execute(command->command.c_str());

  if (!result) {
    LOG(ERROR) << "DB Execute error: " << db_.GetErrorMessage();
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status LedgerDatabase::Run(
    mojom::DBCommand* command) {
  if (!initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  if (!statement.Run()) {
    LOG(ERROR) << "DB Run error: " << db_.GetErrorMessage() << " ("
               << db_.GetErrorCode() << ")";
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status LedgerDatabase::Read(
    mojom::DBCommand* command,
    mojom::DBCommandResponse* command_response) {
  if (!initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command || !command_response) {
    return mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  command_response->result =
      mojom::DBCommandResult::NewRecords(std::vector<mojom::DBRecordPtr>());
  while (statement.Step()) {
    command_response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));
  }

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status LedgerDatabase::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  meta_table_.SetVersionNumber(version);
  meta_table_.SetCompatibleVersionNumber(compatible_version);

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

void LedgerDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace ledger
