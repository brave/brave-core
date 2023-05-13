/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal {
class LedgerImpl;

namespace wallet {

class WalletCreate {
 public:
  explicit WalletCreate(LedgerImpl& ledger);

  void CreateWallet(absl::optional<std::string>&& geo_country,
                    CreateRewardsWalletCallback callback);

 private:
  template <typename Result>
  void OnResult(CreateRewardsWalletCallback,
                absl::optional<std::string>&& geo_country,
                Result&&);
};

}  // namespace wallet
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
