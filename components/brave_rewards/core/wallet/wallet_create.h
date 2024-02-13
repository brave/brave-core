/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace wallet {

class WalletCreate {
 public:
  explicit WalletCreate(RewardsEngineImpl& engine);

  void CreateWallet(std::optional<std::string>&& geo_country,
                    CreateRewardsWalletCallback callback);

 private:
  template <typename Result>
  void OnResult(CreateRewardsWalletCallback,
                std::optional<std::string>&& geo_country,
                Result&&);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace wallet
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
