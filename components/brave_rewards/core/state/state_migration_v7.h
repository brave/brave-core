/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V7_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V7_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV7 {
 public:
  explicit StateMigrationV7(LedgerImpl& ledger);
  ~StateMigrationV7();

  void Migrate(ledger::LegacyResultCallback callback);

 private:
  const raw_ref<LedgerImpl> ledger_;
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V7_H_
