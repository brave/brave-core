/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V13_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V13_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::state {

class StateMigrationV13 {
 public:
  void Migrate(LegacyResultCallback);

 private:
  bool MigrateExternalWallet(const std::string& wallet_type);
};

}  // namespace brave_rewards::internal::state

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V13_H_
