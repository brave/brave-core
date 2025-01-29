/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/bitflyer/bitflyer.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/engine/buildflags.h"
#include "brave/components/brave_rewards/core/engine/global_constants.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "brave/components/brave_rewards/core/engine/wallet_provider/bitflyer/bitflyer_transfer.h"
#include "brave/components/brave_rewards/core/engine/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

namespace brave_rewards::internal::bitflyer {

Bitflyer::Bitflyer(RewardsEngine& engine)
    : WalletProvider(engine), server_(engine) {
  connect_wallet_ = std::make_unique<ConnectBitFlyerWallet>(engine);
  transfer_ = std::make_unique<BitFlyerTransfer>(engine);
}

const char* Bitflyer::WalletType() const {
  return constant::kWalletBitflyer;
}

void Bitflyer::AssignWalletLinks(mojom::ExternalWallet& external_wallet) {
  auto url = engine_->Get<EnvironmentConfig>().bitflyer_url();
  external_wallet.account_url = url.Resolve("/ex/Home?login=1").spec();
  external_wallet.activity_url = url.Resolve("/ja-jp/ex/tradehistory").spec();
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
  return engine_->Get<EnvironmentConfig>().bitflyer_fee_address();
}

}  // namespace brave_rewards::internal::bitflyer
