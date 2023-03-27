/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V3_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V3_H_

#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
namespace state {

class StateMigrationV3 {
 public:
  StateMigrationV3();
  ~StateMigrationV3();

  void Migrate(LegacyResultCallback callback);
};

}  // namespace state
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V3_H_
