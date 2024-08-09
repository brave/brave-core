/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_manager.h"

#include <utility>

#include "base/check_op.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_creation.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_migration.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

DatabaseManager::DatabaseManager() = default;

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
  NotifyWillCreateOrOpenDatabase();

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom_transaction->version = database::kVersion;
  mojom_transaction->compatible_version = database::kCompatibleVersion;

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kCreateOrOpen;
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(
      std::move(mojom_transaction),
      base::BindOnce(&DatabaseManager::CreateOrOpenCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseManager::CreateOrOpenCallback(
    ResultCallback callback,
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to create or open database");

    NotifyFailedToCreateOrOpenDatabase();

    return std::move(callback).Run(/*success=*/false);
  }

  CHECK(mojom_statement_result->rows_union);
  CHECK_EQ(
      mojom_statement_result->rows_union->get_column_value_union()->which(),
      mojom::DBColumnValueUnion::Tag::kIntValue);
  const int from_version =
      mojom_statement_result->rows_union->get_column_value_union()
          ->get_int_value();

  if (from_version == 0) {
    // Fresh install.
    return Create(std::move(callback));
  }

  NotifyDidOpenDatabase();

  MaybeMigrate(from_version, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void DatabaseManager::Create(ResultCallback callback) const {
  BLOG(1, "Create database for schema version " << database::kVersion);

  database::Create(base::BindOnce(&DatabaseManager::CreateCallback,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(callback)));
}

void DatabaseManager::CreateCallback(ResultCallback callback,
                                     const bool success) const {
  const int to_version = database::kVersion;

  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Failed to create database");
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "sqlite_schema_version", to_version);
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to create database for schema version " << to_version);

    NotifyFailedToCreateOrOpenDatabase();

    return std::move(callback).Run(/*success=*/false);
  }

  BLOG(1, "Created database for schema version " << to_version);

  NotifyDidCreateDatabase();

  NotifyDatabaseIsReady();

  std::move(callback).Run(/*success=*/true);
}

void DatabaseManager::MaybeMigrate(const int from_version,
                                   ResultCallback callback) const {
  const int to_version = database::kVersion;
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

void DatabaseManager::MigrateFromVersionCallback(const int from_version,
                                                 ResultCallback callback,
                                                 const bool success) const {
  const int to_version = database::kVersion;

  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "from_sqlite_schema_version",
                            from_version);
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "to_sqlite_schema_version",
                            to_version);
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
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

void DatabaseManager::NotifyWillMigrateDatabase(const int from_version,
                                                const int to_version) const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnWillMigrateDatabase(from_version, to_version);
  }
}

void DatabaseManager::NotifyDidMigrateDatabase(const int from_version,
                                               const int to_version) const {
  CHECK_NE(from_version, to_version);

  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDidMigrateDatabase(from_version, to_version);
  }
}

void DatabaseManager::NotifyFailedToMigrateDatabase(
    const int from_version,
    const int to_version) const {
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
