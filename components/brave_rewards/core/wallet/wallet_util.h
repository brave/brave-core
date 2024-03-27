/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_UTIL_H_

#include <set>
#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace wallet {

mojom::ExternalWalletPtr GetWallet(RewardsEngine& engine,
                                   const std::string& wallet_type);

mojom::ExternalWalletPtr GetWalletIf(RewardsEngine& engine,
                                     const std::string& wallet_type,
                                     const std::set<mojom::WalletStatus>&);

bool SetWallet(RewardsEngine& engine, mojom::ExternalWalletPtr);

mojom::ExternalWalletPtr TransitionWallet(
    RewardsEngine& engine,
    absl::variant<mojom::ExternalWalletPtr, std::string> wallet_info,
    mojom::WalletStatus to);

mojom::ExternalWalletPtr MaybeCreateWallet(RewardsEngine& engine,
                                           const std::string& wallet_type);

bool LogOutWallet(RewardsEngine& engine,
                  const std::string& wallet_type,
                  const std::string& notification = "");

}  // namespace wallet
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_UTIL_H_
