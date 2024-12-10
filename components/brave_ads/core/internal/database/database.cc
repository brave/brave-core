/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_row_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
#include "sql/sqlite_result_code.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_ads {

Database::Database(base::FilePath path) : db_path_(std::move(path)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  db_.set_error_callback(base::BindRepeating(&Database::ErrorCallback,
                                             weak_factory_.GetWeakPtr()));
}

Database::~Database() = default;

mojom::DBTransactionResultInfoPtr Database::RunDBTransaction(
    mojom::DBTransactionInfoPtr mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mojom::DBTransactionResultInfoPtr mojom_db_transaction_result =
      mojom::DBTransactionResultInfo::New();

  // Open the database if it is not already open.
  if (!db_.is_open() && !db_.Open(db_path_)) {
    mojom_db_transaction_result->status_code =
        mojom::DBTransactionResultInfo::StatusCode::kFailedToOpenDatabase;
    return mojom_db_transaction_result;
  }

  // Maybe raze the database. This must be done before any other database
  // actions are run. All tables must be recreated after the raze action has
  // completed.
  mojom_db_transaction_result->status_code = MaybeRaze(mojom_db_transaction);
  if (database::IsError(mojom_db_transaction_result)) {
    VLOG(0) << "Failed to raze database";
    return mojom_db_transaction_result;
  }

  // Run any actions within the transaction, such as creating or opening the
  // database, executing a statement, or migrating the database.
  mojom_db_transaction_result->status_code =
      RunDBActions(mojom_db_transaction, mojom_db_transaction_result);
  if (database::IsError(mojom_db_transaction_result)) {
    VLOG(0) << "Failed run database actions";
    return mojom_db_transaction_result;
  }

  // Maybe vacuum the database. This must be done after any other actions are
  // run. The database is configured to auto-vacuum with some limitations, but
  // it is good practice to run this action manually.
  mojom_db_transaction_result->status_code = MaybeVacuum(mojom_db_transaction);
  if (database::IsError(mojom_db_transaction_result)) {
    VLOG(0) << "Failed to vacuum database";
    return mojom_db_transaction_result;
  }

  return mojom_db_transaction_result;
}

///////////////////////////////////////////////////////////////////////////////

mojom::DBTransactionResultInfo::StatusCode Database::RunDBActions(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  CHECK(mojom_db_transaction);
  CHECK(mojom_db_transaction_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return mojom::DBTransactionResultInfo::StatusCode::kTransactionError;
  }

  for (const auto& mojom_db_action : mojom_db_transaction->actions) {
    mojom::DBTransactionResultInfo::StatusCode result_code =
        mojom::DBTransactionResultInfo::StatusCode::kSuccess;

    switch (mojom_db_action->type) {
      case mojom::DBActionInfo::Type::kInitialize: {
        result_code = Initialize(mojom_db_transaction_result);
        break;
      }

      case mojom::DBActionInfo::Type::kExecute: {
        result_code = Execute(mojom_db_action);
        break;
      }

      case mojom::DBActionInfo::Type::kRunStatement: {
        result_code = RunStatement(mojom_db_action);
        break;
      }

      case mojom::DBActionInfo::Type::kStepStatement: {
        result_code =
            StepStatement(mojom_db_action, mojom_db_transaction_result);
        break;
      }

      case mojom::DBActionInfo::Type::kMigrate: {
        result_code = Migrate();
        break;
      }
    }

    // Rollback the transaction if the action failed.
    if (result_code != mojom::DBTransactionResultInfo::StatusCode::kSuccess) {
      transaction.Rollback();
      return result_code;
    }
  }

  // Commit the transaction if all actions succeeded.
  if (!transaction.Commit()) {
    return mojom::DBTransactionResultInfo::StatusCode::kTransactionError;
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::MaybeRaze(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  if (mojom_db_transaction->should_raze && !db_.Raze()) {
    return mojom::DBTransactionResultInfo::StatusCode::kFailedToRazeDatabase;
  }

  if (!InitializeMetaTable()) {
    return mojom::DBTransactionResultInfo::StatusCode::
        kFailedToInitializeMetaTable;
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

bool Database::InitializeMetaTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Reset the meta table so that it can be reinitialized.
  meta_table_.Reset();

  return meta_table_.Init(&db_, database::kVersionNumber,
                          database::kCompatibleVersionNumber);
}

bool Database::ShouldCreateTables() {
  if (is_initialized_) {
    // The database is already initialized, so the tables should already exist.
    return false;
  }

  // We need to create the necessary tables if the database contains only the
  // `meta` table or no tables at all, This can happen if the browser crashed
  // after initializing the meta table but before creating the tables.

  sql::Statement statement(db_.GetUniqueStatement(
      "SELECT COUNT(*) <= 1 FROM sqlite_schema WHERE type = 'table';"));
  return statement.Step() && statement.ColumnBool(0);
}

mojom::DBTransactionResultInfo::StatusCode Database::Initialize(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Must be called before initializing the meta table.
  const bool should_create_tables = ShouldCreateTables();

  if (!is_initialized_) {
    if (!InitializeMetaTable()) {
      return mojom::DBTransactionResultInfo::StatusCode::
          kFailedToInitializeMetaTable;
    }

    memory_pressure_listener_ = std::make_unique<base::MemoryPressureListener>(
        FROM_HERE,
        base::BindRepeating(&Database::MemoryPressureListenerCallback,
                            weak_factory_.GetWeakPtr()));

    is_initialized_ = true;
  }

  const int meta_table_version_number =
      should_create_tables ? 0 : meta_table_.GetVersionNumber();

  mojom_db_transaction_result->rows_union =
      mojom::DBRowsUnion::NewColumnValueUnion(
          mojom::DBColumnValueUnion::NewIntValue(meta_table_version_number));

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::Execute(
    const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::StatusCode::kDatabaseNotInitialized;
  }

  if (!db_.Execute(*mojom_db_action->sql)) {
    VLOG(0) << "Failed to execute SQL statement: " << *mojom_db_action->sql;
    return mojom::DBTransactionResultInfo::StatusCode::kStatementError;
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::RunStatement(
    const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::StatusCode::kDatabaseNotInitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(*mojom_db_action->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to invalid SQL statement: " << *mojom_db_action->sql;
    return mojom::DBTransactionResultInfo::StatusCode::kStatementError;
  }

  for (const auto& mojom_db_bind_column : mojom_db_action->bind_columns) {
    database::BindColumn(&statement, *mojom_db_bind_column);
  }

  if (!statement.Run()) {
    VLOG(0) << "Failed to run SQL statement: " << *mojom_db_action->sql;
    return mojom::DBTransactionResultInfo::StatusCode::kStatementError;
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::StepStatement(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);
  CHECK(mojom_db_transaction_result);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::StatusCode::kDatabaseNotInitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(*mojom_db_action->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to invalid SQL statement: " << *mojom_db_action->sql;
    return mojom::DBTransactionResultInfo::StatusCode::kStatementError;
  }

  for (const auto& mojom_db_bind_column : mojom_db_action->bind_columns) {
    database::BindColumn(&statement, *mojom_db_bind_column);
  }

  mojom_db_transaction_result->rows_union =
      mojom::DBRowsUnion::NewRows(std::vector<mojom::DBRowInfoPtr>());
  std::vector<mojom::DBRowInfoPtr>& rows =
      mojom_db_transaction_result->rows_union->get_rows();
  while (statement.Step()) {
    mojom::DBRowInfoPtr row =
        database::CreateRow(&statement, mojom_db_action->bind_column_types);
    rows.push_back(std::move(row));
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::Migrate() {
  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::StatusCode::kDatabaseNotInitialized;
  }

  if (!meta_table_.SetVersionNumber(database::kVersionNumber) ||
      !meta_table_.SetCompatibleVersionNumber(
          database::kCompatibleVersionNumber)) {
    VLOG(0) << "Failed to migrate database";
    return mojom::DBTransactionResultInfo::StatusCode::kMigrationError;
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

mojom::DBTransactionResultInfo::StatusCode Database::MaybeVacuum(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  if (mojom_db_transaction->should_vacuum && !db_.Execute("VACUUM;")) {
    // Log the error and continue. The vacuum action is not critical.
    VLOG(0) << "Failed to vacuum database";
  }

  return mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

void Database::ErrorCallback(int extended_error,
                             sql::Statement* const statement) {
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

    const sql::SqliteResultCode result_code =
        sql::ToSqliteResultCode(extended_error);

    if (result_code != sql::SqliteResultCode::kFullDisk &&
        result_code != sql::SqliteResultCode::kIoRead &&
        result_code != sql::SqliteResultCode::kIoWrite &&
        result_code != sql::SqliteResultCode::kIoFsync &&
        result_code != sql::SqliteResultCode::kIoTruncate) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_NUMBER("Issue32066", "sqlite_schema_version",
                              database::kVersionNumber);
      SCOPED_CRASH_KEY_STRING1024(
          "Issue32066", "sqlite_diagnostic_info",
          db_.GetDiagnosticInfo(extended_error, statement));
      SCOPED_CRASH_KEY_STRING1024("Issue32066", "sqlite_error_message",
                                  db_.GetErrorMessage());
      SCOPED_CRASH_KEY_NUMBER("Issue32066", "sqlite_result_code",
                              static_cast<int>(result_code));
      base::debug::DumpWithoutCrashing();
    }
  }
}

void Database::MemoryPressureListenerCallback(
    base::MemoryPressureListener::
        MemoryPressureLevel /*memory_pressure_level*/) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  db_.TrimMemory();
}

}  // namespace brave_ads
