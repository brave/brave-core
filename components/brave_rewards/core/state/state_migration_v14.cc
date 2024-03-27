/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v14.h"

#include <utility>
#include <vector>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal::state {

StateMigrationV14::StateMigrationV14(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV14::~StateMigrationV14() = default;

bool StateMigrationV14::MigrateExternalWallet(const std::string& wallet_type) {
  auto wallet = wallet::GetWallet(*engine_, wallet_type);
  if (wallet && wallet->status != mojom::WalletStatus::kNotConnected) {
    engine_->SetState(state::kExternalWalletType, wallet_type);
    return true;
  }
  return false;
}

void StateMigrationV14::Migrate(ResultCallback callback) {
  if (!engine_->GetState<std::string>(state::kExternalWalletType).empty()) {
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  std::vector providers{constant::kWalletBitflyer, constant::kWalletGemini,
                        constant::kWalletUphold, constant::kWalletZebPay};

  for (auto* provider : providers) {
    MigrateExternalWallet(provider);
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
