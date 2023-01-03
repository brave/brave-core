/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_manager.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/legacy_migration/database/database_constants.h"
#include "bat/ads/internal/legacy_migration/database/database_migration.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

namespace {
DatabaseManager* g_database_manager_instance = nullptr;
}  // namespace

DatabaseManager::DatabaseManager() {
  DCHECK(!g_database_manager_instance);
  g_database_manager_instance = this;
}

DatabaseManager::~DatabaseManager() {
  DCHECK_EQ(this, g_database_manager_instance);
  g_database_manager_instance = nullptr;
}

// static
DatabaseManager* DatabaseManager::GetInstance() {
  DCHECK(g_database_manager_instance);
  return g_database_manager_instance;
}

// static
bool DatabaseManager::HasInstance() {
  return !!g_database_manager_instance;
}

void DatabaseManager::AddObserver(DatabaseManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void DatabaseManager::RemoveObserver(DatabaseManagerObserver* observer) {
  DCHECK(observer);
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

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseManager::OnCreateOrOpen, base::Unretained(this),
                     std::move(callback)));
}

void DatabaseManager::OnCreateOrOpen(ResultCallback callback,
                                     mojom::DBCommandResponseInfoPtr response) {
  DCHECK(response);

  if (response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK ||
      !response->result) {
    BLOG(0, "Failed to open or create database");
    NotifyFailedToCreateOrOpenDatabase();
    std::move(callback).Run(/*success*/ false);
    return;
  }

  NotifyDidCreateOrOpenDatabase();

  DCHECK(response->result->get_value()->which() ==
         mojom::DBValue::Tag::kIntValue);
  const int from_version = response->result->get_value()->get_int_value();
  MaybeMigrate(from_version, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void DatabaseManager::MaybeMigrate(const int from_version,
                                   ResultCallback callback) const {
  const int to_version = database::kVersion;
  DCHECK(from_version <= to_version);

  if (from_version == to_version) {
    std::move(callback).Run(/*success*/ true);
    return;
  }

  BLOG(1, "Migrating database from schema version "
              << from_version << " to schema version " << to_version);

  NotifyWillMigrateDatabase(from_version, to_version);

  database::MigrateFromVersion(
      from_version,
      base::BindOnce(&DatabaseManager::OnMigrate, base::Unretained(this),
                     from_version, std::move(callback)));
}

void DatabaseManager::OnMigrate(const int from_version,
                                ResultCallback callback,
                                const bool success) const {
  const int to_version = database::kVersion;
  if (!success) {
    BLOG(1, "Failed to migrate database from schema version "
                << from_version << " to schema version " << to_version);
    NotifyFailedToMigrateDatabase(from_version, to_version);
    std::move(callback).Run(/*success*/ false);
    return;
  }

  BLOG(1, "Migrated database from schema version "
              << from_version << " to schema version " << to_version);

  NotifyDidMigrateDatabase(from_version, to_version);

  NotifyDatabaseIsReady();

  std::move(callback).Run(/*success*/ true);
}

void DatabaseManager::NotifyWillCreateOrOpenDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnWillCreateOrOpenDatabase();
  }
}

void DatabaseManager::NotifyDidCreateOrOpenDatabase() const {
  for (DatabaseManagerObserver& observer : observers_) {
    observer.OnDidCreateOrOpenDatabase();
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
  DCHECK_NE(from_version, to_version);

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

}  // namespace ads
