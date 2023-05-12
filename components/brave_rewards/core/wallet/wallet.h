/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_

#include <stdint.h>

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/wallet/wallet_balance.h"
#include "brave/components/brave_rewards/core/wallet/wallet_create.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal::wallet {

class Wallet {
 public:
  void CreateWalletIfNecessary(absl::optional<std::string>&& geo_country,
                               CreateRewardsWalletCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  mojom::RewardsWalletPtr GetWallet();
  mojom::RewardsWalletPtr GetWallet(bool* corrupted);

  bool SetWallet(mojom::RewardsWalletPtr wallet);

 private:
  WalletCreate create_;
  WalletBalance balance_;
  endpoint::PromotionServer promotion_server_;
};

}  // namespace brave_rewards::internal::wallet

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_H_
