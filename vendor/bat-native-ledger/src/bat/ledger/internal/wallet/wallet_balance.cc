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
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace wallet {

WalletBalance::WalletBalance(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    promotion_server_(new endpoint::PromotionServer(ledger)) {
}

WalletBalance::~WalletBalance() = default;

void WalletBalance::Fetch(ledger::FetchBalanceCallback callback) {
  // if we don't have user funds in anon card anymore
  // we can skip balance server ping
  if (!ledger_->state()->GetFetchOldBalanceEnabled()) {
    auto balance = ledger::Balance::New();
    GetUnblindedTokens(std::move(balance), callback);
    return;
  }

  if (ledger_->state()->GetPaymentId().empty()) {
    BLOG(0, "Payment ID is empty");
    callback(ledger::Result::LEDGER_ERROR, nullptr);
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
    const ledger::Result result,
    ledger::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !balance) {
    BLOG(0, "Couldn't fetch wallet balance");
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (balance->total == 0.0) {
    ledger_->state()->SetFetchOldBalanceEnabled(false);
  }

  GetUnblindedTokens(std::move(balance), callback);
}

void WalletBalance::GetUnblindedTokens(
    ledger::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    callback(ledger::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto tokens_callback = std::bind(&WalletBalance::OnGetUnblindedTokens,
      this,
      *balance,
      callback,
      _1);
  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {ledger::CredsBatchType::PROMOTION},
      tokens_callback);
}

void WalletBalance::OnGetUnblindedTokens(
    ledger::Balance info,
    ledger::FetchBalanceCallback callback,
    ledger::UnblindedTokenList list) {
  auto info_ptr = ledger::Balance::New(info);
  double total = 0.0;
  for (auto & item : list) {
    total+=item->value;
  }
  info_ptr->total += total;
  info_ptr->wallets.insert(std::make_pair(ledger::kWalletUnBlinded, total));
  ExternalWallets(std::move(info_ptr), callback);
}

void WalletBalance::ExternalWallets(
    ledger::BalancePtr balance,
    ledger::FetchBalanceCallback callback) {
  if (!balance) {
    BLOG(0, "Balance is null");
    callback(ledger::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();

  if (wallets.empty()) {
    callback(ledger::Result::LEDGER_OK, std::move(balance));
    return;
  }

  auto uphold_callback = std::bind(&WalletBalance::OnUpholdFetchBalance,
                                   this,
                                   *balance,
                                   callback,
                                   _1,
                                   _2);

  uphold_->FetchBalance(uphold_callback);
}

void WalletBalance::OnUpholdFetchBalance(ledger::Balance info,
                                   ledger::FetchBalanceCallback callback,
                                   ledger::Result result,
                                   double balance) {
  ledger::BalancePtr info_ptr = ledger::Balance::New(info);

  if (result == ledger::Result::LEDGER_ERROR) {
    BLOG(0, "Can't get uphold balance");
    callback(ledger::Result::LEDGER_ERROR, std::move(info_ptr));
    return;
  }

  info_ptr->wallets.insert(std::make_pair(ledger::kWalletUphold, balance));
  info_ptr->total += balance;
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
