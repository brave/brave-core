/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_H_

#include "base/observer_list.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/database/database_manager_observer.h"

namespace ads {

class DatabaseManager final {
 public:
  DatabaseManager();

  DatabaseManager(const DatabaseManager& other) = delete;
  DatabaseManager& operator=(const DatabaseManager& other) = delete;

  DatabaseManager(DatabaseManager&& other) noexcept = delete;
  DatabaseManager& operator=(DatabaseManager&& other) noexcept = delete;

  ~DatabaseManager();

  static DatabaseManager* GetInstance();

  static bool HasInstance();

  void AddObserver(DatabaseManagerObserver* observer);
  void RemoveObserver(DatabaseManagerObserver* observer);

  void CreateOrOpen(ResultCallback callback);

 private:
  void OnCreateOrOpen(ResultCallback callback,
                      mojom::DBCommandResponseInfoPtr response);

  void MaybeMigrate(int from_version, ResultCallback callback) const;
  void OnMigrate(int from_version, ResultCallback callback, bool success) const;

  void NotifyWillCreateOrOpenDatabase() const;
  void NotifyDidCreateOrOpenDatabase() const;
  void NotifyFailedToCreateOrOpenDatabase() const;
  void NotifyWillMigrateDatabase(int from_version, int to_version) const;
  void NotifyDidMigrateDatabase(int from_version, int to_version) const;
  void NotifyFailedToMigrateDatabase(int from_version, int to_version) const;
  void NotifyDatabaseIsReady() const;

  base::ObserverList<DatabaseManagerObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_H_
