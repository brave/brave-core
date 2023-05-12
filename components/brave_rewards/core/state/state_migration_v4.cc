/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v4.h"

#include <utility>

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::internal::state {

void StateMigrationV4::Migrate(ResultCallback callback) {
  ledger().client()->DeleteLog(std::move(callback));
}

}  // namespace brave_rewards::internal::state
