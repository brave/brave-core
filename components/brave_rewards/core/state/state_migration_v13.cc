/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v13.h"

#include <vector>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal::state {

bool StateMigrationV13::MigrateExternalWallet(const std::string& wallet_type) {
  if (!wallet::GetWalletIf(wallet_type, {mojom::WalletStatus::kConnected})) {
    BLOG(1, "User doesn't have a connected " << wallet_type << " wallet.");
  } else {
    ledger().client()->ExternalWalletConnected();
  }

  return true;
}

void StateMigrationV13::Migrate(LegacyResultCallback callback) {
  callback(base::ranges::all_of(
               std::vector{constant::kWalletBitflyer, constant::kWalletGemini,
                           constant::kWalletUphold},
               [this](const std::string& wallet_type) {
                 return MigrateExternalWallet(wallet_type);
               })
               ? mojom::Result::LEDGER_OK
               : mojom::Result::LEDGER_ERROR);
}

}  // namespace brave_rewards::internal::state
