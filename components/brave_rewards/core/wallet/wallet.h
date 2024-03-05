/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_

#include <stdint.h>

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/wallet/wallet_balance.h"
#include "brave/components/brave_rewards/core/wallet/wallet_create.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace wallet {

class Wallet {
 public:
  explicit Wallet(RewardsEngineImpl& engine);
  ~Wallet();

  void CreateWalletIfNecessary(std::optional<std::string>&& geo_country,
                               CreateRewardsWalletCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  mojom::RewardsWalletPtr GetWallet();
  mojom::RewardsWalletPtr GetWallet(bool* corrupted);

  bool SetWallet(mojom::RewardsWalletPtr wallet);

 private:
  const raw_ref<RewardsEngineImpl> engine_;
  WalletCreate create_;
  WalletBalance balance_;
};

}  // namespace wallet
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_
