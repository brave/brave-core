/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/legacy/media/media.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"

namespace ledger {

mojom::Environment _environment = mojom::Environment::PRODUCTION;

bool is_debug = false;
bool is_testing = false;
int state_migration_target_version_for_testing = -1;
int reconcile_interval = 0;  // minutes
int retry_interval = 0;      // seconds

// static
Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new LedgerImpl(client);
}

}  // namespace ledger
