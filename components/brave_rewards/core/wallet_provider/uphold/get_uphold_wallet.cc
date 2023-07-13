/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/uphold/get_uphold_wallet.h"

#include "brave/components/brave_rewards/core/global_constants.h"

namespace brave_rewards::internal::uphold {

GetUpholdWallet::GetUpholdWallet(RewardsEngineImpl& engine)
    : wallet_provider::GetExternalWallet(engine) {}

GetUpholdWallet::~GetUpholdWallet() = default;

const char* GetUpholdWallet::WalletType() const {
  return constant::kWalletUphold;
}

}  // namespace brave_rewards::internal::uphold
