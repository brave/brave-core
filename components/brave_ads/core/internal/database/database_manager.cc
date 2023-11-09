/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_manager.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
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

void DatabaseManager::AddObserver(DatabaseManagerObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void DatabaseManager::RemoveObserver(DatabaseManagerObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

void DatabaseManager::CreateOrOpen(ResultCallback callback) {
  NotifyWillCreateOrOpenDatabase();

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->version = database::kVersion;
  transaction->compatible_version = database::kCompatibleVersion;

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::INITIALIZE;

  transaction->commands.push_back(std::move(command));

  RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseManager::CreateOrOpenCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseManager::CreateOrOpenCallback(
    ResultCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to open or create database");
    NotifyFailedToCreateOrOpenDatabase();
    return std::move(callback).Run(/*success=*/false);
  }

  CHECK(command_response->result);
  CHECK(command_response->result->get_value()->which() ==
        mojom::DBValue::Tag::kIntValue);
  const int from_version =
      command_response->result->get_value()->get_int_value();

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
    BLOG(1, "Failed to create database for schema version " << to_version);
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
    BLOG(0, "Failed to migrate database from schema version "
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
    BLOG(1, "Failed to migrate database from schema version "
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
