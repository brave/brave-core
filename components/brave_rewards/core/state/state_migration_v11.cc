/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v11.h"

#include "base/check.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal::state {

void StateMigrationV11::Migrate(LegacyResultCallback callback) {
  // In version 7 encryption was added for |kWalletBrave|. However due to wallet
  // corruption, users copying their profiles to new computers or reinstalling
  // their operating system we are reverting this change

  const auto decrypted_wallet =
      ledger().state()->GetEncryptedString(kWalletBrave);
  if (decrypted_wallet) {
    ledger().SetState(kWalletBrave, decrypted_wallet.value());
  }

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace brave_rewards::internal::state
