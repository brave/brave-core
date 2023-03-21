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

namespace ledger {
class LedgerImpl;

namespace database {

class DatabaseInitialize {
 public:
  explicit DatabaseInitialize(LedgerImpl* ledger);
  ~DatabaseInitialize();

  void Start(bool execute_create_script, ledger::LegacyResultCallback callback);

 private:
  void OnInitialize(mojom::DBCommandResponsePtr response,
                    bool execute_create_script,
                    ledger::LegacyResultCallback callback);

  void GetCreateScript(ledger::LegacyResultCallback callback);

  void ExecuteCreateScript(ledger::LegacyResultCallback callback,
                           const std::string& script,
                           int table_version);

  void OnExecuteCreateScript(mojom::DBCommandResponsePtr response,
                             int table_version,
                             ledger::LegacyResultCallback callback);

  std::unique_ptr<ledger::database::DatabaseMigration> migration_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
