/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet_balance.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal::wallet {

namespace {
std::string GetConnectedWalletType(RewardsEngineImpl& engine) {
  return GetWalletIf(engine, constant::kWalletBitflyer,
                     {mojom::WalletStatus::kConnected})
             ? constant::kWalletBitflyer
         : GetWalletIf(engine, constant::kWalletGemini,
                       {mojom::WalletStatus::kConnected})
             ? constant::kWalletGemini
         : GetWalletIf(engine, constant::kWalletUphold,
                       {mojom::WalletStatus::kConnected})
             ? constant::kWalletUphold
         : GetWalletIf(engine, constant::kWalletZebPay,
                       {mojom::WalletStatus::kConnected})
             ? constant::kWalletZebPay
             : "";
}
}  // namespace

WalletBalance::WalletBalance(RewardsEngineImpl& engine) : engine_(engine) {}

WalletBalance::~WalletBalance() = default;

void WalletBalance::Fetch(FetchBalanceCallback callback) {
  auto tokens_callback =
      base::BindOnce(&WalletBalance::OnGetUnblindedTokens,
                     base::Unretained(this), std::move(callback));

  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION},
      [callback = std::make_shared<decltype(tokens_callback)>(std::move(
           tokens_callback))](std::vector<mojom::UnblindedTokenPtr> list) {
        std::move(*callback).Run(std::move(list));
      });
}

void WalletBalance::OnGetUnblindedTokens(
    FetchBalanceCallback callback,
    std::vector<mojom::UnblindedTokenPtr> tokens) {
  double total = 0.0;
  for (const auto& token : tokens) {
    total += token->value;
  }

  auto balance = mojom::Balance::New();
  balance->total = total;
  balance->wallets.emplace(constant::kWalletUnBlinded, balance->total);

  const auto wallet_type = GetConnectedWalletType(*engine_);
  if (wallet_type.empty()) {
    return std::move(callback).Run(std::move(balance));
  }

  wallet::FetchBalance(
      *engine_, wallet_type,
      base::BindOnce(&WalletBalance::OnFetchExternalWalletBalance,
                     base::Unretained(this), wallet_type, std::move(balance),
                     std::move(callback)));
}

void WalletBalance::OnFetchExternalWalletBalance(const std::string& wallet_type,
                                                 mojom::BalancePtr balance_ptr,
                                                 FetchBalanceCallback callback,
                                                 mojom::Result result,
                                                 double balance) {
  if (result == mojom::Result::OK) {
    DCHECK(balance_ptr);
    balance_ptr->total += balance;
    balance_ptr->wallets.emplace(wallet_type, balance);
    std::move(callback).Run(std::move(balance_ptr));
  } else {
    BLOG(0, "Failed to fetch balance for " << wallet_type << " wallet!");

    std::move(callback).Run(
        base::unexpected(result == mojom::Result::EXPIRED_TOKEN
                             ? mojom::FetchBalanceError::kAccessTokenExpired
                             : mojom::FetchBalanceError::kUnexpected));
  }
}

}  // namespace brave_rewards::internal::wallet
