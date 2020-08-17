/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <utility>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "brave/components/brave_rewards/browser/rewards_database.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {

void HandleBinding(
    sql::Statement* statement,
    const ledger::DBCommandBinding& binding) {
  if (!statement) {
    return;
  }

  switch (binding.value->which()) {
    case ledger::DBValue::Tag::STRING_VALUE: {
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    }
    case ledger::DBValue::Tag::INT_VALUE: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    }
    case ledger::DBValue::Tag::INT64_VALUE: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    }
    case ledger::DBValue::Tag::DOUBLE_VALUE: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    }
    case ledger::DBValue::Tag::BOOL_VALUE: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      return;
    }
    case ledger::DBValue::Tag::NULL_VALUE: {
      statement->BindNull(binding.index);
      return;
    }
    default: {
      NOTREACHED();
    }
  }
}

ledger::DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<ledger::DBCommand::RecordBindingType>& bindings) {
  auto record = ledger::DBRecord::New();
  int column = 0;

  if (!statement) {
    return record;
  }

  for (const auto& binding : bindings) {
    auto value = ledger::DBValue::New();
    switch (binding) {
      case ledger::DBCommand::RecordBindingType::STRING_TYPE: {
        value->set_string_value(statement->ColumnString(column));
        break;
      }
      case ledger::DBCommand::RecordBindingType::INT_TYPE: {
        value->set_int_value(statement->ColumnInt(column));
        break;
      }
      case ledger::DBCommand::RecordBindingType::INT64_TYPE: {
        value->set_int64_value(statement->ColumnInt64(column));
        break;
      }
      case ledger::DBCommand::RecordBindingType::DOUBLE_TYPE: {
        value->set_double_value(statement->ColumnDouble(column));
        break;
      }
      case ledger::DBCommand::RecordBindingType::BOOL_TYPE: {
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

RewardsDatabase::RewardsDatabase(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RewardsDatabase::~RewardsDatabase() = default;

void RewardsDatabase::RunTransaction(
    ledger::DBTransactionPtr transaction,
    ledger::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!command_response) {
    return;
  }

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status =
        ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status =
        ledger::DBCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  bool vacuum_requested = false;

  for (auto const& command : transaction->commands) {
    ledger::DBCommandResponse::Status status;

    VLOG(8) << "Query: " << command->command;

    switch (command->type) {
      case ledger::DBCommand::Type::INITIALIZE: {
        status = Initialize(
            transaction->version,
            transaction->compatible_version,
            command_response);
        break;
      }
      case ledger::DBCommand::Type::READ: {
        status = Read(command.get(), command_response);
        break;
      }
      case ledger::DBCommand::Type::EXECUTE: {
        status = Execute(command.get());
        break;
      }
      case ledger::DBCommand::Type::RUN: {
        status = Run(command.get());
        break;
      }
      case ledger::DBCommand::Type::MIGRATE: {
        status = Migrate(
            transaction->version,
            transaction->compatible_version);
        break;
      }
      case ledger::DBCommand::Type::VACUUM: {
        vacuum_requested = true;
        status = ledger::DBCommandResponse::Status::RESPONSE_OK;
        break;
      }
      default: {
        NOTREACHED();
      }
    }

    if (status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
      committer.Rollback();
      command_response->status = status;
      return;
    }
  }

  if (!committer.Commit()) {
    command_response->status =
        ledger::DBCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  if (vacuum_requested) {
    VLOG(8) << "Performing database vacuum";
    if (!db_.Execute("VACUUM")) {
      // If vacuum was not successful, log an error but do not
      // prevent forward progress.
      LOG(ERROR) << "Error executing VACUUM: " << db_.GetErrorMessage();
    }
  }
}

ledger::DBCommandResponse::Status RewardsDatabase::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    ledger::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!command_response) {
    return ledger::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  int table_version = 0;
  if (!initialized_) {
    bool table_exists = false;
    if (meta_table_.DoesTableExist(&db_)) {
      table_exists = true;
    }

    if (!meta_table_.Init(&db_, version, compatible_version)) {
      return ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
    }

    if (table_exists) {
      table_version = meta_table_.GetVersionNumber();
    }

    initialized_ = true;
    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        FROM_HERE,
        base::Bind(&RewardsDatabase::OnMemoryPressure,
        base::Unretained(this))));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  auto value = ledger::DBValue::New();
  value->set_int_value(table_version);
  auto result = ledger::DBCommandResult::New();
  result->set_value(std::move(value));
  command_response->result = std::move(result);

  return ledger::DBCommandResponse::Status::RESPONSE_OK;
}

ledger::DBCommandResponse::Status RewardsDatabase::Execute(
    ledger::DBCommand* command) {
  if (!initialized_) {
    return ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return ledger::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  bool result = db_.Execute(command->command.c_str());

  if (!result) {
    LOG(ERROR) << "DB Execute error: " << db_.GetErrorMessage();
    return ledger::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return ledger::DBCommandResponse::Status::RESPONSE_OK;
}

ledger::DBCommandResponse::Status RewardsDatabase::Run(
    ledger::DBCommand* command) {
  if (!initialized_) {
    return ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command) {
    return ledger::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  if (!statement.Run()) {
    LOG(ERROR) <<
    "DB Run error: " <<
    db_.GetErrorMessage() <<
    " (" << db_.GetErrorCode() <<
    ")";
    return ledger::DBCommandResponse::Status::COMMAND_ERROR;
  }

  return ledger::DBCommandResponse::Status::RESPONSE_OK;
}

ledger::DBCommandResponse::Status RewardsDatabase::Read(
    ledger::DBCommand* command,
    ledger::DBCommandResponse* command_response) {
  if (!initialized_) {
    return ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!command || !command_response) {
    return ledger::DBCommandResponse::Status::RESPONSE_ERROR;
  }

  sql::Statement statement(
      db_.GetUniqueStatement(command->command.c_str()));

  for (auto const& binding : command->bindings) {
    HandleBinding(&statement, *binding.get());
  }

  auto result = ledger::DBCommandResult::New();
  result->set_records(std::vector<ledger::DBRecordPtr>());
  command_response->result = std::move(result);
  while (statement.Step()) {
    command_response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));
  }

  return ledger::DBCommandResponse::Status::RESPONSE_OK;
}

ledger::DBCommandResponse::Status RewardsDatabase::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!initialized_) {
    return ledger::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  meta_table_.SetVersionNumber(version);
  meta_table_.SetCompatibleVersionNumber(compatible_version);

  return ledger::DBCommandResponse::Status::RESPONSE_OK;
}

void RewardsDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace brave_rewards
