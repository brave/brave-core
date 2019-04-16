/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/sqlite_datastore_driver.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "bat/ledger/ledger_client.h"
#include "sql/statement.h"
#include "sql/transaction.h"

using bat_ledger::mojom::DataStoreCommand;
using bat_ledger::mojom::DataStoreCommandBinding;
using bat_ledger::mojom::DataStoreCommandResponse;
using bat_ledger::mojom::DataStoreRecord;
using bat_ledger::mojom::DataStoreRecordBinding;
using bat_ledger::mojom::DataStoreRecordBindingPtr;
using bat_ledger::mojom::DataStoreRecordPtr;
using bat_ledger::mojom::DataStoreTransaction;
using bat_ledger::mojom::DataStoreValue;

namespace brave_rewards {

namespace {

void HandleBinding(sql::Statement* statement,
                   const DataStoreCommandBinding& binding) {
  switch (binding.value->which()) {
    case DataStoreValue::Tag::STRING_VALUE:
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    case DataStoreValue::Tag::INT_VALUE:
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    case DataStoreValue::Tag::INT64_VALUE:
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    case DataStoreValue::Tag::DOUBLE_VALUE:
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    case DataStoreValue::Tag::BOOL_VALUE:
      statement->BindBool(binding.index, binding.value->get_bool_value());
      return;
    default:
      NOTREACHED();
  }
}

DataStoreRecordPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<DataStoreRecordBindingPtr>& bindings) {
  auto record = DataStoreRecord::New();

  int column = 0;

  for (auto const& binding : bindings) {
    auto value = DataStoreValue::New();
    switch (binding->type) {
      case DataStoreRecordBinding::Type::STRING_TYPE:
        value->set_string_value(statement->ColumnString(column));
        break;
      case DataStoreRecordBinding::Type::INT_TYPE:
        value->set_int_value(statement->ColumnInt(column));
        break;
      case DataStoreRecordBinding::Type::INT64_TYPE:
        value->set_int64_value(statement->ColumnInt64(column));
        break;
      case DataStoreRecordBinding::Type::DOUBLE_TYPE:
        value->set_double_value(statement->ColumnDouble(column));
        break;
      case DataStoreRecordBinding::Type::BOOL_TYPE:
        value->set_bool_value(statement->ColumnBool(column));
        break;
      default:
        NOTREACHED();
    }
    record->fields.push_back(std::move(value));
    column++;
  }

  return record;
}

}  // namespace

SqliteDatastoreDriver::SqliteDatastoreDriver(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

SqliteDatastoreDriver::~SqliteDatastoreDriver() {}

void SqliteDatastoreDriver::RunDataStoreTransaction(
    DataStoreTransaction* transaction,
    DataStoreCommandResponse* response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_) {
    response->status = DataStoreCommandResponse::Status::INITIALIZATION_ERROR;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    response->status = DataStoreCommandResponse::Status::TRANSACTION_ERROR;
    return;
  }

  for (auto const& command : transaction->commands) {
    DataStoreCommandResponse::Status status;

    switch (command->type) {
      case DataStoreCommand::Type::INITIALIZE:
        status = Inititalize(command.get(), response);
        break;
      case DataStoreCommand::Type::CREATE:
        status = Execute(command.get());
        break;
      case DataStoreCommand::Type::READ:
        status = Query(command.get(), response);
        break;
      case DataStoreCommand::Type::UPDATE:
        status = Execute(command.get());
        break;
      case DataStoreCommand::Type::DELETE:
        status = Execute(command.get());
        break;
      case DataStoreCommand::Type::MIGRATE:
        status = Migrate(command.get());
        break;
      default:
        NOTREACHED();
    }

    if (status != DataStoreCommandResponse::Status::OK) {
      committer.Rollback();
      response->status = status;
      return;
    }
  }

  if (!committer.Commit()) {
    response->status = DataStoreCommandResponse::Status::TRANSACTION_ERROR;
  }
}

DataStoreCommandResponse::Status SqliteDatastoreDriver::Inititalize(
    DataStoreCommand* command,
    DataStoreCommandResponse* response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_) {
    if (!db_.Open(db_path_))
      return DataStoreCommandResponse::Status::INITIALIZATION_ERROR;

    if (!meta_table_.Init(
        &db_,
        command->version,
        command->compatible_version))
      return DataStoreCommandResponse::Status::INITIALIZATION_ERROR;

    initialized_ = true;
    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        base::Bind(&SqliteDatastoreDriver::OnMemoryPressure,
        base::Unretained(this))));
  }

  auto value = DataStoreValue::New();
  value->set_int_value(meta_table_.GetVersionNumber());
  response->result->set_value(std::move(value));

  return DataStoreCommandResponse::Status::OK;
}

DataStoreCommandResponse::Status SqliteDatastoreDriver::Execute(
    DataStoreCommand* command) {
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      command->command.c_str()));

  for (auto const& binding : command->bindings)
    HandleBinding(&statement, *binding.get());

  if (statement.Run())
    return DataStoreCommandResponse::Status::COMMAND_ERROR;

  return DataStoreCommandResponse::Status::OK;
}

DataStoreCommandResponse::Status SqliteDatastoreDriver::Query(
    DataStoreCommand* command,
    DataStoreCommandResponse* response) {
  sql::Statement statement(
      db_.GetCachedStatement(SQL_FROM_HERE, command->command.c_str()));

  for (auto const& binding : command->bindings)
    HandleBinding(&statement, *binding.get());

  response->result->set_records(std::vector<DataStoreRecordPtr>());
  while (statement.Step())
    response->result->get_records().push_back(
        CreateRecord(&statement, command->record_bindings));

  return DataStoreCommandResponse::Status::OK;
}

DataStoreCommandResponse::Status SqliteDatastoreDriver::Migrate(
    DataStoreCommand* command) {
  auto status = Execute(command);

  if (status == DataStoreCommandResponse::Status::OK) {
    meta_table_.SetVersionNumber(command->version);
    meta_table_.SetCompatibleVersionNumber(command->compatible_version);
  }

  return status;
}

void SqliteDatastoreDriver::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace brave_rewards
