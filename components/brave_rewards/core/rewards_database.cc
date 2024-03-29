/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_database.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards::internal {

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
  }
}

mojom::DBRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<mojom::DBCommand::RecordBindingType>& bindings) {
  auto record = mojom::DBRecord::New();
  if (!statement) {
    return record;
  }

  // NOTE: The `record_bindings` member of DBCommand is deprecated but still
  // supported for existing commands. It was previously used to specify how to
  // pull data out of the SQL results.
  if (bindings.size() > 0) {
    int column = 0;
    for (const auto& binding : bindings) {
      mojom::DBValuePtr value;
      switch (binding) {
        case mojom::DBCommand::RecordBindingType::kString: {
          value =
              mojom::DBValue::NewStringValue(statement->ColumnString(column));
          break;
        }
        case mojom::DBCommand::RecordBindingType::kInt: {
          value = mojom::DBValue::NewIntValue(statement->ColumnInt(column));
          break;
        }
        case mojom::DBCommand::RecordBindingType::kInt64: {
          value = mojom::DBValue::NewInt64Value(statement->ColumnInt64(column));
          break;
        }
        case mojom::DBCommand::RecordBindingType::kDouble: {
          value =
              mojom::DBValue::NewDoubleValue(statement->ColumnDouble(column));
          break;
        }
        case mojom::DBCommand::RecordBindingType::kBool: {
          value = mojom::DBValue::NewBoolValue(statement->ColumnBool(column));
          break;
        }
      }
      record->fields.push_back(std::move(value));
      column++;
    }
    return record;
  }

  for (int column = 0; column < statement->ColumnCount(); ++column) {
    mojom::DBValuePtr value;
    switch (statement->GetColumnType(column)) {
      case sql::ColumnType::kInteger:
        value = mojom::DBValue::NewInt64Value(statement->ColumnInt64(column));
        break;
      case sql::ColumnType::kFloat:
        value = mojom::DBValue::NewDoubleValue(statement->ColumnDouble(column));
        break;
      case sql::ColumnType::kText:
        value = mojom::DBValue::NewStringValue(statement->ColumnString(column));
        break;
      case sql::ColumnType::kBlob: {
        std::string blob_string;
        statement->ColumnBlobAsString(column, &blob_string);
        value = mojom::DBValue::NewStringValue(blob_string);
        break;
      }
      case sql::ColumnType::kNull:
        value = mojom::DBValue::NewNullValue(0);
        break;
    }
    record->fields.push_back(std::move(value));
  }

  return record;
}

mojom::DBCommandResponsePtr CreateResponse(
    mojom::DBCommandResponse::Status status) {
  auto response = mojom::DBCommandResponse::New();
  response->status = status;
  return response;
}

}  // namespace

RewardsDatabase::RewardsDatabase(const base::FilePath& path) : db_path_(path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RewardsDatabase::~RewardsDatabase() = default;

mojom::DBCommandResponsePtr RewardsDatabase::RunTransaction(
    mojom::DBTransactionPtr transaction) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Return success if there are no commands to execute.
  if (transaction->commands.empty()) {
    return CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  }

  // The close command must always be sent as single command. Otherwise an error
  // will be generated below.
  if (transaction->commands.size() == 1 &&
      transaction->commands[0]->type == mojom::DBCommand::Type::kClose) {
    if (db_.is_open()) {
      db_.Close();
      meta_table_.Reset();
      initialized_ = false;
    }
    return CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  }

  // Attempt to open the database if it is not already open.
  if (!db_.is_open() && !db_.Open(db_path_)) {
    return CreateResponse(
        mojom::DBCommandResponse::Status::kInitializationError);
  }

  // Start a transaction.
  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    return CreateResponse(mojom::DBCommandResponse::Status::kTransactionError);
  }

  mojom::DBCommandResponsePtr response;
  bool vacuum_requested = false;

  // Attempt to execute each command in the transaction.
  for (auto& command : transaction->commands) {
    switch (command->type) {
      case mojom::DBCommand::Type::kInitialize: {
        response =
            Initialize(transaction->version, transaction->compatible_version);
        break;
      }
      case mojom::DBCommand::Type::kRead: {
        response = Read(*command);
        break;
      }
      case mojom::DBCommand::Type::kExecute: {
        response = Execute(*command);
        break;
      }
      case mojom::DBCommand::Type::kRun: {
        response = Run(*command);
        break;
      }
      case mojom::DBCommand::Type::kMigrate: {
        response =
            Migrate(transaction->version, transaction->compatible_version);
        break;
      }
      case mojom::DBCommand::Type::kVacuum: {
        vacuum_requested = true;
        response = CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
        break;
      }
      case mojom::DBCommand::Type::kClose: {
        // The close command cannot appear in a transaction with other commands.
        response =
            CreateResponse(mojom::DBCommandResponse::Status::kCommandError);
        break;
      }
    }

    // If an error was encountered, then rollback the transaction and return the
    // error to the caller.
    if (response->status != mojom::DBCommandResponse::Status::kSuccess) {
      committer.Rollback();
      return response;
    }
  }

  DCHECK(response);

  // Attempt to commit the transaction.
  if (!committer.Commit()) {
    return CreateResponse(mojom::DBCommandResponse::Status::kTransactionError);
  }

  if (vacuum_requested) {
    if (!db_.Execute("VACUUM")) {
      // If vacuum was not successful, log an error but do not prevent forward
      // progress.
      LOG(ERROR) << "Error executing VACUUM: " << db_.GetErrorMessage();
    }
  }

  return response;
}

mojom::DBCommandResponsePtr RewardsDatabase::Initialize(
    int32_t version,
    int32_t compatible_version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int table_version = 0;
  if (!initialized_) {
    bool should_create_tables = ShouldCreateTables();

    // NOTE: For a new database, the meta table will be initialized with the
    // current DB version. The current version will be immediately overwritten
    // by the first migration, but it is not atomic; there will be a time when a
    // new, empty database has the current version in its meta table.
    if (!meta_table_.Init(&db_, version, compatible_version)) {
      return CreateResponse(
          mojom::DBCommandResponse::Status::kInitializationError);
    }

    if (!should_create_tables) {
      table_version = meta_table_.GetVersionNumber();
    }

    initialized_ = true;
    memory_pressure_listener_ = std::make_unique<base::MemoryPressureListener>(
        FROM_HERE, base::BindRepeating(&RewardsDatabase::OnMemoryPressure,
                                       base::Unretained(this)));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  auto record = mojom::DBRecord::New();
  record->fields.push_back(mojom::DBValue::NewIntValue(table_version));

  auto response = CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  response->records.push_back(std::move(record));
  return response;
}

mojom::DBCommandResponsePtr RewardsDatabase::Execute(
    const mojom::DBCommand& command) {
  if (!initialized_) {
    return CreateResponse(
        mojom::DBCommandResponse::Status::kInitializationError);
  }

  bool result = db_.Execute(command.command);

  if (!result) {
    LOG(ERROR) << "DB Execute error: " << db_.GetErrorMessage();
    return CreateResponse(mojom::DBCommandResponse::Status::kCommandError);
  }

  auto response = CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  response->records.push_back(GetLastChangeCount());
  return response;
}

mojom::DBCommandResponsePtr RewardsDatabase::Run(
    const mojom::DBCommand& command) {
  if (!initialized_) {
    return CreateResponse(
        mojom::DBCommandResponse::Status::kInitializationError);
  }

  sql::Statement statement(db_.GetUniqueStatement(command.command));

  for (auto& binding : command.bindings) {
    HandleBinding(&statement, *binding.get());
  }

  if (!statement.Run()) {
    LOG(ERROR) << "DB Run error: " << db_.GetErrorMessage() << " ("
               << db_.GetErrorCode() << ")";
    return CreateResponse(mojom::DBCommandResponse::Status::kCommandError);
  }

  auto response = CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  response->records.push_back(GetLastChangeCount());
  return response;
}

mojom::DBCommandResponsePtr RewardsDatabase::Read(
    const mojom::DBCommand& command) {
  if (!initialized_) {
    return CreateResponse(
        mojom::DBCommandResponse::Status::kInitializationError);
  }

  sql::Statement statement(db_.GetUniqueStatement(command.command));

  for (auto& binding : command.bindings) {
    HandleBinding(&statement, *binding.get());
  }

  auto response = CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
  while (statement.Step()) {
    response->records.push_back(
        CreateRecord(&statement, command.record_bindings));
  }

  return response;
}

mojom::DBCommandResponsePtr RewardsDatabase::Migrate(
    int32_t version,
    int32_t compatible_version) {
  if (!initialized_) {
    return CreateResponse(
        mojom::DBCommandResponse::Status::kInitializationError);
  }

  CHECK(meta_table_.SetVersionNumber(version));
  CHECK(meta_table_.SetCompatibleVersionNumber(compatible_version));

  return CreateResponse(mojom::DBCommandResponse::Status::kSuccess);
}

mojom::DBRecordPtr RewardsDatabase::GetLastChangeCount() {
  auto record = mojom::DBRecord::New();
  record->fields.push_back(
      mojom::DBValue::NewIntValue(db_.GetLastChangeCount()));
  return record;
}

bool RewardsDatabase::ShouldCreateTables() {
  if (!sql::MetaTable::DoesTableExist(&db_)) {
    return true;
  }

  // If there is only one table in the database, assume that it is the `meta`
  // table and that all other tables need to be created.
  return GetTablesCount() <= 1;
}

int RewardsDatabase::GetTablesCount() {
  sql::Statement statement(db_.GetUniqueStatement(
      "SELECT COUNT(*) FROM sqlite_schema WHERE type='table'"));

  int tables_count = 0;
  if (statement.Step()) {
    tables_count = statement.ColumnInt(0);
  }

  return tables_count;
}

void RewardsDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace brave_rewards::internal
