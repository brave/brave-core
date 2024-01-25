/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V9_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V9_H_

#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal::state {

class StateMigrationV9 {
 public:
  StateMigrationV9();
  ~StateMigrationV9();

  void Migrate(ResultCallback callback);
};

}  // namespace brave_rewards::internal::state

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V9_H_
