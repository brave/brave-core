/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v8.h"

#include <string>

#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::core {
namespace state {

StateMigrationV8::StateMigrationV8(LedgerImpl* ledger) : ledger_(ledger) {}

StateMigrationV8::~StateMigrationV8() = default;

void StateMigrationV8::Migrate(LegacyResultCallback callback) {
  const bool enabled = ledger_->ledger_client()->GetBooleanState("enabled");

  if (!enabled) {
    ledger_->ledger_client()->SetBooleanState(kAutoContributeEnabled, false);
  }

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace brave_rewards::core
