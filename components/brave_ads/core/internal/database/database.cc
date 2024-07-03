/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/database/database.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_record_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_ads {

namespace {

constexpr char kGetTablesCountSql[] =
    R"(
        SELECT
          COUNT(*)
        FROM
          sqlite_schema
        WHERE
          type = 'table';)";

}  // namespace

Database::Database(base::FilePath path) : db_path_(std::move(path)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  db_.set_error_callback(base::BindRepeating(&Database::ErrorCallback,
                                             weak_factory_.GetWeakPtr()));
}

Database::~Database() = default;

mojom::DBCommandResponseInfoPtr Database::RunTransaction(
    mojom::DBTransactionInfoPtr transaction) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mojom::DBCommandResponseInfoPtr command_response =
      mojom::DBCommandResponseInfo::New();

  RunTransactionImpl(std::move(transaction), command_response.get());

  return command_response;
}

void Database::RunTransactionImpl(
    mojom::DBTransactionInfoPtr transaction,
    mojom::DBCommandResponseInfo* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(transaction);
  CHECK(command_response);

  if (!db_.is_open() && !db_.Open(db_path_)) {
    command_response->status =
        mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    command_response->status =
        mojom::DBCommandResponseInfo::StatusType::TRANSACTION_ERROR;
    return;
  }

  for (const auto& command : transaction->commands) {
    mojom::DBCommandResponseInfo::StatusType status;

    switch (command->type) {
      case mojom::DBCommandInfo::Type::INITIALIZE: {
        status = Initialize(transaction->version,
                            transaction->compatible_version, command_response);
        break;
      }

      case mojom::DBCommandInfo::Type::READ: {
        status = Read(&*command, command_response);
        break;
      }

      case mojom::DBCommandInfo::Type::EXECUTE: {
        status = Execute(&*command);
        break;
      }

      case mojom::DBCommandInfo::Type::RUN: {
        status = Run(&*command);
        break;
      }

      case mojom::DBCommandInfo::Type::MIGRATE: {
        status = Migrate(transaction->version, transaction->compatible_version);
        break;
      }
    }

    if (status != mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
      committer.Rollback();
      command_response->status = status;
      return;
    }
  }

  if (!committer.Commit()) {
    command_response->status =
        mojom::DBCommandResponseInfo::StatusType::TRANSACTION_ERROR;
  }
}

mojom::DBCommandResponseInfo::StatusType Database::Initialize(
    const int32_t version,
    const int32_t compatible_version,
    mojom::DBCommandResponseInfo* command_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(command_response);

  int table_version = 0;

  if (!is_initialized_) {
    const bool should_create_tables = ShouldCreateTables();

    if (!meta_table_.Init(&db_, version, compatible_version)) {
      return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
    }

    if (!should_create_tables) {
      table_version = meta_table_.GetVersionNumber();
    }

    is_initialized_ = true;
    memory_pressure_listener_ = std::make_unique<base::MemoryPressureListener>(
        FROM_HERE,
        base::BindRepeating(&Database::MemoryPressureListenerCallback,
                            weak_factory_.GetWeakPtr()));
  } else {
    table_version = meta_table_.GetVersionNumber();
  }

  command_response->result = mojom::DBCommandResult::NewValue(
      mojom::DBValue::NewIntValue(table_version));

  return mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK;
}

mojom::DBCommandResponseInfo::StatusType Database::Execute(
    mojom::DBCommandInfo* command) {
  CHECK(command);

  if (!is_initialized_) {
    return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
  }

  if (!db_.Execute(command->sql)) {
    VLOG(0) << "Database error: " << db_.GetErrorMessage();
    return mojom::DBCommandResponseInfo::StatusType::COMMAND_ERROR;
  }

  return mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK;
}

mojom::DBCommandResponseInfo::StatusType Database::Run(
    mojom::DBCommandInfo* command) {
  CHECK(command);

  if (!is_initialized_) {
    return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(command->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Invalid database statement " << statement.GetSQLStatement();
    return mojom::DBCommandResponseInfo::StatusType::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    database::Bind(&statement, *binding);
  }

  if (!statement.Run()) {
    return mojom::DBCommandResponseInfo::StatusType::COMMAND_ERROR;
  }

  return mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK;
}

mojom::DBCommandResponseInfo::StatusType Database::Read(
    mojom::DBCommandInfo* command,
    mojom::DBCommandResponseInfo* command_response) {
  CHECK(command);
  CHECK(command_response);

  if (!is_initialized_) {
    return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(command->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Invalid database statement " << statement.GetSQLStatement();
    return mojom::DBCommandResponseInfo::StatusType::COMMAND_ERROR;
  }

  for (const auto& binding : command->bindings) {
    database::Bind(&statement, *binding);
  }

  command_response->result =
      mojom::DBCommandResult::NewRecords(std::vector<mojom::DBRecordInfoPtr>());

  while (statement.Step()) {
    command_response->result->get_records().push_back(
        database::CreateRecord(&statement, command->record_bindings));
  }

  return mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK;
}

mojom::DBCommandResponseInfo::StatusType Database::Migrate(
    const int32_t version,
    const int32_t compatible_version) {
  if (!is_initialized_) {
    return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
  }

  if (!meta_table_.SetVersionNumber(version) ||
      !meta_table_.SetCompatibleVersionNumber(compatible_version)) {
    return mojom::DBCommandResponseInfo::StatusType::INITIALIZATION_ERROR;
  }

  return mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK;
}

bool Database::ShouldCreateTables() {
  if (!sql::MetaTable::DoesTableExist(&db_)) {
    return true;
  }

  return GetTablesCount() <= 1;
}

int Database::GetTablesCount() {
  sql::Statement statement(db_.GetUniqueStatement(kGetTablesCountSql));

  int tables_count = 0;
  if (statement.Step()) {
    tables_count = statement.ColumnInt(0);
  }

  return tables_count;
}

void Database::ErrorCallback(const int extended_error,
                             sql::Statement* statement) {
  // Attempt to recover a corrupt database, if it is eligible to be recovered.
  if (sql::Recovery::RecoverIfPossible(
          &db_, extended_error,
          sql::Recovery::Strategy::kRecoverWithMetaVersionOrRaze)) {
    // The `DLOG(FATAL)` below is intended to draw immediate attention to errors
    // in newly-written code. Database corruption is generally a result of OS or
    // hardware issues, not coding errors at the client level, so displaying the
    // error would probably lead to confusion. The ignored call signals the
    // test-expectation framework that the error was handled.
    std::ignore = sql::Database::IsExpectedSqliteError(extended_error);
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Database::IsExpectedSqliteError(extended_error)) {
    DLOG(FATAL) << db_.GetErrorMessage();

    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "sqlite_schema_version",
                            database::kVersion);
    SCOPED_CRASH_KEY_STRING1024(
        "Issue32066", "sqlite_diagnostic_info",
        db_.GetDiagnosticInfo(extended_error, statement));
    SCOPED_CRASH_KEY_STRING1024("Issue32066", "sqlite_error_message",
                                db_.GetErrorMessage());
    base::debug::DumpWithoutCrashing();
  }
}

void Database::MemoryPressureListenerCallback(
    base::MemoryPressureListener::
        MemoryPressureLevel /*memory_pressure_level*/) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  db_.TrimMemory();
}

}  // namespace brave_ads
