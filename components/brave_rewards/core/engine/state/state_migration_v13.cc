/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/state/state_migration_v13.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/engine/global_constants.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/wallet/wallet_util.h"

namespace brave_rewards::internal::state {

StateMigrationV13::StateMigrationV13(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV13::~StateMigrationV13() = default;

bool StateMigrationV13::MigrateExternalWallet(const std::string& wallet_type) {
  if (!wallet::GetWalletIf(*engine_, wallet_type,
                           {mojom::WalletStatus::kConnected})) {
    engine_->Log(FROM_HERE)
        << "User doesn't have a connected " << wallet_type << " wallet.";
  } else {
    engine_->client()->ExternalWalletConnected();
  }

  return true;
}

void StateMigrationV13::Migrate(ResultCallback callback) {
  std::move(callback).Run(
      std::ranges::all_of(
          std::vector{constant::kWalletBitflyer, constant::kWalletGemini,
                      constant::kWalletUphold},
          [this](const std::string& wallet_type) {
            return MigrateExternalWallet(wallet_type);
          })
          ? mojom::Result::OK
          : mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal::state
