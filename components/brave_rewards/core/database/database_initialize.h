/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_

#include "brave/components/brave_rewards/core/database/database_migration.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::database {

class DatabaseInitialize {
 public:
  void Start(LegacyResultCallback callback);

 private:
  void OnInitialize(mojom::DBCommandResponsePtr response,
                    LegacyResultCallback callback);

  DatabaseMigration migration_;
};

}  // namespace brave_rewards::internal::database

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
