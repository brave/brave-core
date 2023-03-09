/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_balance.h"

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "bat/ledger/option_keys.h"

namespace ledger::wallet {

WalletBalance::WalletBalance(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

WalletBalance::~WalletBalance() = default;

void WalletBalance::Fetch(ledger::FetchBalanceCallback callback) {
  auto tokens_callback =
      base::BindOnce(&WalletBalance::OnGetUnblindedTokens,
                     base::Unretained(this), std::move(callback));

  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION},
      [callback = std::make_shared<decltype(tokens_callback)>(std::move(
           tokens_callback))](std::vector<mojom::UnblindedTokenPtr> list) {
        std::move(*callback).Run(std::move(list));
      });
}

void WalletBalance::OnGetUnblindedTokens(
    ledger::FetchBalanceCallback callback,
    std::vector<mojom::UnblindedTokenPtr> tokens) {
  double total = 0.0;
  for (const auto& token : tokens) {
    total += token->value;
  }

  auto balance = mojom::Balance::New();
  balance->total = total;
  balance->wallets.emplace(constant::kWalletUnBlinded, balance->total);

  const auto wallet_type =
      ledger_->ledger_client()->GetStringState(state::kExternalWalletType);
  if (wallet_type.empty() ||
      !wallet::GetWalletIf(ledger_, wallet_type,
                           {mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(std::move(balance));
  }

  wallet::FetchBalance(
      ledger_, wallet_type,
      base::BindOnce(&WalletBalance::OnFetchExternalWalletBalance,
                     base::Unretained(this), wallet_type, std::move(balance),
                     std::move(callback)));
}

void WalletBalance::OnFetchExternalWalletBalance(
    const std::string& wallet_type,
    mojom::BalancePtr balance_ptr,
    ledger::FetchBalanceCallback callback,
    mojom::Result result,
    double balance) {
  if (result == mojom::Result::LEDGER_OK) {
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

}  // namespace ledger::wallet
