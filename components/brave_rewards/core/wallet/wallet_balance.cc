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
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

namespace brave_rewards::internal::wallet {

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

  const auto wallet_type =
      engine_->GetState<std::string>(state::kExternalWalletType);
  if (wallet_type.empty()) {
    return std::move(callback).Run(std::move(balance));
  }

  auto* provider = engine_->GetExternalWalletProvider(wallet_type);
  if (!provider) {
    engine_->LogError(FROM_HERE) << "Invalid external wallet type";
    return std::move(callback).Run(std::move(balance));
  }

  provider->FetchBalance(base::BindOnce(
      &WalletBalance::OnFetchExternalWalletBalance, base::Unretained(this),
      wallet_type, std::move(balance), std::move(callback)));
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
    engine_->LogError(FROM_HERE)
        << "Failed to fetch balance for " << wallet_type << " wallet";
    std::move(callback).Run(nullptr);
  }
}

}  // namespace brave_rewards::internal::wallet
