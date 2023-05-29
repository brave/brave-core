/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v9.h"

namespace brave_rewards::internal::state {

void StateMigrationV9::Migrate(LegacyResultCallback callback) {
  // In version 9, we attempted to set the "ac enabled" pref to false for users
  // in Japan as part of bitFlyer feature support. Later, it was determined
  // that Android users in Japan *should* be allowed to AC and this migration
  // was removed.
  callback(mojom::Result::LEDGER_OK);
}

}  // namespace brave_rewards::internal::state
