/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_DATABASE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/ledger_client.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace brave_rewards {

class RewardsDatabase {
 public:
  explicit RewardsDatabase(const base::FilePath& db_path);

  ~RewardsDatabase();

  void RunTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::DBCommandResponse* command_response);

 private:
  ledger::DBCommandResponse::Status Initialize(
      const int32_t version,
      const int32_t compatible_version,
      ledger::DBCommandResponse* command_response);

  ledger::DBCommandResponse::Status Execute(ledger::DBCommand* command);

  ledger::DBCommandResponse::Status Run(ledger::DBCommand* command);

  ledger::DBCommandResponse::Status Read(
      ledger::DBCommand* command,
      ledger::DBCommandResponse* command_response);

  ledger::DBCommandResponse::Status Migrate(
      const int32_t version,
      const int32_t compatible_version);

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  bool initialized_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(RewardsDatabase);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_DATABASE_H_
