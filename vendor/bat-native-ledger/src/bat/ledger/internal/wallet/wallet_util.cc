/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/wallet/wallet_util.h"

namespace braveledger_wallet {

ledger::ExternalWalletPtr GetWallet(
    const std::string& wallet_type,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  for (auto& wallet : wallets) {
    if (wallet.first == wallet_type) {
      return std::move(wallet.second);
    }
  }

  return nullptr;
}

ledger::ExternalWalletPtr ResetWallet(ledger::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  wallet->token = "";
  wallet->address = "";
  wallet->user_name = "";
  wallet->one_time_string = "";

  if (wallet->status == ledger::WalletStatus::VERIFIED) {
    wallet->status = ledger::WalletStatus::DISCONNECTED_VERIFIED;
  } else {
    wallet->status = ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  }

  return wallet;
}

}  // namespace braveledger_wallet
