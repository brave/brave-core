/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/gemini/gemini.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet_provider/gemini/connect_gemini_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/gemini/gemini_transfer.h"

namespace brave_rewards::internal::gemini {

Gemini::Gemini(RewardsEngineImpl& engine)
    : WalletProvider(engine), server_(engine) {
  connect_wallet_ = std::make_unique<ConnectGeminiWallet>(engine);
  transfer_ = std::make_unique<GeminiTransfer>(engine);
}

const char* Gemini::WalletType() const {
  return constant::kWalletGemini;
}

void Gemini::AssignWalletLinks(mojom::ExternalWallet& external_wallet) {
  external_wallet.account_url = GetAccountUrl();
  external_wallet.activity_url = GetActivityUrl();
}

void Gemini::FetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Gemini::OnFetchBalance, base::Unretained(this), std::move(callback));

  server_.post_balance().Request(wallet->token, std::move(url_callback));
}

std::string Gemini::GetFeeAddress() const {
  return gemini::GetFeeAddress();
}

base::TimeDelta Gemini::GetDelay() const {
  return base::Minutes(5);
}

}  // namespace brave_rewards::internal::gemini
