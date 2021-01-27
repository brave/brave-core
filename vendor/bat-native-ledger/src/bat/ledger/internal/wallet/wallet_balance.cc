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
  if (ledger_->ledger_client()->GetBooleanOption(
          option::kContributionsDisabledForBAPMigration)) {
    BLOG(1, "Fetch balance disabled for BAP migration");
    callback(type::Result::LEDGER_OK, type::Balance::New());
    return;
  }

  // if we don't have user funds in anon card anymore
  // we can skip balance server ping
  if (!ledger_->state()->GetFetchOldBalanceEnabled()) {
    auto balance = type::Balance::New();
    GetUnblindedTokens(std::move(balance), callback);
    return;
  }

  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(1, "Wallet is not created");
    ledger_->state()->SetFetchOldBalanceEnabled(false);
    auto balance = type::Balance::New();
    callback(type::Result::LEDGER_OK, std::move(balance));
    return;
  }

  if (wallet->payment_id.empty()) {
    BLOG(0, "Payment ID is empty");
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  auto load_callback = std::bind(&WalletBalance::OnFetch,
      this,
      _1,
      _2,
      callback);

  promotion_server_->get_wallet_balance()->Request(load_callback);
}

void WalletBalance::OnFetch(
    const type::Result result,
    type::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (result != type::Result::LEDGER_OK || !balance) {
    BLOG(0, "Couldn't fetch wallet balance");
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (balance->total == 0.0) {
    ledger_->state()->SetFetchOldBalanceEnabled(false);
  }

  GetUnblindedTokens(std::move(balance), callback);
}

void WalletBalance::GetUnblindedTokens(
    type::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    callback(type::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto tokens_callback = std::bind(&WalletBalance::OnGetUnblindedTokens,
      this,
      *balance,
      callback,
      _1);
  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {type::CredsBatchType::PROMOTION},
      tokens_callback);
}

void WalletBalance::OnGetUnblindedTokens(
    type::Balance info,
    ledger::FetchBalanceCallback callback,
    type::UnblindedTokenList list) {
  auto info_ptr = type::Balance::New(info);
  double total = 0.0;
  for (auto & item : list) {
    total+=item->value;
  }
  info_ptr->total += total;
  info_ptr->wallets.insert(std::make_pair(constant::kWalletUnBlinded, total));
  ExternalWallets(std::move(info_ptr), callback);
}

void WalletBalance::ExternalWallets(
    type::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  FetchBalanceUphold(std::move(balance), callback);
}

void WalletBalance::FetchBalanceUphold(type::BalancePtr balance,
                                       ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    callback(type::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto balance_callback = std::bind(&WalletBalance::OnFetchBalanceUphold, this,
                                    *balance, callback, _1, _2);

  auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    balance_callback(type::Result::LEDGER_OK, 0);
    return;
  }

  ledger_->uphold()->FetchBalance(balance_callback);
}

void WalletBalance::OnFetchBalanceUphold(type::Balance info,
                                         ledger::FetchBalanceCallback callback,
                                         type::Result result,
                                         double balance) {
  type::BalancePtr info_ptr = type::Balance::New(info);

  if (result == type::Result::LEDGER_OK) {
    info_ptr->wallets.insert(std::make_pair(constant::kWalletUphold, balance));
    info_ptr->total += balance;
  } else {
    BLOG(0, "Can't get uphold balance");
  }

  FetchBalanceBitflyer(std::move(info_ptr), callback);
}

void WalletBalance::FetchBalanceBitflyer(
    type::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    callback(type::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto balance_callback = std::bind(&WalletBalance::OnFetchBalanceBitflyer,
                                    this, *balance, callback, _1, _2);

  auto wallet = ledger_->bitflyer()->GetWallet();
  if (!wallet) {
    balance_callback(type::Result::LEDGER_OK, 0);
    return;
  }

  ledger_->bitflyer()->FetchBalance(balance_callback);
}

void WalletBalance::OnFetchBalanceBitflyer(
    type::Balance info,
    ledger::FetchBalanceCallback callback,
    type::Result result,
    double balance) {
  type::BalancePtr info_ptr = type::Balance::New(info);

  if (result == type::Result::LEDGER_OK) {
    info_ptr->wallets.insert(
        std::make_pair(constant::kWalletBitflyer, balance));
    info_ptr->total += balance;
  } else {
    BLOG(0, "Can't get bitflyer balance");
  }

  callback(result, std::move(info_ptr));
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
