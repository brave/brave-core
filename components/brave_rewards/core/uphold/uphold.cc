/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/uphold/uphold.h"

#include <memory>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/connect_uphold_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/uphold_transfer.h"

namespace brave_rewards::internal::uphold {

Uphold::Uphold(RewardsEngine& engine)
    : WalletProvider(engine), server_(engine) {
  connect_wallet_ = std::make_unique<ConnectUpholdWallet>(engine);
  transfer_ = std::make_unique<UpholdTransfer>(engine);
}

const char* Uphold::WalletType() const {
  return constant::kWalletUphold;
}

void Uphold::AssignWalletLinks(mojom::ExternalWallet& external_wallet) {
  auto url = engine_->Get<EnvironmentConfig>().uphold_oauth_url();

  external_wallet.account_url = url.Resolve("/dashboard").spec();

  if (!external_wallet.address.empty()) {
    external_wallet.activity_url =
        url.Resolve(base::StrCat({"/dashboard/cards/", external_wallet.address,
                                  "/activity"}))
            .spec();
  }
}

void Uphold::FetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Uphold::OnFetchBalance, base::Unretained(this), std::move(callback));

  server_.get_card().Request(wallet->address, wallet->token,
                             std::move(url_callback));
}

std::string Uphold::GetFeeAddress() const {
  return engine_->Get<EnvironmentConfig>().uphold_fee_address();
}

void Uphold::CheckEligibility() {
  static_cast<ConnectUpholdWallet*>(connect_wallet_.get())->CheckEligibility();
}

}  // namespace brave_rewards::internal::uphold
