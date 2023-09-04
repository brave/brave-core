/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v11.h"

#include "base/check.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal {
namespace state {

StateMigrationV11::StateMigrationV11(RewardsEngineImpl& engine)
    : engine_(engine) {}

StateMigrationV11::~StateMigrationV11() = default;

void StateMigrationV11::Migrate(LegacyResultCallback callback) {
  // In version 7 encryption was added for |kWalletBrave|. However due to wallet
  // corruption, users copying their profiles to new computers or reinstalling
  // their operating system we are reverting this change

  const auto decrypted_wallet =
      engine_->state()->GetEncryptedString(kWalletBrave);
  if (decrypted_wallet) {
    engine_->SetState(kWalletBrave, decrypted_wallet.value());
  }

  callback(mojom::Result::OK);
}

}  // namespace state
}  // namespace brave_rewards::internal
