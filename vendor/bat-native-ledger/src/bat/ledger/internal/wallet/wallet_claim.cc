/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_claim.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

}  // namespace

namespace ledger {
namespace wallet {

WalletClaim::WalletClaim(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    promotion_server_(new endpoint::PromotionServer(ledger)) {
}

WalletClaim::~WalletClaim() = default;

void WalletClaim::Start(ledger::ResultCallback callback) {
  ledger_->wallet()->FetchBalance(std::bind(&WalletClaim::OnBalance,
      this,
      _1,
      _2,
      callback));
}

void WalletClaim::OnBalance(
    const ledger::Result result,
    ledger::BalancePtr balance,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !balance) {
    BLOG(0, "Anon funds transfer failed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (ledger_->state()->GetAnonTransferChecked() &&
      balance->user_funds == 0) {
    BLOG(1, "Second ping with zero balance");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = braveledger_uphold::GetWallet(std::move(wallets));

  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto url_callback = std::bind(&WalletClaim::OnTransferFunds,
      this,
      _1,
      callback);

  promotion_server_->post_claim_uphold()->Request(
      balance->user_funds,
      url_callback);
}

void WalletClaim::OnTransferFunds(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->state()->SetAnonTransferChecked(true);
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (result == ledger::Result::ALREADY_EXISTS) {
    ledger_->state()->SetAnonTransferChecked(true);
    callback(ledger::Result::ALREADY_EXISTS);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR);
}

}  // namespace wallet
}  // namespace ledger
