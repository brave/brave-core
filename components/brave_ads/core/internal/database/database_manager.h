/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_H_

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_callback.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace brave_ads {

class Database;

class DatabaseManager final {
 public:
  explicit DatabaseManager(const base::FilePath& path);

  DatabaseManager(const DatabaseManager&) = delete;
  DatabaseManager& operator=(const DatabaseManager&) = delete;

  ~DatabaseManager();

  static DatabaseManager& GetInstance();

  void AddObserver(DatabaseManagerObserver* observer);
  void RemoveObserver(DatabaseManagerObserver* observer);

  // Create or open the database.
  void CreateOrOpen(ResultCallback callback);

  // Run a database transaction. The callback takes one argument -
  // `mojom::DBTransactionResultInfoPtr` containing the info of the transaction.
  void RunTransaction(mojom::DBTransactionInfoPtr mojom_db_transaction,
                      RunDBTransactionCallback callback,
                      uint64_t trace_id);

  // Shutdowns the database.
  void Shutdown(ShutdownCallback callback);

 private:
  void CreateOrOpenCallback(
      ResultCallback callback,
      mojom::DBTransactionResultInfoPtr mojom_db_transaction_result);

  // Create the database from scratch.
  void Create(ResultCallback callback) const;
  void CreateCallback(ResultCallback callback, bool success) const;

  // Raze the database and create it from scratch.
  void RazeAndCreate(int from_version, ResultCallback callback);
  void RazeAndCreateCallback(ResultCallback callback,
                             int from_version,
                             bool success) const;

  // Migrate the database from `from_version` to the current version.
  void MaybeMigrate(int from_version, ResultCallback callback) const;
  void MigrateFromVersionCallback(int from_version,
                                  ResultCallback callback,
                                  bool success) const;

  void NotifyWillCreateOrOpenDatabase() const;
  void NotifyDidCreateDatabase() const;
  void NotifyDidOpenDatabase() const;
  void NotifyFailedToCreateOrOpenDatabase() const;
  void NotifyWillMigrateDatabase(int from_version, int to_version) const;
  void NotifyDidMigrateDatabase(int from_version, int to_version) const;
  void NotifyFailedToMigrateDatabase(int from_version, int to_version) const;
  void NotifyDatabaseIsReady() const;

  const scoped_refptr<base::SequencedTaskRunner> database_task_runner_;
  base::SequenceBound<Database> database_;

  base::ObserverList<DatabaseManagerObserver> observers_;

  base::WeakPtrFactory<DatabaseManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_H_
