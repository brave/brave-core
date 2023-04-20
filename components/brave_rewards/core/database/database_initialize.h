/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/database/database_migration.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace database {

class DatabaseInitialize {
 public:
  explicit DatabaseInitialize(LedgerImpl& ledger);
  ~DatabaseInitialize();

  void Start(ledger::LegacyResultCallback callback);

 private:
  void OnInitialize(mojom::DBCommandResponsePtr response,
                    ledger::LegacyResultCallback callback);

  const raw_ref<LedgerImpl> ledger_;
  ledger::database::DatabaseMigration migration_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
