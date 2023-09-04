/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_

#include "brave/components/brave_rewards/core/wallet_provider/get_external_wallet.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace bitflyer {

class GetBitFlyerWallet : public wallet_provider::GetExternalWallet {
 public:
  explicit GetBitFlyerWallet(RewardsEngineImpl& engine);

  ~GetBitFlyerWallet() override;

 private:
  const char* WalletType() const override;
};

}  // namespace bitflyer
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_
