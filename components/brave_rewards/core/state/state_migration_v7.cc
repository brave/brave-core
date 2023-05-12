/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v7.h"

#include <string>

#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal::state {

void StateMigrationV7::Migrate(LegacyResultCallback callback) {
  const std::string brave = ledger().GetState<std::string>(kWalletBrave);

  if (!ledger().state()->SetEncryptedString(kWalletBrave, brave)) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  const std::string uphold = ledger().GetState<std::string>(kWalletUphold);

  if (!ledger().state()->SetEncryptedString(kWalletUphold, uphold)) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace brave_rewards::internal::state
