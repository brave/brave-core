/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/legacy/bat_state.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV2 {
 public:
  explicit StateMigrationV2(LedgerImpl* ledger);
  ~StateMigrationV2();

  void Migrate(ledger::LegacyResultCallback callback);

 private:
  void OnLoadState(mojom::Result result, ledger::LegacyResultCallback callback);

  std::unique_ptr<braveledger_bat_state::LegacyBatState> legacy_state_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_
