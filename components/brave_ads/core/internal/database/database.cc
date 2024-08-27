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
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_row_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
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
    mojom_db_transaction_result->result_code =
        mojom::DBTransactionResultInfo::ResultCode::kFailedToOpenDatabase;
    return mojom_db_transaction_result;
  }

  // Run any actions within the transaction, such as creating or opening the
  // database, executing a statement, or migrating the database.
  mojom_db_transaction_result->result_code =
      RunDBActions(mojom_db_transaction, &*mojom_db_transaction_result);
  if (mojom_db_transaction_result->result_code !=
      mojom::DBTransactionResultInfo::ResultCode::kSuccess) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom_db_transaction_result;
  }

  // Maybe vacuum the database. This must be done after any other actions are
  // run. The database is configured to auto-vacuum with some limitations, but
  // it is good practice to run this action manually.
  mojom_db_transaction_result->result_code =
      MaybeVacuum(&*mojom_db_transaction);
  if (mojom_db_transaction_result->result_code !=
      mojom::DBTransactionResultInfo::ResultCode::kSuccess) {
    VLOG(0) << "Failed to vacuum database";
    return mojom_db_transaction_result;
  }

  return mojom_db_transaction_result;
}

///////////////////////////////////////////////////////////////////////////////

mojom::DBTransactionResultInfo::ResultCode Database::RunDBActions(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    mojom::DBTransactionResultInfo* const mojom_db_transaction_result) {
  CHECK(mojom_db_transaction);
  CHECK(mojom_db_transaction_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return mojom::DBTransactionResultInfo::ResultCode::kTransactionError;
  }

  for (const auto& mojom_db_action : mojom_db_transaction->actions) {
    mojom::DBTransactionResultInfo::ResultCode result_code =
        mojom::DBTransactionResultInfo::ResultCode::kSuccess;

    switch (mojom_db_action->type) {
      case mojom::DBActionInfo::Type::kInitialize: {
        result_code = Initialize(mojom_db_transaction_result);
        break;
      }

      case mojom::DBActionInfo::Type::kExecute: {
        result_code = Execute(&*mojom_db_action);
        break;
      }

      case mojom::DBActionInfo::Type::kRunStatement: {
        result_code = RunStatement(&*mojom_db_action);
        break;
      }

      case mojom::DBActionInfo::Type::kStepStatement: {
        result_code =
            StepStatement(&*mojom_db_action, mojom_db_transaction_result);
        break;
      }

      case mojom::DBActionInfo::Type::kMigrate: {
        result_code = Migrate();
        break;
      }
    }

    // Rollback the transaction if the action failed.
    if (result_code != mojom::DBTransactionResultInfo::ResultCode::kSuccess) {
      transaction.Rollback();
      return result_code;
    }
  }

  // Commit the transaction if all actions succeeded.
  if (!transaction.Commit()) {
    return mojom::DBTransactionResultInfo::ResultCode::kTransactionError;
  }

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
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

mojom::DBTransactionResultInfo::ResultCode Database::Initialize(
    mojom::DBTransactionResultInfo* const mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Must be called before initializing the meta table.
  const bool should_create_tables = ShouldCreateTables();

  if (!is_initialized_) {
    if (!meta_table_.Init(&db_, database::kVersionNumber,
                          database::kCompatibleVersionNumber)) {
      return mojom::DBTransactionResultInfo::ResultCode::
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

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

mojom::DBTransactionResultInfo::ResultCode Database::Execute(
    const mojom::DBActionInfo* const mojom_db_action) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::ResultCode::kDatabaseNotInitialized;
  }

  if (!db_.Execute(*mojom_db_action->sql)) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom::DBTransactionResultInfo::ResultCode::kStatementError;
  }

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

mojom::DBTransactionResultInfo::ResultCode Database::RunStatement(
    const mojom::DBActionInfo* const mojom_db_action) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::ResultCode::kDatabaseNotInitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(*mojom_db_action->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom::DBTransactionResultInfo::ResultCode::kStatementError;
  }

  for (const auto& mojom_db_bind_column : mojom_db_action->bind_columns) {
    database::BindColumn(&statement, *mojom_db_bind_column);
  }

  if (!statement.Run()) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom::DBTransactionResultInfo::ResultCode::kStatementError;
  }

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

mojom::DBTransactionResultInfo::ResultCode Database::StepStatement(
    const mojom::DBActionInfo* const mojom_db_action,
    mojom::DBTransactionResultInfo* const mojom_db_transaction_result) {
  CHECK(mojom_db_action);
  CHECK(mojom_db_action->sql);
  CHECK(mojom_db_transaction_result);

  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::ResultCode::kDatabaseNotInitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(*mojom_db_action->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom::DBTransactionResultInfo::ResultCode::kStatementError;
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

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

mojom::DBTransactionResultInfo::ResultCode Database::Migrate() {
  if (!is_initialized_) {
    VLOG(0) << "Failed because the database is not initialized";
    return mojom::DBTransactionResultInfo::ResultCode::kDatabaseNotInitialized;
  }

  if (!meta_table_.SetVersionNumber(database::kVersionNumber) ||
      !meta_table_.SetCompatibleVersionNumber(
          database::kCompatibleVersionNumber)) {
    VLOG(0) << "Failed to migrate database";
    return mojom::DBTransactionResultInfo::ResultCode::kMigrationError;
  }

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

mojom::DBTransactionResultInfo::ResultCode Database::MaybeVacuum(
    const mojom::DBTransactionInfo* const mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  if (mojom_db_transaction->should_vacuum && !db_.Execute("VACUUM;")) {
    // Log the error and continue. The vacuum action is not critical.
    VLOG(0) << "Failed to vacuum database";
  }

  return mojom::DBTransactionResultInfo::ResultCode::kSuccess;
}

void Database::ErrorCallback(const int extended_error,
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

    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "sqlite_schema_version",
                            database::kVersionNumber);
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
