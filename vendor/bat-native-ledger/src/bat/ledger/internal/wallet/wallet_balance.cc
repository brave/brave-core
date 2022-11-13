/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_balance.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/option_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace wallet {

WalletBalance::WalletBalance(LedgerImpl* ledger) :
    ledger_(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
}

WalletBalance::~WalletBalance() = default;

void WalletBalance::Fetch(ledger::FetchBalanceCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(1, "Wallet is not created");
    std::move(callback).Run(mojom::Result::LEDGER_OK, mojom::Balance::New());
    return;
  }

  if (wallet->payment_id.empty()) {
    BLOG(0, "Payment ID is empty");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, nullptr);
    return;
  }

  GetUnblindedTokens(mojom::Balance::New(), std::move(callback));
}

void WalletBalance::GetUnblindedTokens(mojom::BalancePtr balance,
                                       ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto tokens_callback =
      base::BindOnce(&WalletBalance::OnGetUnblindedTokens,
                     base::Unretained(this), *balance, std::move(callback));
  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION},
      [callback = std::make_shared<decltype(tokens_callback)>(std::move(
           tokens_callback))](std::vector<mojom::UnblindedTokenPtr> list) {
        std::move(*callback).Run(std::move(list));
      });
}

void WalletBalance::OnGetUnblindedTokens(
    mojom::Balance info,
    ledger::FetchBalanceCallback callback,
    std::vector<mojom::UnblindedTokenPtr> list) {
  auto info_ptr = mojom::Balance::New(info);
  double total = 0.0;
  for (auto & item : list) {
    total+=item->value;
  }
  info_ptr->total += total;
  info_ptr->wallets.insert(std::make_pair(constant::kWalletUnBlinded, total));
  ExternalWallets(std::move(info_ptr), std::move(callback));
}

void WalletBalance::ExternalWallets(mojom::BalancePtr balance,
                                    ledger::FetchBalanceCallback callback) {
  FetchBalanceUphold(std::move(balance), std::move(callback));
}

void WalletBalance::FetchBalanceUphold(mojom::BalancePtr balance,
                                       ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  ledger_->uphold()->FetchBalance(
      base::BindOnce(&WalletBalance::OnFetchBalanceUphold,
                     base::Unretained(this), *balance, std::move(callback)));
}

void WalletBalance::OnFetchBalanceUphold(mojom::Balance info,
                                         ledger::FetchBalanceCallback callback,
                                         mojom::Result result,
                                         double balance) {
  mojom::BalancePtr info_ptr = mojom::Balance::New(info);

  if (result == mojom::Result::LEDGER_OK) {
    info_ptr->wallets.insert(std::make_pair(constant::kWalletUphold, balance));
    info_ptr->total += balance;
  } else {
    BLOG(0, "Can't get uphold balance");
  }

  FetchBalanceBitflyer(std::move(info_ptr), std::move(callback));
}

void WalletBalance::FetchBalanceBitflyer(
    mojom::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  ledger_->bitflyer()->FetchBalance(
      base::BindOnce(&WalletBalance::OnFetchBalanceBitflyer,
                     base::Unretained(this), *balance, std::move(callback)));
}

void WalletBalance::OnFetchBalanceBitflyer(
    mojom::Balance info,
    ledger::FetchBalanceCallback callback,
    mojom::Result result,
    double balance) {
  mojom::BalancePtr info_ptr = mojom::Balance::New(info);

  if (result == mojom::Result::LEDGER_OK) {
    info_ptr->wallets.insert(
        std::make_pair(constant::kWalletBitflyer, balance));
    info_ptr->total += balance;
  } else {
    BLOG(0, "Can't get bitflyer balance");
  }

  FetchBalanceGemini(std::move(info_ptr), std::move(callback));
}

void WalletBalance::FetchBalanceGemini(mojom::BalancePtr balance,
                                       ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  ledger_->gemini()->FetchBalance(
      base::BindOnce(&WalletBalance::OnFetchBalanceGemini,
                     base::Unretained(this), *balance, std::move(callback)));
}

void WalletBalance::OnFetchBalanceGemini(mojom::Balance info,
                                         ledger::FetchBalanceCallback callback,
                                         mojom::Result result,
                                         double balance) {
  mojom::BalancePtr info_ptr = mojom::Balance::New(info);

  if (result == mojom::Result::LEDGER_OK) {
    info_ptr->wallets.insert(std::make_pair(constant::kWalletGemini, balance));
    info_ptr->total += balance;
  } else {
    BLOG(0, "Can't get gemini balance");
  }

  std::move(callback).Run(result, std::move(info_ptr));
}

// static
double WalletBalance::GetPerWalletBalance(
    const std::string& type,
    base::flat_map<std::string, double> wallets) {
  if (type.empty() || wallets.size() == 0) {
    return 0.0;
  }

  for (const auto& wallet : wallets) {
    if (wallet.first == type) {
      return wallet.second;
    }
  }

  return  0.0;
}

}  // namespace wallet
}  // namespace ledger
