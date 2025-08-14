/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_manager.h"

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/database/database.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_creation.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_raze.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

DatabaseManager::DatabaseManager(const base::FilePath& path)
    : database_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      database_(base::SequenceBound<Database>(database_task_runner_, path)) {}

DatabaseManager::~DatabaseManager() = default;

// static
DatabaseManager& DatabaseManager::GetInstance() {
  return GlobalState::GetInstance()->GetDatabaseManager();
}

void DatabaseManager::AddObserver(DatabaseManagerObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void DatabaseManager::RemoveObserver(DatabaseManagerObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void DatabaseManager::CreateOrOpen(ResultCallback callback) {
  CHECK(callback);

  NotifyWillCreateOrOpenDatabase();

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kInitialize;
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&DatabaseManager::CreateOrOpenCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      /*trace_id=*/0);
}

void DatabaseManager::RunTransaction(
    mojom::DBTransactionInfoPtr mojom_db_transaction,
    RunDBTransactionCallback callback,
    uint64_t trace_id) {
  CHECK(mojom_db_transaction);
  CHECK(callback);

  // Wrap `callback` with `DatabaseManager::OnRunTransactionCallback` to ensure
  // it's not called after `DatabaseManager` deletion.
  database_.AsyncCall(&Database::RunTransaction)
      .WithArgs(std::move(mojom_db_transaction), trace_id)
      .Then(base::BindOnce(&DatabaseManager::OnRunTransactionCallback,
                           weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseManager::Shutdown(ShutdownCallback callback) {
  CHECK(callback);

  // Wrap `callback` with `DatabaseManager::OnShutdownCallback` to ensure it's
  // not called after `DatabaseManager` deletion.
  database_.AsyncCall(&Database::Poison)
      .Then(base::BindOnce(&DatabaseManager::OnShutdownCallback,
                           weak_factory_.GetWeakPtr(), std::move(callback),
                           /*success=*/true));
}

///////////////////////////////////////////////////////////////////////////////

void DatabaseManager::CreateOrOpenCallback(
    ResultCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!database::IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to create or open database");

    NotifyFailedToCreateOrOpenDatabase();

    return std::move(callback).Run(/*success=*/false);
  }

  CHECK(mojom_db_transaction_result->rows_union);
  CHECK_EQ(mojom_db_transaction_result->rows_union->get_column_value_union()
               ->which(),
           mojom::DBColumnValueUnion::Tag::kIntValue);
  const int from_version =
      mojom_db_transaction_result->rows_union->get_column_value_union()
          ->get_int_value();

  if (from_version == 0) {
    // Fresh install.
    return Create(std::move(callback));
  }

  if (from_version <= database::kRazeDatabaseThresholdVersionNumber) {
    return RazeAndCreate(from_version, std::move(callback));
  }

  NotifyDidOpenDatabase();

  MaybeMigrate(from_version, std::move(callback));
}

void DatabaseManager::OnRunTransactionCallback(
    RunDBTransactionCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  CHECK(callback);
  CHECK(mojom_db_transaction_result);
  std::move(callback).Run(std::move(mojom_db_transaction_result));
}

void DatabaseManager::OnShutdownCallback(ShutdownCallback callback,
                                         bool success) {
  CHECK(callback);
  std::move(callback).Run(success);
}

void DatabaseManager::Create(ResultCallback callback) const {
  BLOG(1, "Create database for schema version " << database::kVersionNumber);

  database::Create(base::BindOnce(&DatabaseManager::CreateCallback,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(callback)));
}

void DatabaseManager::CreateCallback(ResultCallback callback,
                                     bool success) const {
  if (!success) {
    SCOPED_CRASH_KEY_STRING64("Issue43317", "failure_reason",
                              "Failed to create database");
    SCOPED_CRASH_KEY_NUMBER("Issue43317", "sqlite_schema_version",
                            database::kVersionNumber);
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to create database for schema version "
                << database::kVersionNumber);

    NotifyFailedToCreateOrOpenDatabase();

    return std::move(callback).Run(/*success=*/false);
  }

  BLOG(1, "Created database for schema version " << database::kVersionNumber);

  NotifyDidCreateDatabase();

  NotifyDatabaseIsReady();

  std::move(callback).Run(/*success=*/true);
}

void DatabaseManager::RazeAndCreate(int from_version, ResultCallback callback) {
  BLOG(1, "Razing database for schema version " << from_version);

  database::Raze(base::BindOnce(&DatabaseManager::RazeAndCreateCallback,
                                weak_factory_.GetWeakPtr(), std::move(callback),
                                from_version));
}

void DatabaseManager::RazeAndCreateCallback(ResultCallback callback,
                                            int from_version,
                                            bool success) const {
  if (!success) {
    SCOPED_CRASH_KEY_STRING64("Issue43331", "failure_reason",
                              "Failed to raze database");
    SCOPED_CRASH_KEY_NUMBER("Issue43331", "from_sqlite_schema_version",
                            from_version);
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to raze database for schema version " << from_version);
    return std::move(callback).Run(/*success=*/false);
  }

  BLOG(1, "Razed database for schema version " << from_version);

  Create(std::move(callback));
}

void DatabaseManager::MaybeMigrate(int from_version,
                                   ResultCallback callback) const {
  const int to_version = database::kVersionNumber;
  if (from_version == to_version) {
    BLOG(1, "Database is up to date on schema version " << from_version);

    NotifyDatabaseIsReady();

    return std::move(callback).Run(/*success=*/true);
  }

  if (from_version > to_version) {
    BLOG(0, "Database downgrade not supported from schema version "
                << from_version << " to schema version " << to_version);

    NotifyFailedToMigrateDatabase(from_version, to_version);

    return std::move(callback).Run(/*success=*/false);
  }

  BLOG(1, "Migrating database from schema version "
              << from_version << " to schema version " << to_version);

  NotifyWillMigrateDatabase(from_version, to_version);

  database::MigrateFromVersion(
      from_version, base::BindOnce(&DatabaseManager::MigrateFromVersionCallback,
                                   weak_factory_.GetWeakPtr(), from_version,
                                   std::move(callback)));
}

void DatabaseManager::MigrateFromVersionCallback(int from_version,
                                                 ResultCallback callback,
                                                 bool success) const {
  const int to_version = database::kVersionNumber;

  if (!success) {
    SCOPED_CRASH_KEY_NUMBER("Issue43326", "from_sqlite_schema_version",
                            from_version);
    SCOPED_CRASH_KEY_NUMBER("Issue43326", "to_sqlite_schema_version",
                            to_version);
    SCOPED_CRASH_KEY_STRING64("Issue43326", "failure_reason",
                              "Database migration failed");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to migrate database from schema version "
                << from_version << " to schema version " << to_version);

    NotifyFailedToMigrateDatabase(from_version, to_version);

    return std::move(callback).Run(/*success=*/false);
  }

  BLOG(1, "Migrated database from schema version "
              << from_version << " to schema version " << to_version);

  NotifyDidMigrateDatabase(from_version, to_version);

  NotifyDatabaseIsReady();

  std::move(callback).Run(/*success=*/true);
}

void DatabaseManager::NotifyWillCreateOrOpenDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnWillCreateOrOpenDatabase();
  }
}

void DatabaseManager::NotifyDidCreateDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDidCreateDatabase();
  }
}

void DatabaseManager::NotifyDidOpenDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDidOpenDatabase();
  }
}

void DatabaseManager::NotifyFailedToCreateOrOpenDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnFailedToCreateOrOpenDatabase();
  }
}

void DatabaseManager::NotifyWillMigrateDatabase(int from_version,
                                                int to_version) const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnWillMigrateDatabase(from_version, to_version);
  }
}

void DatabaseManager::NotifyDidMigrateDatabase(int from_version,
                                               int to_version) const {
  CHECK_NE(from_version, to_version);

  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDidMigrateDatabase(from_version, to_version);
  }
}

void DatabaseManager::NotifyFailedToMigrateDatabase(int from_version,
                                                    int to_version) const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnFailedToMigrateDatabase(from_version, to_version);
  }
}

void DatabaseManager::NotifyDatabaseIsReady() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDatabaseIsReady();
  }
}

}  // namespace brave_ads
