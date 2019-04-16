/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SQLITE_DATASTORE_DRIVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SQLITE_DATASTORE_DRIVER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/ledger_client.h"
#include "build/build_config.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

using bat_ledger::mojom::DataStoreCommand;
using bat_ledger::mojom::DataStoreCommandResponse;
using bat_ledger::mojom::DataStoreTransaction;

namespace brave_rewards {

class SqliteDatastoreDriver {
 public:
  SqliteDatastoreDriver(const base::FilePath& db_path);

  ~SqliteDatastoreDriver();

  void RunDataStoreTransaction(DataStoreTransaction* transaction,
                               DataStoreCommandResponse* response);
 private:
  DataStoreCommandResponse::Status Inititalize(
      DataStoreCommand* command,
      DataStoreCommandResponse* response);
  DataStoreCommandResponse::Status Execute(DataStoreCommand* command);
  DataStoreCommandResponse::Status Query(
      DataStoreCommand* command,
      DataStoreCommandResponse* response);
  DataStoreCommandResponse::Status Migrate(DataStoreCommand* command);

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  bool initialized_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(SqliteDatastoreDriver);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SQLITE_DATASTORE_DRIVER_H_
