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

mojom::DBStatementResultInfoPtr Database::RunTransaction(
    mojom::DBTransactionInfoPtr mojom_transaction) {
  CHECK(mojom_transaction);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mojom::DBStatementResultInfoPtr mojom_statement_result =
      mojom::DBStatementResultInfo::New();

  RunTransactionImpl(std::move(mojom_transaction), &*mojom_statement_result);

  return mojom_statement_result;
}

///////////////////////////////////////////////////////////////////////////////

void Database::RunTransactionImpl(
    mojom::DBTransactionInfoPtr mojom_transaction,
    mojom::DBStatementResultInfo* const mojom_statement_result) {
  CHECK(mojom_transaction);
  CHECK(mojom_statement_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!db_.is_open() && !db_.Open(db_path_)) {
    mojom_statement_result->result_code =
        mojom::DBStatementResultInfo::ResultCode::kFailedToOpenDatabase;
    return;
  }

  should_vacuum_ = false;

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    mojom_statement_result->result_code =
        mojom::DBStatementResultInfo::ResultCode::kTransactionError;
    return;
  }

  for (const auto& mojom_statement : mojom_transaction->statements) {
    mojom::DBStatementResultInfo::ResultCode result_code =
        mojom::DBStatementResultInfo::ResultCode::kSuccess;

    switch (mojom_statement->operation_type) {
      case mojom::DBStatementInfo::OperationType::kCreateOrOpen: {
        result_code = Initialize(&*mojom_transaction, mojom_statement_result);
        break;
      }

      case mojom::DBStatementInfo::OperationType::kStep: {
        result_code = Step(&*mojom_statement, mojom_statement_result);
        break;
      }

      case mojom::DBStatementInfo::OperationType::kExecute: {
        result_code = Execute(&*mojom_statement);
        break;
      }

      case mojom::DBStatementInfo::OperationType::kRun: {
        result_code = Run(&*mojom_statement);
        break;
      }

      case mojom::DBStatementInfo::OperationType::kMigrate: {
        result_code = Migrate(&*mojom_transaction);
        break;
      }

      case mojom::DBStatementInfo::OperationType::kVacuum: {
        should_vacuum_ = true;
        result_code = mojom::DBStatementResultInfo::ResultCode::kSuccess;
        break;
      }
    }

    if (result_code != mojom::DBStatementResultInfo::ResultCode::kSuccess) {
      // Rollback the transaction if any of the statements failed.
      mojom_statement_result->result_code = result_code;
      return transaction.Rollback();
    }
  }

  // Commit the transaction if all statements succeeded.

  if (!transaction.Commit()) {
    mojom_statement_result->result_code =
        mojom::DBStatementResultInfo::ResultCode::kTransactionError;
  }

  if (should_vacuum_ && !db_.Execute("VACUUM")) {
    // Log the error and continue. The vacuum operation is not critical.
    VLOG(0) << "Database error: " << db_.GetErrorMessage();
  }
}

mojom::DBStatementResultInfo::ResultCode Database::Initialize(
    const mojom::DBTransactionInfo* const mojom_transaction,
    mojom::DBStatementResultInfo* const mojom_statement_result) {
  CHECK(mojom_transaction);
  CHECK(mojom_statement_result);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int table_version = 0;

  if (!is_initialized_) {
    const bool should_create_tables = ShouldCreateTables();

    if (!meta_table_.Init(&db_, mojom_transaction->version,
                          mojom_transaction->compatible_version)) {
      return mojom::DBStatementResultInfo::ResultCode::
          kFailedToInitializeMetaTable;
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

  mojom_statement_result->rows_union = mojom::DBRowsUnion::NewColumnValueUnion(
      mojom::DBColumnValueUnion::NewIntValue(table_version));

  return mojom::DBStatementResultInfo::ResultCode::kSuccess;
}

mojom::DBStatementResultInfo::ResultCode Database::Execute(
    const mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  if (!is_initialized_) {
    VLOG(0) << "Failed because database is not initialized";
    return mojom::DBStatementResultInfo::ResultCode::kUninitialized;
  }

  if (!db_.Execute(mojom_statement->sql)) {
    VLOG(0) << "Failed due to database error: " << db_.GetErrorMessage();
    return mojom::DBStatementResultInfo::ResultCode::kStatementError;
  }

  return mojom::DBStatementResultInfo::ResultCode::kSuccess;
}

mojom::DBStatementResultInfo::ResultCode Database::Run(
    const mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  if (!is_initialized_) {
    VLOG(0) << "Failed because database is not initialized";
    return mojom::DBStatementResultInfo::ResultCode::kUninitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(mojom_statement->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to invalid database SQL statement: "
            << statement.GetSQLStatement();
    return mojom::DBStatementResultInfo::ResultCode::kStatementError;
  }

  for (const auto& mojom_bind_column : mojom_statement->bind_columns) {
    database::BindColumn(&statement, *mojom_bind_column);
  }

  if (!statement.Run()) {
    VLOG(0) << "Failed due to invalid database SQL statement: "
            << statement.GetSQLStatement();
    return mojom::DBStatementResultInfo::ResultCode::kStatementError;
  }

  return mojom::DBStatementResultInfo::ResultCode::kSuccess;
}

mojom::DBStatementResultInfo::ResultCode Database::Step(
    const mojom::DBStatementInfo* const mojom_statement,
    mojom::DBStatementResultInfo* const mojom_statement_result) {
  CHECK(mojom_statement);
  CHECK(mojom_statement_result);

  if (!is_initialized_) {
    VLOG(0) << "Failed because database is not initialized";
    return mojom::DBStatementResultInfo::ResultCode::kUninitialized;
  }

  sql::Statement statement;
  statement.Assign(db_.GetUniqueStatement(mojom_statement->sql));
  if (!statement.is_valid()) {
    VLOG(0) << "Failed due to invalid database SQL statement "
            << statement.GetSQLStatement();
    return mojom::DBStatementResultInfo::ResultCode::kStatementError;
  }

  for (const auto& mojom_bind_column : mojom_statement->bind_columns) {
    database::BindColumn(&statement, *mojom_bind_column);
  }

  mojom_statement_result->rows_union =
      mojom::DBRowsUnion::NewRows(std::vector<mojom::DBRowInfoPtr>());

  while (statement.Step()) {
    mojom_statement_result->rows_union->get_rows().push_back(
        database::CreateRow(&statement, mojom_statement->bind_column_types));
  }

  return mojom::DBStatementResultInfo::ResultCode::kSuccess;
}

mojom::DBStatementResultInfo::ResultCode Database::Migrate(
    const mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  if (!is_initialized_) {
    VLOG(0) << "Failed because database is not initialized";
    return mojom::DBStatementResultInfo::ResultCode::kUninitialized;
  }

  if (!meta_table_.SetVersionNumber(mojom_transaction->version) ||
      !meta_table_.SetCompatibleVersionNumber(
          mojom_transaction->compatible_version)) {
    VLOG(0) << "Failed to migrate database";
    return mojom::DBStatementResultInfo::ResultCode::kMigrationError;
  }

  return mojom::DBStatementResultInfo::ResultCode::kSuccess;
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
