/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/balance.h"

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_wallet {

Balance::Balance(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

Balance::~Balance() {
}

void Balance::Fetch(ledger::FetchBalanceCallback callback) {
  auto wallet_callback = std::bind(&Balance::OnWalletProperties,
                            this,
                            _1,
                            _2,
                            callback);
  ledger_->FetchWalletProperties(wallet_callback);
}

void Balance::OnWalletProperties(
    const ledger::Result result,
    ledger::WalletPropertiesPtr properties,
    ledger::FetchBalanceCallback callback) {
  ledger::BalancePtr balance = ledger::Balance::New();

  balance->alt_currency = properties->alt_currency;
  balance->probi = properties->probi;
  balance->total = properties->balance;
  balance->rates = properties->rates;

  callback(result, std::move(balance));
}

}  // namespace braveledger_wallet
