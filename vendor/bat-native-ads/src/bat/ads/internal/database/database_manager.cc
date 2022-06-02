/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_manager.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "bat/ads/internal/ads_client_helper.h"
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
DatabaseManager* DatabaseManager::Get() {
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

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->version = database::kVersion;
  transaction->compatible_version = database::kCompatibleVersion;

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), [=](mojom::DBCommandResponsePtr response) {
        DCHECK(response);

        if (response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
          NotifyFailedToCreateOrOpenDatabase();
          callback(/* success */ false);
          return;
        }

        if (!response->result) {
          NotifyFailedToCreateOrOpenDatabase();
          callback(/* success */ false);
          return;
        }

        NotifyDidCreateOrOpenDatabase();

        DCHECK(response->result->get_value()->which() ==
               mojom::DBValue::Tag::INT_VALUE);
        const int from_version = response->result->get_value()->get_int_value();
        MaybeMigrate(from_version, callback);
      });
}

///////////////////////////////////////////////////////////////////////////////

void DatabaseManager::MaybeMigrate(const int from_version,
                                   ResultCallback callback) const {
  const int to_version = database::kVersion;
  DCHECK(from_version <= to_version);

  if (from_version == to_version) {
    callback(/* success */ true);
    return;
  }

  NotifyWillMigrateDatabase(from_version, to_version);

  database::Migration database_migration;
  database_migration.FromVersion(from_version, [=](const bool success) {
    if (!success) {
      NotifyFailedToMigrateDatabase(from_version, to_version);
      callback(/* success */ false);
      return;
    }

    NotifyDidMigrateDatabase(from_version, to_version);

    callback(/* success */ true);
  });
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

}  // namespace ads
