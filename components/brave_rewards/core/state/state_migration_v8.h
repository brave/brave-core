/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V8_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V8_H_

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal {
class LedgerImpl;

namespace state {

class StateMigrationV8 {
 public:
  explicit StateMigrationV8(LedgerImpl& ledger);
  ~StateMigrationV8();

  void Migrate(LegacyResultCallback callback);
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V8_H_
