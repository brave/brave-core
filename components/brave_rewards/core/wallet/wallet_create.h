/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class WalletCreate {
 public:
  explicit WalletCreate(LedgerImpl*);

  void CreateWallet(absl::optional<std::string>&& geo_country,
                    CreateRewardsWalletCallback callback);

 private:
  template <typename Result>
  void OnResult(CreateRewardsWalletCallback,
                absl::optional<std::string>&& geo_country,
                Result&&);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_CREATE_H_
