/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/common/mojom/rewards_types.mojom.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/bitflyer_transfer.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/connect_bitflyer_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/get_bitflyer_wallet.h"

namespace brave_rewards::internal::bitflyer {

Bitflyer::Bitflyer(RewardsEngineImpl& engine)
    : WalletProvider(engine), server_(engine) {
  connect_wallet_ = std::make_unique<ConnectBitFlyerWallet>(engine);
  get_wallet_ = std::make_unique<GetBitFlyerWallet>(engine);
  transfer_ = std::make_unique<BitFlyerTransfer>(engine);
}

const char* Bitflyer::WalletType() const {
  return constant::kWalletBitflyer;
}

void Bitflyer::FetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Bitflyer::OnFetchBalance, base::Unretained(this), std::move(callback));

  server_.get_balance().Request(wallet->token, std::move(url_callback));
}

std::string Bitflyer::GetFeeAddress() const {
  return bitflyer::GetFeeAddress();
}

}  // namespace brave_rewards::internal::bitflyer
