/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_DATABASE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace brave_rewards::internal {

class RewardsDatabase {
 public:
  explicit RewardsDatabase(const base::FilePath& path);

  RewardsDatabase(const RewardsDatabase&) = delete;
  RewardsDatabase& operator=(const RewardsDatabase&) = delete;

  ~RewardsDatabase();

  mojom::DBCommandResponsePtr RunTransaction(
      mojom::DBTransactionPtr transaction);

  sql::Database& GetInternalDatabaseForTesting() { return db_; }

 private:
  mojom::DBCommandResponsePtr Initialize(int32_t version,
                                         int32_t compatible_version);

  mojom::DBCommandResponsePtr Execute(const mojom::DBCommand& command);

  mojom::DBCommandResponsePtr Run(const mojom::DBCommand& command);

  mojom::DBCommandResponsePtr Read(const mojom::DBCommand& command);

  mojom::DBCommandResponsePtr Migrate(int32_t version,
                                      int32_t compatible_version);

  mojom::DBRecordPtr GetLastChangeCount();

  bool ShouldCreateTables();

  int GetTablesCount();

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  bool initialized_ = false;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_DATABASE_H_
