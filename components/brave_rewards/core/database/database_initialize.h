/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/core/database/database_migration.h"
#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace database {

class DatabaseInitialize {
 public:
  explicit DatabaseInitialize(LedgerImpl* ledger);
  ~DatabaseInitialize();

  void Start(bool execute_create_script, LegacyResultCallback callback);

 private:
  void OnInitialize(mojom::DBCommandResponsePtr response,
                    bool execute_create_script,
                    LegacyResultCallback callback);

  void GetCreateScript(LegacyResultCallback callback);

  void ExecuteCreateScript(const std::string& script,
                           int table_version,
                           LegacyResultCallback callback);

  void OnExecuteCreateScript(mojom::DBCommandResponsePtr response,
                             int table_version,
                             LegacyResultCallback callback);

  std::unique_ptr<database::DatabaseMigration> migration_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
