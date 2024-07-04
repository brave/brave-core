/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v7.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal::state {

StateMigrationV7::StateMigrationV7(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV7::~StateMigrationV7() = default;

void StateMigrationV7::Migrate(ResultCallback callback) {
  const std::string brave = engine_->GetState<std::string>(kWalletBrave);

  if (!engine_->state()->SetEncryptedString(kWalletBrave, brave)) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  const std::string uphold = engine_->GetState<std::string>(kWalletUphold);

  if (!engine_->state()->SetEncryptedString(kWalletUphold, uphold)) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
