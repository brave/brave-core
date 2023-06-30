/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/get_bitflyer_wallet.h"

#include "brave/components/brave_rewards/core/global_constants.h"

namespace brave_rewards::internal::bitflyer {

GetBitFlyerWallet::GetBitFlyerWallet(RewardsEngineImpl& engine)
    : wallet_provider::GetExternalWallet(engine) {}

GetBitFlyerWallet::~GetBitFlyerWallet() = default;

const char* GetBitFlyerWallet::WalletType() const {
  return constant::kWalletBitflyer;
}

}  // namespace brave_rewards::internal::bitflyer
