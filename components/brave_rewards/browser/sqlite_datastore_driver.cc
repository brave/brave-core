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

namespace brave_rewards {

namespace {

void HandleBinding(sql::Statement* statement,
                   const bat_ledger::mojom::DataStoreBinding& binding) {
  switch (binding.value->which()) {
    case bat_ledger::mojom::DataStoreValue::Tag::STRING_VALUE:
      statement->BindString(binding.index, binding.value->get_string_value());
      return;
    case bat_ledger::mojom::DataStoreValue::Tag::INT_VALUE:
      statement->BindInt(binding.index, binding.value->get_int_value());
      return;
    case bat_ledger::mojom::DataStoreValue::Tag::INT64_VALUE:
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      return;
    case bat_ledger::mojom::DataStoreValue::Tag::DOUBLE_VALUE:
      statement->BindDouble(binding.index, binding.value->get_double_value());
      return;
    default:
      NOTREACHED();
  }
}

bat_ledger::mojom::DataStoreRecordPtr HandleResult(sql::Statement* statement) {
  auto result = bat_ledger::mojom::DataStoreRecord::New();
  // TODO(bridiver) - iterate through the bindings to populate the result
  // result.SetString(1, statement.ColumnString(1)), etc..
  return result;
}

}  // namespace

SqliteDatastoreDriver::SqliteDatastoreDriver(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

SqliteDatastoreDriver::~SqliteDatastoreDriver() {}

bool SqliteDatastoreDriver::Inititalize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (initialized_)
    return true;

  if (!db_.Open(db_path_))
    return false;

  initialized_ = true;
  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&SqliteDatastoreDriver::OnMemoryPressure,
      base::Unretained(this))));

  return initialized_;
}

void SqliteDatastoreDriver::RunDataStoreCommand(
    bat_ledger::mojom::DataStoreCommand* command,
    bat_ledger::mojom::DataStoreCommandResponse* response) {

  switch (command->type) {
    case bat_ledger::mojom::DataStoreCommand::Type::INITIALIZE:
      if (Inititalize())
        response->status = ledger::Result::LEDGER_OK;
      else
        response->status = ledger::Result::LEDGER_ERROR;
      return;
    case bat_ledger::mojom::DataStoreCommand::Type::CREATE:
      Execute(command, response);
      return;
    case bat_ledger::mojom::DataStoreCommand::Type::READ:
      Query(command, response);
      return;
    case bat_ledger::mojom::DataStoreCommand::Type::UPDATE:
      Execute(command, response);
      return;
    case bat_ledger::mojom::DataStoreCommand::Type::DELETE:
      Execute(command, response);
      return;
    default:
      NOTREACHED();
  }
}

void SqliteDatastoreDriver::Execute(
    bat_ledger::mojom::DataStoreCommand* command,
    bat_ledger::mojom::DataStoreCommandResponse* response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_) {
    response->status = ledger::Result::LEDGER_ERROR;
    return;
  }

  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      command->payload.c_str()));

  for (auto const& binding : command->bindings)
    HandleBinding(&statement, *binding.get());

  if (statement.Run())
    response->status = ledger::Result::LEDGER_OK;
}

void SqliteDatastoreDriver::Query(
    bat_ledger::mojom::DataStoreCommand* command,
    bat_ledger::mojom::DataStoreCommandResponse* response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_) {
    response->status = ledger::Result::LEDGER_ERROR;
    return;
  }

  sql::Statement statement(db_.GetUniqueStatement(command->payload.c_str()));

  for (auto const& binding : command->bindings)
    HandleBinding(&statement, *binding.get());

  while (statement.Step()) {
    response->results.push_back(HandleResult(&statement));
  }

  response->status = ledger::Result::LEDGER_OK;
}

void SqliteDatastoreDriver::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

}  // namespace brave_rewards
