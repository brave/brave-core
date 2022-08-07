/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/database.h"

#include <vector>

#include "base/bind.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "bat/ads/internal/base/database/database_bind_util.h"
#include "bat/ads/internal/base/database/database_record_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "third_party/sqlite/sqlite3.h"

namespace ads {

Database::Database(const base::FilePath& path) : db_path_(path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  db_.set_error_callback(
      base::BindRepeating(&Database::OnErrorCallback, base::Unretained(this)));
}

Database::~Database() = default;

void Database::RunTransaction(mojom::DBTransactionPtr transaction,
                              mojom::DBCommandResponse* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(transaction);
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
    DCHECK(mojom::IsKnownEnumValue(command->type));

    mojom::DBCommandResponse::Status status;

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

  command_response->result = mojom::DBCommandResult::NewValue(
      mojom::DBValue::NewIntValue(table_version));

  return mojom::DBCommandResponse::Status::RESPONSE_OK;
}

mojom::DBCommandResponse::Status Database::Execute(mojom::DBCommand* command) {
  DCHECK(command);

  if (!is_initialized_) {
    return mojom::DBCommandResponse::Status::INITIALIZATION_ERROR;
  }

  if (!db_.Execute(command->command.c_str())) {
    VLOG(0) << "Database store error: " << db_.GetErrorMessage();
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
    VLOG(0) << "Database store error: Invalid statement";
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    database::Bind(&statement, *binding.get());
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
    VLOG(0) << "Database store error: Invalid statement";
    return mojom::DBCommandResponse::Status::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    database::Bind(&statement, *binding.get());
  }

  command_response->result =
      mojom::DBCommandResult::NewRecords(std::vector<mojom::DBRecordPtr>());

  while (statement.Step()) {
    command_response->result->get_records().push_back(
        database::CreateRecord(&statement, command->record_bindings));
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
  VLOG(0) << "Database error: " << db_.GetDiagnosticInfo(error, statement);
}

void Database::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace ads
