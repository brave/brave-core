/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/bitflyer/bitflyer_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace bitflyer {

BitflyerWallet::BitflyerWallet(LedgerImpl* ledger) : ledger_(ledger) {}

BitflyerWallet::~BitflyerWallet() = default;

void BitflyerWallet::Generate(ledger::ResultCallback callback) {
  auto wallet = GetWallet(ledger_);
  if (!wallet) {
    wallet = type::ExternalWallet::New();
    wallet->type = constant::kWalletBitflyer;
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = GenerateRandomString(ledger::is_testing);
  }

  if (wallet->token.empty() &&
      (wallet->status == type::WalletStatus::PENDING ||
       wallet->status == type::WalletStatus::CONNECTED)) {
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  wallet = GenerateLinks(std::move(wallet));
  ledger_->bitflyer()->SetWallet(wallet->Clone());

  if (wallet->status == type::WalletStatus::VERIFIED) {
    // If the wallet is verified, attempt to transfer any applicable grants to
    // the user's external wallet.
    //
    // For Uphold, this is accomplished by calling ledger->wallet()->ClaimFunds
    // as the last step of the GenerateWallet flow. ClaimFunds performs both
    // Uphold wallet linking and attempts to drain legacy Brave user funds to
    // that linked wallet. For bitFlyer, wallet linking is performed during
    // authorization, so bypass ClaimFunds and call promotion()->TransferTokens
    // directly.
    ledger_->promotion()->TransferTokens([callback](const type::Result result) {
      if (result != type::Result::LEDGER_OK) {
        BLOG(0, "Claiming tokens failed");
        callback(type::Result::CONTINUE);
        return;
      }
      callback(type::Result::LEDGER_OK);
    });
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace bitflyer
}  // namespace ledger
