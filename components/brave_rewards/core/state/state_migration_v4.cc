/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v4.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace state {

StateMigrationV4::StateMigrationV4(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV4::~StateMigrationV4() = default;

void StateMigrationV4::Migrate(LegacyResultCallback callback) {
  ledger_->ledger_client()->DeleteLog(callback);
}

}  // namespace state
}  // namespace brave_rewards::core
